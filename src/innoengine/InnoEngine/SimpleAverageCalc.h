#pragma once
#include "BaseTypes.h"

#include <vector>
#include <cstdint>

namespace InnoEngine
{
    template <typename ProbeType>
    class AverageCalc
    {
    public:
        AverageCalc() = default;
        AverageCalc( uint32_t numProbes, uint32_t probeFrequency );
        AverageCalc(uint32_t numProbes, uint32_t probeFrequency, ProbeType ignore_val);
        ~AverageCalc() = default;

        void init( uint32_t numProbes, uint32_t probeFrequency );
        void init(uint32_t numProbes, uint32_t probeFrequency, ProbeType ignore_val);

        bool      update( ProbeType probe);
        ProbeType get_average();

    private:
        uint64_t               m_lastUpdateTimestamp = 0;
        uint64_t               m_timeBetweenUpates   = 0;
        std::vector<ProbeType> m_lastProbes          = {};
        uint32_t               m_lastProbeIndex      = 0;
        ProbeType              m_averageProbe        = {};
        ProbeType              m_ignoreProbeVal      = {};
        bool                   m_ignoreProbe         = false;
    };

    template <typename ProbeType>
    inline AverageCalc<ProbeType>::AverageCalc( uint32_t numProbes, uint32_t probeFrequency )
    {
        init( numProbes, probeFrequency );
    }

    template<typename ProbeType>
    inline AverageCalc<ProbeType>::AverageCalc(uint32_t numProbes, uint32_t probeFrequency, ProbeType ignore_val)
    {
        init(numProbes, probeFrequency);
    }

    template <typename ProbeType>
    inline void AverageCalc<ProbeType>::init( uint32_t numProbes, uint32_t probeFrequency )
    {
        m_lastProbes.resize( numProbes );
        m_timeBetweenUpates = probeFrequency > 0 ? TicksPerSecond / probeFrequency : 0;
    }

    template <typename ProbeType>
    inline void AverageCalc<ProbeType>::init(uint32_t numProbes, uint32_t probeFrequency, ProbeType ignore_val)
    {
        m_ignoreProbe = true;
        m_ignoreProbeVal = ignore_val;
        m_lastProbes.resize(numProbes);
        m_timeBetweenUpates = probeFrequency > 0 ? TicksPerSecond / probeFrequency : 0;
    }

    template <typename ProbeType>
    inline bool AverageCalc<ProbeType>::update(ProbeType probe)
    {
        if ( m_timeBetweenUpates > 0 && get_tick_count() - m_lastUpdateTimestamp < m_timeBetweenUpates ) {
            return false;
        }

        m_lastProbes[ m_lastProbeIndex ] = probe;
        m_lastProbeIndex++;
        if ( m_lastProbeIndex == m_lastProbes.size() )
            m_lastProbeIndex = 0;

        ProbeType sum = 0;
        int32_t probe_count = 0;
        for ( size_t i = 0; i < m_lastProbes.size(); i++ )  {
            if(m_ignoreProbe == false || m_lastProbes[i] != m_ignoreProbeVal) {
                sum += m_lastProbes[ i ];
                ++probe_count;
            }      
        }

        m_averageProbe = sum / probe_count;

        m_lastUpdateTimestamp = get_tick_count();
        return true;
    }

    template <typename ProbeType>
    inline ProbeType AverageCalc<ProbeType>::get_average()
    {
        return m_averageProbe;
    }
}    // namespace InnoEngine
