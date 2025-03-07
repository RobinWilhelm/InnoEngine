#include "iepch.h"
#include "Profiler.h"

namespace InnoEngine
{
    auto Profiler::create() -> std::optional<Own<Profiler>>
    {
        Own<Profiler> profiler = Own<Profiler>( new Profiler() );
        for ( size_t i = 0; i < profiler->m_timings.size(); i++ ) {
            profiler->m_timings[ i ].AverageCalc.init( 3, 10 );
        }

        return profiler;
    }

    void Profiler::update()
    {
        for ( size_t i = 0; i < m_timings.size(); i++ ) {
            m_timings[ i ].AverageCalc.update( m_timings[ i ].TotalFrame );
            m_timings[ i ].TotalFrame = 0;
        }
    }

    void Profiler::start( ProfileElements element )
    {
        m_timings[ static_cast<uint32_t>( element ) ].Current = get_tick_count();
    }

    void Profiler::stop( ProfileElements element )
    {
        uint64_t deltaTime = get_tick_count() - m_timings[ static_cast<uint32_t>( element ) ].Current;
        m_timings[ static_cast<uint32_t>( element ) ].TotalFrame += deltaTime;
    }

    float Profiler::get_average( ProfileElements element )
    {
        return m_timings[ static_cast<uint32_t>( element ) ].AverageCalc.get_average();
    }
}    // namespace InnoEngine
