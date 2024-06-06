#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <map>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{ io }, strand_{ net::make_strand(io) }, gas_cooker_{ std::make_shared<GasCooker>(io, 8) } { // 8 горелок
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        auto id = next_id_++;
        auto sausage = store_.GetSausage();
        auto bread = store_.GetBread();
        // Начало приготовления сосиски
        sausage->StartFry(*gas_cooker_, [this, sausage, bread, id, handler]() {
            auto sausage_timer = std::make_shared<net::steady_timer>(io_, HotDog::MIN_SAUSAGE_COOK_DURATION);
            sausage_timer->async_wait(net::bind_executor(strand_, [this, sausage, bread, id, handler, sausage_timer](const boost::system::error_code& ec) {
                sausage->StopFry();
                HandleIngredientDone(id, handler, sausage, bread, "sausage");
                }));
            });

        // Начало приготовления булки
        bread->StartBake(*gas_cooker_, [this, sausage, bread, id, handler]() {
            auto bread_timer = std::make_shared<net::steady_timer>(io_, HotDog::MIN_BREAD_COOK_DURATION);
            bread_timer->async_wait(net::bind_executor(strand_, [this, sausage, bread, id, handler, bread_timer](const boost::system::error_code& ec) {
                bread->StopBaking();
                HandleIngredientDone(id, handler, sausage, bread, "bread");
                }));
            });
    }

private:
    void HandleIngredientDone(int id, HotDogHandler handler, std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread, const std::string& ingredient) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& state = order_state_[id];

            if (ingredient == "sausage") {
                state.sausage_done = true;
                state.sausage = sausage;
            }
            else if (ingredient == "bread") {
                state.bread_done = true;
                state.bread = bread;
            }

            if (!state.sausage_done || !state.bread_done) {
                return;
            }
        }

        AssembleHotDog(id, sausage, bread, handler);
    }

    void AssembleHotDog(int id, std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread, HotDogHandler handler) {
        try {
            // Проверка времени приготовления ингредиентов
            if (sausage->GetCookDuration() < HotDog::MIN_SAUSAGE_COOK_DURATION
                || sausage->GetCookDuration() > HotDog::MAX_SAUSAGE_COOK_DURATION) {
                throw std::invalid_argument("Invalid sausage cook duration");
            }

            if (bread->GetBakingDuration() < HotDog::MIN_BREAD_COOK_DURATION
                || bread->GetBakingDuration() > HotDog::MAX_BREAD_COOK_DURATION) {
                throw std::invalid_argument("Invalid bread cook duration");
            }

            auto hot_dog = HotDog{ id, sausage, bread };
            handler(Result<HotDog>{std::move(hot_dog)});
        }
        catch (const std::invalid_argument& e) {
            handler(Result<HotDog>{std::current_exception()});
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            order_state_.erase(id);
        }
    }

    struct OrderState {
        bool sausage_done = false;
        bool bread_done = false;
        std::shared_ptr<Sausage> sausage;
        std::shared_ptr<Bread> bread;
    };

    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_;
    std::mutex mutex_;
    std::map<int, OrderState> order_state_;
    int next_id_ = 0;
};
