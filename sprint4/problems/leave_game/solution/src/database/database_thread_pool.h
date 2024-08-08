#pragma once
#include <boost/asio.hpp>
#include <pqxx/pqxx>

#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace database {
    class DatabaseConnectionPool {
    public:
        class ConnectionWrapper {
        public:
            ConnectionWrapper(std::shared_ptr<pqxx::connection>&& connection, DatabaseConnectionPool& pool);

            ~ConnectionWrapper();

            pqxx::connection& operator*();
            pqxx::connection* operator->();
        private:
            std::shared_ptr<pqxx::connection> connection_;
            DatabaseConnectionPool* pool_;
        };

        DatabaseConnectionPool(const std::string& connection_info, std::size_t pool_size);

        ConnectionWrapper GetConnection();

        void ReturnConnection(std::shared_ptr<pqxx::connection>&& connection);

        DatabaseConnectionPool(DatabaseConnectionPool&& other) noexcept;
        DatabaseConnectionPool& operator=(DatabaseConnectionPool&& other) noexcept;

    private:
        std::queue<std::shared_ptr<pqxx::connection>> pool_;
        std::mutex mutex_;
        std::condition_variable condition_;
    };

    class ThreadPool {
    public:
        explicit ThreadPool(std::size_t num_threads);

        ~ThreadPool();

        template <typename F>
        void Post(F&& f) {
            boost::asio::post(*io_context_, std::forward<F>(f));
        }

        ThreadPool(ThreadPool&& other) noexcept;
        ThreadPool& operator=(ThreadPool&& other) noexcept;

    private:
        std::shared_ptr<boost::asio::io_context> io_context_;
        std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;
        std::vector<std::thread> threads_;
    };
}