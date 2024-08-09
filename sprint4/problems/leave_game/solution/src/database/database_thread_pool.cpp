#include "database_thread_pool.h"

#include <stdexcept>

namespace database {
    DatabaseConnectionPool::ConnectionWrapper::ConnectionWrapper(std::shared_ptr<pqxx::connection>&& connection, DatabaseConnectionPool& pool)
        : connection_(std::move(connection)), pool_(&pool) {
    }

    DatabaseConnectionPool::ConnectionWrapper::~ConnectionWrapper() {
        if (connection_) {
            pool_->ReturnConnection(std::move(connection_));
        }
    }

    pqxx::connection& DatabaseConnectionPool::ConnectionWrapper::operator*() { return *connection_; }
    pqxx::connection* DatabaseConnectionPool::ConnectionWrapper::operator->() { return connection_.get(); }

    DatabaseConnectionPool::DatabaseConnectionPool(const std::string& connection_info, std::size_t pool_size) {
        try {
            pool_.emplace(std::make_shared<pqxx::connection>(connection_info));
        }
        catch (const pqxx::broken_connection& e) {
            throw std::runtime_error("Failed to connect to the database : " + std::string(e.what()));  
        }
    }

    DatabaseConnectionPool::ConnectionWrapper DatabaseConnectionPool::GetConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() { return !pool_.empty(); });

        auto connection = std::move(pool_.front());
        pool_.pop();
        return { std::move(connection), *this };
    }

    void DatabaseConnectionPool::ReturnConnection(std::shared_ptr<pqxx::connection>&& connection) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(std::move(connection));
        condition_.notify_one();
    }

    DatabaseConnectionPool::DatabaseConnectionPool(DatabaseConnectionPool&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        pool_ = std::move(other.pool_);
    }

    DatabaseConnectionPool& DatabaseConnectionPool::operator=(DatabaseConnectionPool&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock(other.mutex_);
            pool_ = std::move(other.pool_);
        }
        return *this;
    }


}