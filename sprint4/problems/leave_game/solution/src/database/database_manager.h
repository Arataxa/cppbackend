#pragma once
#include "database_thread_pool.h"
#include <iostream>

namespace database {
    struct Record {
        std::string name;
        int score;
        double play_time;
    };

    class DatabaseManager {
    public:
        DatabaseManager(const std::string& connection_info, std::size_t num_threads);

        void SaveScore(const std::string& name, int score, double play_time);

        std::vector<Record> GetRecords(int start, int item_limit);

        DatabaseManager(DatabaseManager&& other) noexcept;

        DatabaseManager& operator=(DatabaseManager&& other) noexcept;

    private:
        DatabaseConnectionPool db_pool_;

        void InitializeDataBase();
    };
}