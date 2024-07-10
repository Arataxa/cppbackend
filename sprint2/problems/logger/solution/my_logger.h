#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <filesystem>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    std::string GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t_c), "%F %T");
        return oss.str();
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return oss.str();
    }

    void OpenLogFile() {
        if (log_file_.is_open()) {
            log_file_.close();
        }

        current_file_date_ = GetFileTimeStamp();

        std::string file_path = log_directory_ + "sample_log_" + current_file_date_ + ".log";
        log_file_.open(file_path,std::ios::app);

        if (!log_file_.is_open()) {
            throw std::runtime_error("Failed to open log file xd: " + file_path);
        }
    }

    Logger() {
        if (std::filesystem::exists("/var/log") && std::filesystem::is_directory("/var/log")) {
            log_directory_ = "/var/log/";
        }
        else {
            log_directory_ = "./logs/";
            if (!std::filesystem::exists(log_directory_)) {
                std::filesystem::create_directories(log_directory_);
            }
        }
    }
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (current_file_date_ != GetFileTimeStamp()) {
            OpenLogFile();
        }

        log_file_ << GetTimeStamp() << ": ";
        (log_file_ << ... << args) << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex);
        manual_ts_ = ts;
    }

private:
    std::mutex mutex;

    std::optional<std::chrono::system_clock::time_point> manual_ts_;

    std::string current_file_date_;
    std::string log_directory_ = "./var/log/";
    std::ofstream log_file_;
};
