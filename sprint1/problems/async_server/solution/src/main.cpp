#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
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

StringResponse HandleRequest(StringRequest&& request) {
    StringResponse response;

    switch (request.method()) {
    case http::verb::get:
        FillGetResponse(response, request);
        break;
    case http::verb::head:
        FillHeadResponse(response, request);
        break;
    default:
        FillWrongResponse(response, request);
    }

    return response;
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, {address, port}, [](auto&& req, auto&& sender) {
        // sender(HandleRequest(std::forward<decltype(req)>(req)));
    });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
}
