#pragma once
#include "SimpleAverageCalc.h"

#include <string>
#include <array>

namespace InnoEngine
{
    enum class ProfileElements
    {
        TotalFrame = 0,
        MainThreadTotal,
        UpdateThreadTotal,

        Update,
        Render,

        // keep as last
        Count
    };

    constexpr std::string GetProfileElementString( ProfileElements elem )
    {
        switch ( elem ) {
        case ProfileElements::TotalFrame:
            return "Total";
        case ProfileElements::MainThreadTotal:
            return "MainThreadTotal";
        case ProfileElements::UpdateThreadTotal:
            return "UpdateThreadTotal";
        case ProfileElements::Update:
            return "Update";
        case ProfileElements::Render:
            return "Render";
        case ProfileElements::Count:
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

        void start( ProfileElements element );
        void stop( ProfileElements element );

        float get_average( ProfileElements element );

    private:
        std::array<Timing, static_cast<size_t>( ProfileElements::Count )> m_timings;
    };
}    // namespace InnoEngine
