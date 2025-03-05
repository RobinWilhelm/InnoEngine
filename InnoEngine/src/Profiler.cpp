#include "iepch.h"
#include "Profiler.h"

namespace InnoEngine
{
    auto Profiler::create() -> std::optional<Owned<Profiler>>
    {
        Owned<Profiler> profiler = Owned<Profiler>( new Profiler() );
        for ( size_t i = 0; i < profiler->m_timings.size(); i++ ) {
            profiler->m_timingsAvg[ i ].Init( 10, 10 );
        }

        return profiler;
    }

    void Profiler::update()
    {
        for ( size_t i = 0; i < m_timings.size(); i++ ) {
            m_timingsAvg[ i ].Update( m_timings[ i ].TotalFrame );
            m_timings[ i ].TotalFrame = 0;
        }
    }

    void Profiler::start( ProfileElements element )
    {
        m_timings[ static_cast<uint32_t>( element ) ].Current = getTickCount64();
    }

    void Profiler::stop( ProfileElements element )
    {
        uint64_t deltaTime = getTickCount64() - m_timings[ static_cast<uint32_t>( element ) ].Current;
        m_timings[ static_cast<uint32_t>( element ) ].TotalFrame += deltaTime;
    }

    float Profiler::get_average( ProfileElements element )
    {
        return m_timingsAvg[ static_cast<uint32_t>( element ) ].GetAverage();
    }
}    // namespace InnoEngine
