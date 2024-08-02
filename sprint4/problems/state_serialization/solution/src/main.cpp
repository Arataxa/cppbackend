#include <string_view>

#include "logger.h"
#include "json_loader.h"
#include "front_controller.h"
#include "ticker.h"
#include "loot_type_info.h"
#include "serialization.h"

#include <boost/program_options.hpp>
#include <boost/json/src.hpp>
#include <boost/asio.hpp>

#include <boost/archive/text_iarchive.hpp>

#include <fstream>
#include <optional>
#include <thread>

using namespace std::literals;


namespace net = boost::asio;

struct Args {
    std::string config_file;
    std::string state_file;
    std::string www_root;
    std::optional<int> tick_period;
    std::optional<int> save_state_period;
    bool randomize_spawn_points;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{ "Allowed options" };
    Args args;

    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<int>()->value_name("milliseconds"), "set tick period")
        ("config-file,c", po::value<std::string>()->value_name("file"), "set config file path")
        ("www-root,w", po::value<std::string>()->value_name("dir"), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points)->default_value(false), "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return std::nullopt;
    }

    if (vm.count("config-file")) {
        args.config_file = vm["config-file"].as<std::string>();
    }
    else {
        std::cerr << "Config file not specified.\n";
        std::cout << desc << "\n";
        return std::nullopt;
    }

    if (vm.count("tick-period")) {
        args.tick_period = vm["tick-period"].as<int>();
    }

    if (vm.count("www-root")) {
        args.www_root = vm["www-root"].as<std::string>();
    }
    else {
        std::cerr << "www-root not specified.\n";
        std::cout << desc << "\n";
        return std::nullopt;
    }

    if (vm.count("randomize-spawn-points")) {
        args.randomize_spawn_points = true;
    }

    if (vm.count("state-file")) {
        args.state_file = vm["state-file"].as<std::string>();
    }

    if (vm.count("save-state-period")) {
        args.save_state_period = vm["save-state-period"].as<int>();
    }

    return args;
}

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

void LoadState(application::game::Game& game, const std::string& state_file) {
    try {
        std::ifstream ifs(state_file);

        if (!ifs) {
            return;
        }

        boost::archive::text_iarchive ia(ifs);
        application::serialization::GameSerialization game_ser;
        ia >> game_ser;

        game_ser.ToGame(game);

        ifs.close();
    }
    catch (const std::exception& e) {
        boost::json::value data;
        data = { {"code", "EXIT_FAILURE"}, {"exception", e.what()}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "server exited";
        std::exit(EXIT_FAILURE);
    }
    catch (...) {
        boost::json::value data;
        data = { {"code", "0"} };
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "server exited";
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, const char* argv[]) {
    try {
        auto args = ParseCommandLine(argc, argv);

        if (!args) {
            return EXIT_FAILURE;
        }

        try {
            std::cout << std::unitbuf;
            InitializeLogger();

            // 1. Загружаем карту из файла и построить модель игры

            json_loader::GameLoader game_loader(args->randomize_spawn_points);

            auto game = game_loader.Load(args->config_file);

            if (!args->state_file.empty()) {
                LoadState(game, args->state_file);
            }

            application::Application application(std::move(game), std::move(game_loader.GetLootTypeInfo()), args->state_file);

            // 2. Инициализируем io_context
            const unsigned num_threads = std::thread::hardware_concurrency();
            net::io_context ioc(num_threads);

            // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait([&ioc](const boost::system::error_code& ec, int /*signo*/) {
                boost::json::value data;
                if (!ec) {
                    data = { {"code", "0"} };
                }
                else {
                    data = { {"code", "EXIT_FAILURE"}, {"exception", ec.message()} };
                }
                BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "server exited";
                ioc.stop();
                });

            auto api_strand = net::make_strand(ioc);

            // Настраиваем вызов метода Application::Tick каждые tick_period миллисекунд внутри strand
            auto last_save_time = std::chrono::steady_clock::now();
            const int save_state_period = args->save_state_period.value();

            if (args->tick_period) {
                auto ticker = std::make_shared<Ticker>(
                    api_strand,
                    std::chrono::milliseconds(args->tick_period.value()),
                    [&application, &last_save_time, save_state_period](std::chrono::milliseconds delta) {
                        application.ProcessTime(delta.count() / 1000.0);

                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_save_time).count();
                        if (elapsed >= save_state_period) {
                            application.SaveGame();
                            last_save_time = now;
                        }
                    }
                );
                ticker->Start();
            }

            // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
            http_handler::FrontController handler{ application, args->www_root, api_strand, !args->tick_period.has_value() };

            // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
            std::string interface_address = "0.0.0.0";
            const auto address = net::ip::make_address(interface_address);
            const unsigned short port = 8080;
            http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& send) {
                handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
                });

            boost::json::value data{
                {"port", port},
                {"address", interface_address}
            };
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "server started";

            // 6. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc] {
                ioc.run();
                });

            application.SaveGame();
        }
        catch (const std::exception& ex) {
            boost::json::value data{
                {"code", "EXIT_FAILURE"},
                {"exception", ex.what()}
            };
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data) << "server exited";

            std::cerr << ex.what() << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error parsing command line arguments: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}