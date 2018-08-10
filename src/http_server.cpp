#include "http_server.hpp"
#include "logging.hpp"
#include "pdf_utils.hpp"
#include "string_utils.hpp"
#include <boost/beast/core.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <regex>
#include <memory>
#include <string>
#include "pdf_utils.hpp"

http_worker::http_worker(boost::asio::ip::tcp::acceptor& acceptor) :
    acceptor_(acceptor){
}

void http_worker::start() {
    accept();
    check_deadline();
}

void http_worker::accept() {
    // Clean up any previous connection.
    boost::beast::error_code ec;
    socket_.close(ec);
    buffer_.consume(buffer_.size());

    acceptor_.async_accept(
        socket_,
    [this](boost::beast::error_code ec) {
        if (ec) {
            LOG_ERROR << "Error code: " << ec.value() << " " << ec.message();
            accept();
        } else {
            LOG_INFO << "Accepted request from " << socket_.remote_endpoint().address() << ":" << socket_.remote_endpoint().port();

            // Request must be fully processed within 60 seconds.
            request_deadline_.expires_after(std::chrono::seconds(60));

            read_request();
        }
    });
}

void http_worker::read_request() {
    // On each read the parser needs to be destroyed and
    // recreated. We store it in a boost::optional to
    // achieve that.
    //
    // Arguments passed to the parser constructor are
    // forwarded to the message object. A single argument
    // is forwarded to the body constructor.
    //
    // We construct the dynamic body with a 10MB limit
    // to prevent vulnerability to buffer attacks.
    //
    parser_.emplace(
        std::piecewise_construct,
        std::make_tuple(),
        std::make_tuple(alloc_));

    boost::beast::http::async_read(socket_,
                                   buffer_,
                                   *parser_,
    [this](boost::beast::error_code ec, std::size_t) {
        if (ec) {
            LOG_ERROR << "Error code: " << ec.value() << " " << ec.message();
            accept();
        } else {
            process_request(parser_->get());
        }
    });
}

bool http_worker::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

std::map<std::string, std::string> http_worker::parse(const std::string& query) {
    std::map<std::string, std::string> data;
    std::regex pattern("([\\w+%]+)=([^&]*)");
    auto words_begin = std::sregex_iterator(query.begin(), query.end(), pattern);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; i++)
    {
        std::string key = (*i)[1].str();
        std::string value = (*i)[2].str();
        data[key] = value;
    }

    return data;
}

void http_worker::process_request(boost::beast::http::request<request_body_t, boost::beast::http::basic_fields<alloc_t>> const& req) {
    switch (req.method()) {
        case boost::beast::http::verb::get: {
            /* request parameters:
             *   - required: p   : pdf file path
             *   - optional: opw : owner password
             *   - optional: upw : user password
             */
                boost::beast::string_view target_pdf_file(req.target());
                std::map<std::string, std::string> params = parse(target_pdf_file.to_string());
                std::string request_path;
                url_decode(params.at("path"), request_path);
                LOG_INFO << "Processing request from " << socket_.remote_endpoint().address() << ":" << socket_.remote_endpoint().port() << " PDF file: " << request_path;

                // Create PDFDoc
                char* owner_password = nullptr;
                if (params.find("opw") != params.end()) {
                    owner_password = new char[params.at("opw").length()];
                    std::strcpy(owner_password, params.at("opw").c_str());
                }
                char* user_password = nullptr;
                if (params.find("upw") != params.end()) {
                    user_password = new char[params.at("upw").length()];
                    std::strcpy(user_password, params.at("upw").c_str());
                }

                std::optional<PDF_Document> pdf_doc = parse_pdf_file(params.at("path"));
                PDF_Section_Node doc_root;
                PDF_Document pdf_document;
                PDF_Section root_section;
                root_section.id = 0;
                if (pdf_doc) {
                    pdf_document = pdf_doc.value();
                    root_section.title = pdf_document.document_info.title;
                    root_section.paragraphs = pdf_document.prefix_content;
                    doc_root = construct_document_tree(pdf_document, root_section);
                }

                if (owner_password) {
                    delete[] owner_password;
                }

                if (user_password) {
                    delete[] user_password;
                }

                string_response_.emplace(
                            std::piecewise_construct,
                            std::make_tuple(),
                            std::make_tuple(alloc_));
                string_response_->result(boost::beast::http::status::ok);
                string_response_->keep_alive(false);
                string_response_->set(boost::beast::http::field::content_type, "application/json");
                string_response_->body() = pdf_doc ? format_pdf_document_tree(doc_root) : "{}";
                string_response_->prepare_payload();
                string_serializer_.emplace(*string_response_);

                boost::beast::http::async_write(
                            socket_,
                            *string_serializer_,
                            [this](boost::beast::error_code ec, std::size_t)
                            {
                                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                                string_serializer_.reset();
                                string_response_.reset();
                                accept();
                            });
            }
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            send_bad_response(
                boost::beast::http::status::bad_request,
                "Invalid request-method '" + req.method_string().to_string() + "'\r\n");
            break;
    }
}

void http_worker::send_bad_response(
    boost::beast::http::status status,
    std::string const& error) {
    string_response_.emplace(
        std::piecewise_construct,
        std::make_tuple(),
        std::make_tuple(alloc_));

    string_response_->result(status);
    string_response_->keep_alive(false);
    string_response_->set(boost::beast::http::field::server, "Beast");
    string_response_->set(boost::beast::http::field::content_type, "text/plain");
    string_response_->body() = error;
    string_response_->prepare_payload();

    string_serializer_.emplace(*string_response_);

    boost::beast::http::async_write(
        socket_,
        *string_serializer_,
    [this](boost::beast::error_code ec, std::size_t) {
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        string_serializer_.reset();
        string_response_.reset();
        accept();
    });
}

void http_worker::check_deadline() {
    // The deadline may have moved, so check it has really passed.
    if (request_deadline_.expiry() <= std::chrono::steady_clock::now()) {
        // Close socket to cancel any outstanding operation.
        boost::beast::error_code ec;
        socket_.close();

        // Sleep indefinitely until we're given a new deadline.
        request_deadline_.expires_at(
            std::chrono::steady_clock::time_point::max());
    }

    request_deadline_.async_wait(
    [this](boost::beast::error_code) {
        check_deadline();
    });
}
