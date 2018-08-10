#include <iostream>
#include "logging.hpp"
#include "http_server.hpp"
#include "pdf_utils.hpp"
#include "string_utils.hpp"

int main(int argc, char* argv[]) {
    try {
        // Check command line arguments.
        if (argc < 3) {
            std::cerr << "Usage: http_server_fast <address> <port> <number_of_workers>\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    http_server_fast 0.0.0.0 8080 100\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    http_server_fast 0::0 8080 100\n";
            return EXIT_FAILURE;
        }

        auto const address = boost::asio::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));
        int num_workers = std::atoi(argv[3]);

        // assume that ioc is accessed from single thread
        boost::asio::io_context ioc{1};
        boost::asio::ip::tcp::acceptor acceptor{ioc, {address, port}};

        std::list<http_worker> workers;
        for (int i = 0; i < num_workers; ++i) {
            workers.emplace_back(acceptor);
            workers.back().start();
        }

        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    LOG_INFO << "Server stopped";
    return EXIT_SUCCESS;
}
