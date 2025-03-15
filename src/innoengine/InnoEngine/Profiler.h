#pragma once
#include "SimpleAverageCalc.h"

#include <string>
#include <array>

namespace InnoEngine
{
    enum class ProfilePoint
    {
        MainThreadTotal = 0,
        UpdateThreadTotal,
        WaitAndSynchronize,

        LayerUpdate,
        LayerRender,
        LayerEvent,

        ProcessRenderCommands,
        GPUSwapChainWait,

        // keep as last
        Count
    };

    constexpr std::string GetProfileElementString( ProfilePoint ppoint )
    {
        switch ( ppoint ) {
        case ProfilePoint::MainThreadTotal:
            return "MainThreadTotal";
        case ProfilePoint::UpdateThreadTotal:
            return "UpdateThreadTotal";
        case ProfilePoint::LayerUpdate:
            return "Layer Update";
        case ProfilePoint::LayerRender:
            return "Layer Render";
        case ProfilePoint::LayerEvent:
            return "Layer Event";
        case ProfilePoint::ProcessRenderCommands:
            return "Process Render Commands";
        case ProfilePoint::GPUSwapChainWait:
            return "Wait for GPUSwapchain";
        case ProfilePoint::Count:
            return "Invalid Profileelement";
        }
        return "Unknown";
    };

    class Profiler
    {
#pragma warning( push )
#pragma warning( disable :4324 )

        struct alignas( std::hardware_destructive_interference_size ) Timing
        {
            uint64_t              Current;
            uint64_t              TotalFrame;
            AverageCalc<uint64_t> AverageCalc;
        };

#pragma warning( pop )

        Profiler() = default;

    public:
        [[nodiscard]]
        static auto create() -> std::optional<Own<Profiler>>;

        void update();

        void start( ProfilePoint ppoint );
        void stop( ProfilePoint ppoint );

        // returns average time in nanoseconds
        uint64_t get_average( ProfilePoint ppoint );

    private:
        std::array<Timing, static_cast<size_t>( ProfilePoint::Count )> m_timings;
    };
}    // namespace InnoEngine
