#include "Log.h"

void InnoEngine::Logger::printLog(const LogEntry& log)
{
#ifndef _DEBUG
    if(log.Category == Category::Debug)
        return;
#endif

    std::cout << "\033[" << static_cast<int>(get_category_color(log.Category)) << ";1m";
    std::cout << std::format("{:%H:%M:%S} {}", std::chrono::time_point_cast<std::chrono::seconds>(log.Timestamp), log.Message);
    std::cout << "\033[" << static_cast<int>(AnsiForegroundColor::SystemDefault) << ";1m" << "\n";   
}
