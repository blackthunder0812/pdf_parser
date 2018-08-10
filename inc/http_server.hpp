#pragma once
#include "fields_alloc.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#ifndef BOOST_BEAST_READ_HEADER_BUFFER
#define BOOST_BEAST_READ_HEADER_BUFFER 8192
#endif

#ifndef BOOST_BEAST_READ_BODY_BUFFER
#define BOOST_BEAST_READ_BODY_BUFFER 10*1024*2014
#endif

class http_worker {
  public:
    // disable copy constructor and copy assignment (non-copyable)
    http_worker(http_worker const&) = delete;
    http_worker& operator=(http_worker const&) = delete;

    http_worker(boost::asio::ip::tcp::acceptor& acceptor);

    void start();

  private:
    using alloc_t = fields_alloc<char>;
    using request_body_t = boost::beast::http::basic_dynamic_body<boost::beast::flat_static_buffer<BOOST_BEAST_READ_BODY_BUFFER>>;

    // The acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor& acceptor_;

    // The socket for the currently connected client.
    boost::asio::ip::tcp::socket socket_{acceptor_.get_executor().context()};

    // The buffer for performing reads
    boost::beast::flat_static_buffer<BOOST_BEAST_READ_HEADER_BUFFER> buffer_;

    // The allocator used for the fields in the request and reply.
    alloc_t alloc_{BOOST_BEAST_READ_HEADER_BUFFER};

    // The parser for reading the requests
    boost::optional<boost::beast::http::request_parser<request_body_t, alloc_t>> parser_;

    // The timer putting a time limit on requests.
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> request_deadline_{acceptor_.get_executor().context(), (std::chrono::steady_clock::time_point::max)()};

    // The string-based response message.
    boost::optional<boost::beast::http::response<boost::beast::http::string_body, boost::beast::http::basic_fields<alloc_t>>> string_response_;

    // The string-based response serializer.
    boost::optional<boost::beast::http::response_serializer<boost::beast::http::string_body, boost::beast::http::basic_fields<alloc_t>>> string_serializer_;

    // The file-based response message.
    boost::optional<boost::beast::http::response<boost::beast::http::file_body, boost::beast::http::basic_fields<alloc_t>>> file_response_;

    // The file-based response serializer.
    boost::optional<boost::beast::http::response_serializer<boost::beast::http::file_body, boost::beast::http::basic_fields<alloc_t>>> file_serializer_;

    void accept();

    void read_request();

    bool url_decode(const std::string& in, std::string& out);

    void process_request(boost::beast::http::request<request_body_t, boost::beast::http::basic_fields<alloc_t>> const& req);

    std::map<std::string, std::string> parse(const std::string &query);

    void send_bad_response(boost::beast::http::status status, std::string const& error);

    void check_deadline();
};
