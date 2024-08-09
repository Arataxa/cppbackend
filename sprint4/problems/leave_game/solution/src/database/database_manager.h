#pragma once
#include "database_thread_pool.h"

namespace database {
    struct Record {
        std::string name;
        int score;
        float play_time;
    };

    class DatabaseManager {
    public:
        DatabaseManager(const std::string& connection_info, std::size_t num_threads);

        void SaveScore(const std::string& name, int score, float play_time);

        std::vector<Record> GetRecords(int start);

        DatabaseManager(DatabaseManager&& other) noexcept;

        DatabaseManager& operator=(DatabaseManager&& other) noexcept;

    private:
        DatabaseConnectionPool db_pool_;
        const std::string item_limit = "100";

        void ExecuteTransaction(const std::string& query, const std::vector<std::string>& params);

        void InitializeDataBase();
    };
}