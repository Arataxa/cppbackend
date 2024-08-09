#include "database_manager.h"

namespace database {
    DatabaseManager::DatabaseManager(const std::string& connection_info, std::size_t num_threads)
        : db_pool_(connection_info, num_threads) {
        InitializeDataBase();
    }

    void DatabaseManager::SaveScore(const std::string& name, int score, float play_time) {
        std::string query = "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3)";
        std::vector<std::string> params = { name, std::to_string(score), std::to_string(play_time) };
        ExecuteTransaction(query, params);
    }

    std::vector<Record> DatabaseManager::GetRecords(int start, int item_limit) {
        std::vector<Record> records;
        std::string query = "SELECT name, score, play_time FROM retired_players ORDER BY score DESC, play_time ASC, name ASC LIMIT $1 OFFSET $2";
        std::vector<std::string> params = { std::to_string(item_limit), std::to_string(start) };

        auto connection = db_pool_.GetConnection();
        try {
            pqxx::work txn(*connection);
            pqxx::result res = txn.exec_params(query, params);
            for (const auto& row : res) {
                records.push_back(Record{ row["name"].as<std::string>(), row["score"].as<int>(), row["play_time"].as<float>() });
            }
        }
        catch (const std::exception& e) {
            // Логирование ошибки или обработка
        }

        return records;
    }

    DatabaseManager::DatabaseManager(DatabaseManager&& other) noexcept : db_pool_(std::move(other.db_pool_)) {}

    DatabaseManager& DatabaseManager::operator=(DatabaseManager&& other) noexcept {
        if (this != &other) {
            db_pool_ = std::move(other.db_pool_);
        }

        return *this;
    }

    void DatabaseManager::ExecuteTransaction(const std::string& query, const std::vector<std::string>& params) {
        auto conn = db_pool_.GetConnection();
        try {
            pqxx::work txn(*conn);

            if (params.empty()) {
                txn.exec(query);
            }
            else {
                txn.exec_params(query, params);
            }

            txn.commit();
        }
        catch (const std::exception& e) {
            // Логирование ошибки или обработка
            // std::cerr << "Transaction failed: " << e.what() << std::endl;
        }
    }

    void DatabaseManager::InitializeDataBase() {
        std::string query = R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id SERIAL PRIMARY KEY,
            name VARCHAR(255) NOT NULL,
            score INT NOT NULL,
            play_time FLOAT NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_score_play_time ON retired_players (score DESC, play_time ASC, name ASC);
    )";
        ExecuteTransaction(query, {});
    }
}
