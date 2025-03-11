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
        m_collectQueue.reserve( expectedQueueSize );
        m_dispatchQueue.reserve( expectedQueueSize );
    }

    virtual ~RenderCommandQueue() = default;

    [[nodiscard]]
    T& create_entry()
    {
        grow_if_needed();
        return m_collectQueue.emplace_back();
    }

    void grow_if_needed()
    {
        if ( m_collectQueue.size() == m_collectQueue.capacity() ) {
            // Grow by a factor of 2.
            m_collectQueue.reserve( m_collectQueue.capacity() * 2 );
        }
    }

    void on_submit()
    {
        size_t items = m_collectQueue.size();
        if ( items != 0 ) {
            switch_buffers();

            // clear to get ready for collecting next frames commands
            m_collectQueue.clear();
            m_collectQueue.reserve( items );
        }
    }

    void switch_buffers()
    {
        // make a pointer swap
        m_collectQueue.swap( m_dispatchQueue );
    }

    std::vector<T>& get_collecting_queue()
    {
        return m_collectQueue;
    }

    std::vector<T>& get_dispatching_queue()
    {
        return m_dispatchQueue;
    }

private:
    // force it to align to cache lines to prevent false sharing
    alignas( std::hardware_destructive_interference_size ) std::vector<T> m_collectQueue;
    alignas( std::hardware_destructive_interference_size ) std::vector<T> m_dispatchQueue;
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
#pragma warning( push )
#pragma warning( disable :4324 )
    alignas( std::hardware_destructive_interference_size ) T m_first;
    alignas( std::hardware_destructive_interference_size ) T m_second;
#pragma warning( pop )
};

}    // namespace InnoEngine
