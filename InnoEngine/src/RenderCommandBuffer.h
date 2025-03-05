#pragma once

#include <utility>
#include <vector>
#include <array>
#include <algorithm>
#include <new>

namespace InnoEngine
{
    enum class RenderBufferQueue
    {
        First,
        Second
    };

    template <typename T>
    class RenderCommandQueue
    {
    public:
        RenderCommandQueue( size_t expectedQueueSize = 1000 )
        {
            m_firstQueue.reserve( expectedQueueSize );
            m_secondQueue.reserve( expectedQueueSize );
        }

        virtual ~RenderCommandQueue() = default;

        [[nodiscard]]
        T& create_entry()
        {
            grow_if_needed();
            auto&  ccmd   = get_collecting_queue();
            return ccmd.emplace_back();
        }

        void grow_if_needed()
        {
            auto& ccmd = get_collecting_queue();
            if ( ccmd.size() == ccmd.capacity() ) {
                // Grow by a factor of 2.
                ccmd.reserve( ccmd.capacity() * 2 );
            }
        }

        void on_submit()
        {
            size_t items = get_collecting_queue().size();
            if ( items != 0 ) {
                switch_buffers();

                // clear to get ready for collecting next frames commands
                auto& collectQueue = get_collecting_queue();
                collectQueue.clear();
                collectQueue.reserve( items );
            }
        }

        void switch_buffers()
        {
            m_collectingQueue = ( m_collectingQueue == RenderBufferQueue::First ) ? RenderBufferQueue::Second : RenderBufferQueue::First;
        }

        std::vector<T>& get_collecting_queue()
        {
            return ( m_collectingQueue == RenderBufferQueue::First ) ? m_firstQueue : m_secondQueue;
        }

        std::vector<T>& get_dispatching_queue()
        {
            return ( m_collectingQueue == RenderBufferQueue::First ) ? m_secondQueue : m_firstQueue;
        }

    private:
        RenderBufferQueue m_collectingQueue = RenderBufferQueue::First;
        std::vector<T>    m_firstQueue;
        std::vector<T>    m_secondQueue;
    };

    template <typename T>
    class DoubleBuffered
    {
    public:
        void swap()
        {
            m_switch = !m_switch;
        }

        T& get_first()
        {
            return m_switch ? m_second : m_first;
        }

        T& get_second()
        {
            return m_switch ? m_first : m_second;
        }

    private:
        bool m_switch = false;
        // force it to align to cache lines to prevent false sharing
        alignas( std::hardware_destructive_interference_size ) T m_first;
        alignas( std::hardware_destructive_interference_size ) T m_second;
    };

}    // namespace InnoEngine
