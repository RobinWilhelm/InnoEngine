#pragma once
#include "BaseTypes.h"

#include <vector>
#include <cstdint>

namespace InnoEngine
{
    // will

    template <typename ProbeType>
    class AverageCalc
    {
    public:
        AverageCalc() = default;
        AverageCalc( uint32_t numProbes, uint32_t probeFrequency );
        ~AverageCalc() = default;

        void Init( uint32_t numProbes, uint32_t probeFrequency );

        bool      Update( ProbeType probe);
        ProbeType GetAverage();

    private:
        std::vector<ProbeType> m_lastProbes          = {};
        uint32_t               m_lastProbeIndex      = 0;
        ProbeType              m_averageProbe        = 0;
        uint64_t               m_lastUpdateTimestamp = 0;
        uint64_t               m_timeBetweenUpates   = 0;
    };

    template <typename ProbeType>
    inline AverageCalc<ProbeType>::AverageCalc( uint32_t numProbes, uint32_t probeFrequency )
    {
        Init( numProbes, probeFrequency );
    }

    template <typename ProbeType>
    inline void AverageCalc<ProbeType>::Init( uint32_t numProbes, uint32_t probeFrequency )
    {
        m_lastProbes.resize( numProbes );
        m_timeBetweenUpates = probeFrequency > 0 ? TicksPerSecond / probeFrequency : 0;
    }

    template <typename ProbeType>
    inline bool AverageCalc<ProbeType>::Update(ProbeType probe)
    {
        if ( m_timeBetweenUpates > 0 && getTickCount64() - m_lastUpdateTimestamp < m_timeBetweenUpates ) {
            return false;
        }

        m_lastProbes[ m_lastProbeIndex ] = probe;
        m_lastProbeIndex++;
        if ( m_lastProbeIndex == m_lastProbes.size() )
            m_lastProbeIndex = 0;

        ProbeType sum = 0;
        for ( size_t i = 0; i < m_lastProbes.size(); i++ )
            sum += m_lastProbes[ i ];

        m_averageProbe = sum / static_cast<int32_t>( m_lastProbes.size() );

        m_lastUpdateTimestamp = getTickCount64();
        return true;
    }

    template <typename ProbeType>
    inline ProbeType AverageCalc<ProbeType>::GetAverage()
    {
        return m_averageProbe;
    }
}    // namespace InnoEngine
