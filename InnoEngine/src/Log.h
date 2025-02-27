#pragma once

#include "SDL3/SDL_log.h"
#include "IE_Assert.h"

#include <vector>
#include <string>
#include <format>
#include <mutex>
#include <chrono>
#include <iostream>

namespace InnoEngine
{
    enum class AnsiForegroundColor : int
    {
        Black         = 30,
        Red           = 31,
        Green         = 32,
        Yellow        = 33,
        Blue          = 34,
        Magenta       = 35,
        Cyan          = 36,
        White         = 37,
        BrightBlack   = 90,
        BrightRed     = 91,
        BrightGreen   = 92,
        BrightYellow  = 93,
        BrightBlue    = 94,
        BrightMagenta = 95,
        BrightCyan    = 96,
        BrightWhite   = 97,
        SystemDefault = 39
    };

    class Logger
    {
    private:
        Logger()
        {
            m_logs.reserve( 1000 );
        }

    public:
        enum Category
        {
            Debug,
            Info,
            Warning,
            Error,
            Critical,
        };

        struct LogEntry
        {
            std::chrono::time_point<std::chrono::system_clock> Timestamp;
            Category                                           Category;
            std::string                                        Message;
        };

        static Logger& get_instance()
        {
            static Logger logger;
            return logger;
        }

        template <Category category, typename... Args>
        void add( std::string_view fmt, Args... args )
        {
            m_ulock.lock();
            LogEntry& log = m_logs.emplace_back();
            log.Category  = category;
            log.Timestamp = std::chrono::system_clock::now();
            log.Message   = std::vformat(fmt, std::make_format_args(args...));
            m_ulock.unlock();

            std::cout << "\033[" << static_cast<int>( get_category_color( category ) );
            std::cout << std::format( "{:%H:%M} {} {}", log.Timestamp, get_category_string( category ), log.Message );
            std::cout << "\033[" << static_cast<int>( AnsiForegroundColor::SystemDefault ) << "\n";
        };

        constexpr std::string get_category_string( Category cat )
        {
            switch ( cat ) {
            case Debug:
                return "Debug";
            case Info:
                return "Info";
            case Warning:
                return "Warning";
            case Error:
                return "Error";
            case Critical:
                return "Critical";
            default:
                return "Unknown category";
            }
        }

        constexpr AnsiForegroundColor get_category_color( Category cat )
        {
            switch ( cat ) {
            case Debug:
                return AnsiForegroundColor::White;
            case Info:
                return AnsiForegroundColor::BrightBlack;
            case Warning:
                return AnsiForegroundColor::Yellow;
            case Error:
                return AnsiForegroundColor::Red;
            case Critical:
                return AnsiForegroundColor::BrightRed;
            default:
                return AnsiForegroundColor::SystemDefault;
            }
        }

    private:
        std::unique_lock<std::mutex> m_ulock;
        std::mutex                   m_mutex;
        std::vector<LogEntry>        m_logs;
    };
}    // namespace InnoEngine

#define IE_LOG_DEBUG( ... )    Logger::get_instance().add<Logger::Category::Debug>( __VA_ARGS__ )
#define IE_LOG_INFO( ... )     Logger::get_instance().add<Logger::Category::Info>( __VA_ARGS__ )
#define IE_LOG_WARNING( ... )  Logger::get_instance().add<Logger::Category::Warning>( __VA_ARGS__ )
#define IE_LOG_ERROR( ... )    Logger::get_instance().add<Logger::Category::Error>( __VA_ARGS__ )
#define IE_LOG_CRITICAL( ... ) Logger::get_instance().add<Logger::Category::Critical>( __VA_ARGS__ )
