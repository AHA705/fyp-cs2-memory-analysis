#include "logger.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

std::shared_ptr<spdlog::logger> logger;

void setupLogger()
{
    try
    {
        std::vector<spdlog::sink_ptr> sinks;
        auto now = std::time(nullptr);
        std::tm tm_now;
        localtime_s(&tm_now, &now);
        char timebuf[32];
        std::strftime(timebuf, sizeof(timebuf), "%Y%m%d-%H%M%S", &tm_now);
        std::string logFileName = "logs/track-enemy_" + std::string(timebuf) + ".txt";
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFileName, true);
        console_sink->set_level(spdlog::level::info);
        file_sink->set_level(spdlog::level::debug);
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);
        logger = std::make_shared<spdlog::logger>("track-enemy", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);
        logger->info("Logging initialized.");
        logger->info(
            "Console log level: {}. Log file level {}",
            spdlog::level::to_string_view(console_sink->level()),
            spdlog::level::to_string_view(file_sink->level()));
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}
