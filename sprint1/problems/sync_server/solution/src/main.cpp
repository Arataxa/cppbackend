#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;


using StringRequest = http::request<http::string_body>;

using StringResponse = http::response<http::string_body>;

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    // Считываем из socket запрос req, используя buffer для хранения данных.
    // В ec функция запишет код ошибки.
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }
    if (ec) {
        throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }
    return req;
}

void DumpRequest(const StringRequest& req) {
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    // Выводим заголовки запроса
    for (const auto& header : req) {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
}

void FillGetResponse(StringResponse& response, StringRequest& request) {
    std::string response_body = "Hello, " + std::string(request.target().substr(1));

    // Формируем ответ со статусом 200 и версией равной версии запроса
    response.result(http::status::ok);
    response.version(request.version());
    // Добавляем заголовок Content-Type: text/html
    response.set(http::field::content_type, "text/html"sv);
    response.body() = response_body;
    // Формируем заголовок Content-Length, сообщающий длину тела ответа
    response.content_length(response.body().size());
    // Формируем заголовок Connection в зависимости от значения заголовка в запросе
    response.keep_alive(request.keep_alive());
}

void FillHeadResponse(StringResponse& response, StringRequest& request) {
    FillGetResponse(response, request);

    response.body().clear();
}

void FillWrongResponse(StringResponse& response, StringRequest& request) {
    response.result(http::status::method_not_allowed);
    response.version(request.version());
    response.set(http::field::content_type, "text/html"sv);
    response.set(http::field::allow, "GET, HEAD");
    response.body() = "Invalid method";
    response.content_length(response.body().size());
}

void HandleConnection(tcp::socket& socket) {
    try {
        // Буфер для чтения данных в рамках текущей сессии.
        beast::flat_buffer buffer;

        // Продолжаем обработку запросов, пока клиент их отправляет
        while (auto request = ReadRequest(socket, buffer)) {
            DumpRequest(*request);
            
            StringResponse response;

            switch (request.value().method()) {
            case http::verb::get:
                FillGetResponse(response, request.value());
                break;
            case http::verb::head:
                FillHeadResponse(response, request.value());
                break;
            default:
                FillWrongResponse(response, request.value());
            }
            

            // Отправляем ответ сервера клиенту
            http::write(socket, response);

            // Прекращаем обработку запросов, если семантика ответа требует это
            if (response.need_eof()) {
                break;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code ec;
    // Запрещаем дальнейшую отправку данных через сокет
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main() {
    net::io_context ioc;

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    tcp::acceptor acceptor(ioc, { address, port });

    while (true) {
        tcp::socket socket(ioc);
        std::cout << "Server has started..."s << std::endl;
        acceptor.accept(socket);
        HandleConnection(socket);
    }
}
