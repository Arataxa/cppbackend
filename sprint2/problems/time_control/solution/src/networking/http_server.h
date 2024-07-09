#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>

#include "logger.h"

#include <iostream>

namespace http_server {
    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;

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
            // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
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
            // Захватываем умный указатель на текущий объект Session в лямбде,
            // чтобы продлить время жизни сессии до вызова лямбды.
            // Используется generic-лямбда функция, способная принять response произвольного типа
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
            // Обработчики асинхронных операций acceptor_ будут вызываться в своём strand
            , acceptor_(net::make_strand(ioc))
            , request_handler_(std::forward<Handler>(request_handler)) {
            // Открываем acceptor, используя протокол (IPv4 или IPv6), указанный в endpoint
            acceptor_.open(endpoint.protocol());

            // После закрытия TCP-соединения сокет некоторое время может считаться занятым,
            // чтобы компьютеры могли обменяться завершающими пакетами данных.
            // Однако это может помешать повторно открыть сокет в полузакрытом состоянии.
            // Флаг reuse_address разрешает открыть сокет, когда он "наполовину закрыт"
            acceptor_.set_option(net::socket_base::reuse_address(true));
            // Привязываем acceptor к адресу и порту endpoint
            acceptor_.bind(endpoint);
            // Переводим acceptor в состояние, в котором он способен принимать новые соединения
            // Благодаря этому новые подключения будут помещаться в очередь ожидающих соединений
            acceptor_.listen(net::socket_base::max_listen_connections);
        }

        void Run() {
            DoAccept();
        }
    private:
        void DoAccept() {
            acceptor_.async_accept(
                // Передаём последовательный исполнитель, в котором будут вызываться обработчики
                // асинхронных операций сокета
                net::make_strand(ioc_),
                // С помощью bind_front_handler создаём обработчик, привязанный к методу OnAccept
                // текущего объекта.
                // Так как Listener — шаблонный класс, нужно подсказать компилятору, что
                // shared_from_this — метод класса, а не свободная функция.
                // Для этого вызываем его, используя this
                // Этот вызов bind_front_handler аналогичен
                // namespace ph = std::placeholders;
                // std::bind(&Listener::OnAccept, this->shared_from_this(), ph::_1, ph::_2)
                beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
        }

        // Метод socket::async_accept создаст сокет и передаст его передан в OnAccept
        void OnAccept(sys::error_code ec, tcp::socket socket) {
            using namespace std::literals;

            if (ec) {
                return ReportError(ec, "accept"sv);
            }

            // Асинхронно обрабатываем сессию
            AsyncRunSession(std::move(socket));

            // Принимаем новое соединение
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