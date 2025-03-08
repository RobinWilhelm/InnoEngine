#pragma once
#include "SimpleAverageCalc.h"

#include <string>
#include <array>

namespace InnoEngine
{
    enum class ProfilePoint
    {
        TotalFrame = 0,
        MainThreadTotal,
        UpdateThreadTotal,

        Update,
        Render,

        // keep as last
        Count
    };

    constexpr std::string GetProfileElementString( ProfilePoint ppoint )
    {
        switch ( ppoint ) {
        case ProfilePoint::TotalFrame:
            return "Total";
        case ProfilePoint::MainThreadTotal:
            return "MainThreadTotal";
        case ProfilePoint::UpdateThreadTotal:
            return "UpdateThreadTotal";
        case ProfilePoint::Update:
            return "Update";
        case ProfilePoint::Render:
            return "Render";
        case ProfilePoint::Count:
            return "Invalid Profileelement";
        }
        return "Unknown";
    };

    class Profiler
    {
        struct alignas( std::hardware_destructive_interference_size ) Timing
        {
            uint64_t              Current;
            uint64_t              TotalFrame;
            AverageCalc<uint64_t> AverageCalc;
        };

        Profiler() = default;

    public:
        [[nodiscard]]
        static auto create() -> std::optional<Own<Profiler>>;

        void update();

        void start( ProfilePoint ppoint );
        void stop( ProfilePoint ppoint );

        float get_average( ProfilePoint ppoint );

    private:
        std::array<Timing, static_cast<size_t>( ProfilePoint::Count )> m_timings;
    };
}    // namespace InnoEngine
