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
        COUNT
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
        case ProfileElements::COUNT:
            return "Invalid Profileelement";
        }
        return "Unknown";
    };

    class Profiler
    {
        struct Timing
        {
            uint64_t Current;
            uint64_t TotalFrame;
        };

        Profiler() = default;

    public:
        [[nodiscard]]
        static auto create() -> std::optional<Owned<Profiler>>;

        void update();

        void start( ProfileElements element );
        void stop( ProfileElements element );

        float get_average( ProfileElements element );

    private:
        // TODO prevent false sharing
        std::array<Timing, static_cast<size_t>( ProfileElements::COUNT )>                m_timings;
        std::array<AverageCalc<uint64_t>, static_cast<size_t>( ProfileElements::COUNT )> m_timingsAvg;
    };
}    // namespace InnoEngine
