#include "front_controller.h"
#include "json_loader.h"
#include "logger.h"
#include "loot_type_info.h"
#include "serialization.h"
#include "ticker.h"

#include <boost/asio.hpp>
#include <boost/json/src.hpp>
#include <boost/program_options.hpp>
#include <pqxx\pqxx>

#include <fstream>
#include <optional>
#include <string_view>
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
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points)->default_value(false), "spawn dogs at random positions")
        ("state-file", po::value<std::string>()->value_name("file"), "set state file path")
        ("save-state-period", po::value<int>()->value_name("milliseconds"), "set save state period");

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

    template <typename Fn>
    void RunWorkers(unsigned numWorkers, const Fn& fn) {
        numWorkers = std::max(1u, numWorkers);
        std::vector<std::jthread> workers;
        workers.reserve(numWorkers - 1);
        while (--numWorkers) {
            workers.emplace_back(fn);
        }
        fn();
    }

}

void InitializeSignalHandler(net::io_context& ioc) {
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
}

void StartTicker(application::Application& application ,int tick_period, boost::asio::strand<net::io_context::executor_type>& api_strand) {
    auto ticker = std::make_shared<Ticker>(
        api_strand,
        std::chrono::milliseconds(tick_period),
        [&application](std::chrono::milliseconds delta) {
            application.ProcessTime(delta.count() / 1000.0);
        }
    );
    ticker->Start();
}

void TryLoadState(application::Application& application) {
    try {
        application.LoadGame();
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

constexpr const char DB_URL_ENV_NAME[]{ "BOOKYPEDIA_DB_URL" };

std::string GetDbUrlFromEnv() {
    std::string db_url;
    if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
        db_url = url;
    }
    else {
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    }
    return db_url;
}

application::Application InitializeApplication(Args& args) {
    json_loader::GameLoader game_loader(args.randomize_spawn_points);

    auto game = game_loader.Load(args.config_file);

    int save_period = -1;

    if (args.save_state_period.has_value()) {
        save_period = args.save_state_period.value();
    }

    database::DatabaseManager db_manager(GetDbUrlFromEnv(), std::thread::hardware_concurrency());

    application::Application application(std::move(game), std::move(db_manager), std::move(game_loader.GetLootTypeInfo()), args.state_file, save_period);

    TryLoadState(application);

    return application;
}

void StartHttpHandler(net::io_context& ioc, http_handler::FrontController& handler) {
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
}

void StartServer(Args& args) {
    application::Application application = InitializeApplication(args);

    const unsigned num_threads = std::thread::hardware_concurrency();
    net::io_context ioc(num_threads);

    auto api_strand = net::make_strand(ioc);

    InitializeSignalHandler(ioc);
    
    if (args.tick_period) {
        StartTicker(application, args.tick_period.value(), api_strand);
    }
    
    http_handler::FrontController handler{ application, args.www_root, api_strand, !args.tick_period.has_value() };
    StartHttpHandler(ioc, handler);

    RunWorkers(std::max(1u, num_threads), [&ioc] {
    ioc.run();
        });

    application.SaveGame();
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

            StartServer(*args);
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