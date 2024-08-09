#include "database_manager.h"

#include <iostream>

namespace database {
    DatabaseManager::DatabaseManager(const std::string& connection_info, std::size_t num_threads)
        : db_pool_(connection_info, num_threads) {
        InitializeDataBase();
    }

    void DatabaseManager::SaveScore(const std::string& name, int score, float play_time) {
        std::string query = "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3);";

        try {
            auto connection = db_pool_.GetConnection();
            pqxx::work txn(*connection, "serializable");

            txn.exec_params(query, name, score, play_time);

            txn.commit();
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to save score: " << e.what() << std::endl;
        }
    }

    std::vector<Record> DatabaseManager::GetRecords(int start, int item_limit) {
        std::vector<Record> records;

        auto connection = db_pool_.GetConnection();
 
        try {
            pqxx::work txn(*connection, "serializable");

            std::string select_query = "SELECT name, score, play_time FROM retired_players "
                "ORDER BY score DESC, play_time ASC, name ASC "
                "LIMIT $1 OFFSET $2;";
            pqxx::result res = txn.exec_params(select_query, std::to_string(item_limit), std::to_string(start));

            for (const auto& row : res) {
                try {
                    records.push_back(Record{
                        row["name"].as<std::string>(),
                        row["score"].as<int>(),
                        row["play_time"].as<float>()
                        });
                }
                catch (const std::exception& e) {
                    std::cout << "Error processing record: " << e.what() << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cout << "Transaction failed: " << e.what() << std::endl;
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

        auto conn = db_pool_.GetConnection();
        pqxx::work txn(*conn);
        txn.exec(query);
        txn.commit();
    }
}
