#pragma once
#include "logger.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <iostream>
#include <chrono>

namespace http_server {
    namespace net = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;

    using tcp = net::ip::tcp;

    using namespace std::literals;

    void ReportError(beast::error_code ec, std::string_view what);

    class SessionBase {
    public:
        using HttpRequest = http::request<http::string_body>;

        SessionBase(const SessionBase&) = delete;
        SessionBase& operator=(const SessionBase&) = delete;

        virtual ~SessionBase() = default;

        void Run();

        template <typename Body, typename Fields>
        void Write(http::response<Body, Fields>&& response) {
            auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

            auto end_time = std::chrono::steady_clock::now();

            auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_).count();

            std::string content_type;
            if (safe_response->base().find(boost::beast::http::field::content_type) != safe_response->base().end()) {
                content_type = safe_response->base().at(boost::beast::http::field::content_type).to_string();
            }
            else {
                content_type = "null";
            }

            boost::json::value data{ 
                {"response_time", response_time}, 
                {"code", safe_response->result_int()}, 
                {"content_type", content_type} };
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "response sent";

            auto self = GetSharedThis();
            http::async_write(stream_, *safe_response,
                [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                    self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                });
        }
    protected:
        explicit SessionBase(tcp::socket&& socket);
    private:
        void Read();

        void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);

        void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);

        void Close();

        virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

        std::string GetClientIp() const;

        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        HttpRequest request_;
        virtual void HandleRequest(HttpRequest&& request) = 0;

        std::chrono::steady_clock::time_point start_time_;
    };

    template <typename RequestHandler>
    class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
    public:
        template <typename Handler>
        Session(tcp::socket&& socket, Handler&& request_handler)
            : SessionBase(std::move(socket))
            , request_handler_(std::forward<Handler>(request_handler)) {
        }

    private:
        std::shared_ptr<SessionBase> GetSharedThis() override {
            return this->shared_from_this();
        }

        void HandleRequest(HttpRequest&& request) override {
            request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
                self->Write(std::move(response));
                });
        }

        RequestHandler request_handler_;
    };

    template <typename RequestHandler>
    class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
    public:
        template <typename Handler>
        Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
            : ioc_(ioc)
            , acceptor_(net::make_strand(ioc))
            , request_handler_(std::forward<Handler>(request_handler)) {
            acceptor_.open(endpoint.protocol());

            acceptor_.set_option(net::socket_base::reuse_address(true));

            acceptor_.bind(endpoint);

            acceptor_.listen(net::socket_base::max_listen_connections);
        }

        void Run() {
            DoAccept();
        }
    private:
        void DoAccept() {
            acceptor_.async_accept(
                net::make_strand(ioc_),
                beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
        }

        void OnAccept(sys::error_code ec, tcp::socket socket) {
            using namespace std::literals;

            if (ec) {
                return ReportError(ec, "accept"sv);
            }

            AsyncRunSession(std::move(socket));

            DoAccept();
        }

        void AsyncRunSession(tcp::socket&& socket) {
            std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
        }

        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        RequestHandler request_handler_;
    };

    template <typename RequestHandler>
    void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
        using MyListener = Listener<std::decay_t<RequestHandler>>;

        std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
    }

}  // namespace http_server