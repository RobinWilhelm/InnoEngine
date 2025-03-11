#include "iepch.h"
#include "Profiler.h"

namespace InnoEngine
{
    auto Profiler::create() -> std::optional<Own<Profiler>>
    {
        Own<Profiler> profiler = Own<Profiler>( new Profiler() );
        for ( size_t i = 0; i < profiler->m_timings.size(); i++ ) {
            profiler->m_timings[ i ].AverageCalc.init( 10, 3 );
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

    void Profiler::start( ProfilePoint ppoint )
    {
        m_timings[ static_cast<uint32_t>( ppoint ) ].Current = get_tick_count();
    }

    void Profiler::stop( ProfilePoint ppoint )
    {
        uint64_t deltaTime = get_tick_count() - m_timings[ static_cast<uint32_t>( ppoint ) ].Current;
        m_timings[ static_cast<uint32_t>( ppoint ) ].TotalFrame += deltaTime;
    }

    uint64_t Profiler::get_average( ProfilePoint ppoint )
    {
        return m_timings[ static_cast<uint32_t>( ppoint ) ].AverageCalc.get_average();
    }
}    // namespace InnoEngine
