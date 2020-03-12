#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace pt = boost::property_tree;
namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

// Performs an HTTP GET and prints the response
int main(int argc, char** argv)
{
    try
    {
        // Check command line arguments.
        if(argc != 4 && argc != 5)
        {
            std::cerr <<
                "Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
                "Example:\n" <<
                "    http-client-sync www.example.com 80 /\n" <<
                "    http-client-sync www.example.com 80 / 1.0\n";
            return EXIT_FAILURE;
        }
        auto const host = argv[1];
        auto const port = argv[2];
        auto const target = argv[3];
        std::cout << argv[1] <<" - " << argv[2] << " - "<< argv[3] << "\n";
        // return 1;
        int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host,port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP POST request message
        http::request<http::string_body> req{http::verb::post, target, version};
        req.set(http::field::host, host);
        req.set(beast::http::field::content_type, "application/json; charset=utf-8");

        // JSON POST message
        // {
        //     "signature_name" : "serving_default",
        //     "instances" : [                
        //         {
		// 		       "image": 
        //                 {
        //                     "b64": "aW1hZ2UgYnl0ZXM=",
        //                 }
        //         },
        //     ]
		// }
        pt::ptree request;
        request.put("signature_name", "serving_default");
        pt::ptree image;
        image.put("b64", "aW1hZ2UgYnl0ZXM=");
        pt::ptree instance_node;
        instance_node.add_child("image",image);      
        pt::ptree instances_node;
        instances_node.push_back(std::make_pair("", instance_node));
        request.add_child("instances", instances_node);

        // Print the generated JSON
        pt::write_json(std::cout, request);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Response JSON message
        // {
        //     "predictions":[
        //         {
        //             "classes": int,
        //             "probabilities": [ ],
        //         },
        //     ]
        // }
        pt::ptree response;
        pt::read_json(res, response);
        pt::write_json(std::cout, response);

        // Write the message to standard out
        std::cout << res << std::endl;

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        // If we get here then the connection is closed gracefully
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}