#include "InnoEngine/iepch.h"
#include "InnoEngine/utility/Profiler.h"

namespace InnoEngine
{
    auto Profiler::create() -> std::optional<Own<Profiler>>
    {
        Own<Profiler> profiler = Own<Profiler>( new Profiler() );
        for ( size_t i = 0; i < profiler->m_timings.size(); i++ ) {
            profiler->m_timings[ i ].AverageCalc.init( 5, 5, 0 );
        }

        ProfileScoped::ms_Profiler = profiler.get();
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
        IE_ASSERT( m_timings[ static_cast<uint32_t>( ppoint ) ].Active == false );
        m_timings[ static_cast<uint32_t>( ppoint ) ].Current = get_tick_count();
        m_timings[static_cast<uint32_t>(ppoint)].Active = true;
    }

    void Profiler::stop( ProfilePoint ppoint )
    {
        if ( m_timings[ static_cast<uint32_t>( ppoint ) ].Active ) {
            uint64_t deltaTime = get_tick_count() - m_timings[ static_cast<uint32_t>( ppoint ) ].Current;
            m_timings[ static_cast<uint32_t>( ppoint ) ].TotalFrame += deltaTime;
            m_timings[ static_cast<uint32_t>( ppoint ) ].Active = false;
        }
    }

    uint64_t Profiler::get_average( ProfilePoint ppoint )
    {
        return m_timings[ static_cast<uint32_t>( ppoint ) ].AverageCalc.get_average();
    }

    Profiler* ProfileScoped::ms_Profiler = nullptr;

    ProfileScoped::ProfileScoped( ProfilePoint profile_point )
    {
        ms_Profiler->start( profile_point );
        m_ProfilePoint = profile_point;
    }

    ProfileScoped::~ProfileScoped()
    {
        ms_Profiler->stop( m_ProfilePoint );
    }

    void ProfileScoped::stop()
    {
        ms_Profiler->stop( m_ProfilePoint );
    }
}    // namespace InnoEngine
