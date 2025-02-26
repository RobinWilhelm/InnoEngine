#pragma once

#include <utility>
#include <vector>
#include <array>
#include <algorithm>

enum class RenderBufferQueue
{
    First,
    Second
};

class RenderCommandQueue
{
    friend class GPUPipeline;

public:
    virtual ~RenderCommandQueue() = default;

protected:
    virtual void on_submit() = 0;
};

template <typename T>
class DoubleBufferedCommandQueue : public RenderCommandQueue
{
public:
    DoubleBufferedCommandQueue( size_t expectedQueueSize = 1000 );
    virtual ~DoubleBufferedCommandQueue() = default;

    const std::vector<T*>& get_rendercommands();

    [[nodiscard]]
    T*   create_entry();
    void grow_if_needed();
    void sort( bool ( *compare )( const T*, const T* ) );

private:
    // Geerbt über RenderCommandQueue
    void on_submit() override;
    void switch_buffers();

    std::vector<T>&  get_collecting_queue();
    std::vector<T*>& get_collecting_ptrqueue();

    const std::vector<T>& get_dispatching_queue();
    std::vector<T*>&      get_dispatching_ptrqueue();

private:
    RenderBufferQueue m_collectingQueue = RenderBufferQueue::First;
    std::vector<T>    m_firstQueue;
    std::vector<T>    m_secondQueue;

    std::vector<T*> m_firstQueueSorted;
    std::vector<T*> m_secondQueueSorted;
};

template <typename T>
inline DoubleBufferedCommandQueue<T>::DoubleBufferedCommandQueue( size_t expectedQueueSize )
{
    m_firstQueue.reserve( expectedQueueSize );
    m_secondQueue.reserve( expectedQueueSize );

    m_firstQueueSorted.reserve( expectedQueueSize );
    m_secondQueueSorted.reserve( expectedQueueSize );
}

template <typename T>
inline std::vector<T>& DoubleBufferedCommandQueue<T>::get_collecting_queue()
{
    return ( m_collectingQueue == RenderBufferQueue::First ) ? m_firstQueue : m_secondQueue;
}

template <typename T>
inline void DoubleBufferedCommandQueue<T>::on_submit()
{
    size_t items = get_collecting_queue().size();
    if ( items != 0 ) {
        switch_buffers();

        // clear to get ready for collecting next frames commands
        auto& collectQueue = get_collecting_queue();
        collectQueue.clear();
        collectQueue.reserve( items );

        auto& collectQueueSorted = get_collecting_ptrqueue();
        collectQueueSorted.clear();
        collectQueueSorted.reserve( items );
    }
}

template <typename T>
inline void DoubleBufferedCommandQueue<T>::switch_buffers()
{
    m_collectingQueue = ( m_collectingQueue == RenderBufferQueue::First ) ? RenderBufferQueue::Second : RenderBufferQueue::First;
}

template <typename T>
const inline std::vector<T>& DoubleBufferedCommandQueue<T>::get_dispatching_queue()
{
    return ( m_collectingQueue == RenderBufferQueue::First ) ? m_secondQueue : m_firstQueue;
}

template <typename T>
std::vector<T*>& DoubleBufferedCommandQueue<T>::get_collecting_ptrqueue()
{
    return ( m_collectingQueue == RenderBufferQueue::First ) ? m_firstQueueSorted : m_secondQueueSorted;
}

template <typename T>
inline std::vector<T*>& DoubleBufferedCommandQueue<T>::get_dispatching_ptrqueue()
{
    return ( m_collectingQueue == RenderBufferQueue::First ) ? m_secondQueueSorted : m_firstQueueSorted;
}

template <typename T>
const inline std::vector<T*>& DoubleBufferedCommandQueue<T>::get_rendercommands()
{
    return get_dispatching_ptrqueue();
}

template <typename T>
inline T* DoubleBufferedCommandQueue<T>::create_entry()
{
    auto&  ccmd   = get_collecting_queue();
    size_t newIdx = ccmd.size();
    ccmd.emplace_back( T {} );

    auto& ccmds  = get_collecting_ptrqueue();
    auto  cmdPtr = &ccmd[newIdx];
    ccmds.emplace_back( cmdPtr );
    return cmdPtr;
}

template <typename T>
inline void DoubleBufferedCommandQueue<T>::grow_if_needed()
{
    auto& ccmd = get_collecting_queue();
    if ( ccmd.size() == ccmd.capacity() ) {
        // Grow by a factor of 2.
        ccmd.reserve( ccmd.capacity() * 2 );
    }
}

template <typename T>
inline void DoubleBufferedCommandQueue<T>::sort( bool ( *compare )( const T*, const T* ) )
{
    std::vector<T*>& dispatchqueue = get_dispatching_ptrqueue();
    std::sort( dispatchqueue.begin(), dispatchqueue.end(), compare );
}
