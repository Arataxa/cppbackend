#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <syncstream>
#include <thread>

namespace net = boost::asio;
namespace sys = boost::system;
namespace ph = std::placeholders;
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;

namespace {

class Hamburger {
public:
    [[nodiscard]] bool IsCutletRoasted() const {
        return cutlet_roasted_;
    }
    void SetCutletRoasted() {
        if (IsCutletRoasted()) {  // Котлету можно жарить только один раз
            throw std::logic_error("Cutlet has been roasted already"s);
        }
        cutlet_roasted_ = true;
    }

    [[nodiscard]] bool HasOnion() const {
        return has_onion_;
    }
    // Добавляем лук
    void AddOnion() {
        if (IsPacked()) {  // Если гамбургер упакован, класть лук в него нельзя
            throw std::logic_error("Hamburger has been packed already"s);
        }
        AssureCutletRoasted();  // Лук разрешается класть лишь после прожаривания котлеты
        has_onion_ = true;
    }

    [[nodiscard]] bool IsPacked() const {
        return is_packed_;
    }
    void Pack() {
        AssureCutletRoasted();  // Нельзя упаковывать гамбургер, если котлета не прожарена
        is_packed_ = true;
    }

private:
    // Убеждаемся, что котлета прожарена
    void AssureCutletRoasted() const {
        if (!cutlet_roasted_) {
            throw std::logic_error("Bread has not been roasted yet"s);
        }
    }

    bool cutlet_roasted_ = false;  // Обжарена ли котлета?
    bool has_onion_ = false;       // Есть ли лук?
    bool is_packed_ = false;       // Упакован ли гамбургер?
};

std::ostream& operator<<(std::ostream& os, const Hamburger& h) {
    return os << "Hamburger: "sv << (h.IsCutletRoasted() ? "roasted cutlet"sv : " raw cutlet"sv)
              << (h.HasOnion() ? ", onion"sv : ""sv)
              << (h.IsPacked() ? ", packed"sv : ", not packed"sv);
}

class Logger {
public:
    explicit Logger(std::string id)
        : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

class ThreadChecker {
public:
    explicit ThreadChecker(std::atomic_int& counter)
        : counter_{counter} {
    }

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

// Функция, которая будет вызвана по окончании обработки заказа
using OrderHandler = std::function<void(sys::error_code ec, int id, Hamburger* hamburger)>;

class Restaurant : public std::enable_shared_from_this<Restaurant> {
public:
    explicit Restaurant(net::io_context& io)
        : io_(io), strand_(net::make_strand(io)) {
    }

    int MakeHamburger(bool with_onion, OrderHandler handler) {
        const int order_id = ++next_order_id_;
        auto hamburger = std::make_shared<Hamburger>();

        // Шаг 1: Обжарить котлету и (если нужно) добавить лук
        auto timer1 = std::make_shared<net::steady_timer>(io_, 1s);
        timer1->async_wait(net::bind_executor(strand_, [this, hamburger, with_onion, order_id, handler, timer1](sys::error_code ec) {
            if (ec) {
                handler(ec, order_id, nullptr);
                return;
            }
            hamburger->SetCutletRoasted();
            if (with_onion) {
                // Шаг 2: Добавить лук
                auto timer2 = std::make_shared<net::steady_timer>(io_, 1s);
                timer2->async_wait(net::bind_executor(strand_, [this, hamburger, order_id, handler, timer2](sys::error_code ec) {
                    if (ec) {
                        handler(ec, order_id, nullptr);
                        return;
                    }
                    hamburger->AddOnion();
                    // Шаг 3: Упаковать гамбургер
                    PackHamburger(hamburger, order_id, handler);
                    }));
            }
            else {
                // Шаг 3: Упаковать гамбургер
                PackHamburger(hamburger, order_id, handler);
            }
            }));

        return order_id;
    }

private:
    void PackHamburger(std::shared_ptr<Hamburger> hamburger, int order_id, OrderHandler handler) {
        auto timer = std::make_shared<net::steady_timer>(io_, 1s);
        timer->async_wait(net::bind_executor(strand_, [hamburger, order_id, handler, timer](sys::error_code ec) {
            if (ec) {
                handler(ec, order_id, nullptr);
                return;
            }
            hamburger->Pack();
            handler({}, order_id, hamburger.get());
            }));
    }

    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_;
    int next_order_id_ = 0;
};

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
    const unsigned num_workers = 4;
    net::io_context io(num_workers);

    Restaurant restaurant{io};

    Logger logger{"main"s};

    struct OrderResult {
        sys::error_code ec;
        Hamburger hamburger;
    };

    std::unordered_map<int, OrderResult> orders;

    // Обработчик заказа может быть вызван в любом из потоков, вызывающих io.run().
    // Чтобы избежать состояния гонки при обращении к orders, выполняем обращения к orders через
    // strand, используя функцию dispatch.
    auto handle_result
        = [strand = net::make_strand(io), &orders](sys::error_code ec, int id, Hamburger* h) {
              net::dispatch(strand, [&orders, id, res = OrderResult{ec, ec ? Hamburger{} : *h}] {
                  orders.emplace(id, res);
              });
          };

    const int num_orders = 16;
    for (int i = 0; i < num_orders; ++i) {
        restaurant.MakeHamburger(i % 2 == 0, handle_result);
    }

    assert(orders.empty());
    RunWorkers(num_workers, [&io] {
        io.run();
    });
    std::cout << orders.size() << "  " << num_orders << std::endl;
    assert(orders.size() == num_orders);

    for (const auto& [id, order] : orders) {
        assert(!order.ec);
        assert(order.hamburger.IsCutletRoasted());
        assert(order.hamburger.IsPacked());
        assert(order.hamburger.HasOnion() == (id % 2 != 0));
    }
}
