#include <string_view>

#include "logger.h"
#include "json_loader.h"
#include "front_controller.h"

#include <thread>

#include <boost/json/src.hpp>
#include <boost/asio.hpp>

using namespace std::literals;

namespace net = boost::asio;

void Formater(boost::log::record_view const& rec, boost::log::formatting_ostream& strm) {
    auto timestap = *rec[timestamp];
        auto message = rec[boost::log::expressions::smessage];
        auto data = rec[additional_data];
    
        boost::json::object json_obj;
        json_obj["timestamp"] = boost::posix_time::to_iso_extended_string(timestap);
        if (data) {
            json_obj["data"] = data.get();
        }
        json_obj["message"] = message ? message.get() : "";
    
        strm << boost::json::serialize(json_obj);
}

void InitializeLogger() {
    boost::log::add_common_attributes();

    boost::log::add_console_log(
        std::cout,
        boost::log::keywords::format = &Formater
    );
}

namespace {

    // Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned numWorkers, const Fn& fn) {
        numWorkers = std::max(1u, numWorkers);
        std::vector<std::jthread> workers;
        workers.reserve(numWorkers - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--numWorkers) {
            workers.emplace_back(fn);
        }
        fn();
    }

}

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        std::cout << std::unitbuf;
        InitializeLogger();

        // 1. Загружаем карту из файла и построить модель игры
        application::Application application(json_loader::LoadGame(argv[1]));

        //// 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        //// 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int /*signo*/) {
            boost::json::value data;
            if (!ec) {
                data = { {"code", "0"} };
            }
            else {
                data = { {"code", "EXIT_FAILURE"}, {"exception", ec.message()}};
            }
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "server exited";

            ioc.stop();
            });

        //// 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::FrontController handler{ application, argv[2], ioc };

        //// 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        std::string interface_address = "0.0.0.0";

        const auto address = net::ip::make_address(interface_address);
        const unsigned short port = 8080;
        http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

        boost::json::value data{ 
            {"port", port}, 
            {"address", interface_address}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "server started";

        //// 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
            });
    }
    catch (const std::exception& ex) {
        boost::json::value data{ 
            {"code", "EXIT_FAILURE"}, 
            {"exception", ex.what()}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "server exited";

        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}