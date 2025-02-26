#pragma once
#include <type_traits>
#include <iterator>
#include <limits>
#include <cassert>        // assert
#include <cstring>        // memcpy
#include <minwindef.h>    // numeric_limits

enum class ObjectPoolType
{
    // Allocations will get the last deallocated spaces first
    LastOutFirstIn = 0,

    // Keeps the free pointer chain ordered from lowest to highest address
    // This makes sure that new allocates are filling the first gaps first and try to retain sequentiality
    RestoreSequence,

    // Moves the last object into the deallocated space to always retain sequentiality
    // New objects will always be created at the back
    // The Object needs to be trivially copyable!
    Reorder
};

template <typename T, typename SizeType, ObjectPoolType PoolType>
class ObjectPool
{
    static_assert( std::is_integral<SizeType>::value );
    static_assert( std::is_unsigned<SizeType>::value );

    struct ObjectWithMetaData
    {
        T        Obj;
        SizeType NextFreeOffset = 0;
    };

    using Element = std::conditional_t<PoolType == ObjectPoolType::LastOutFirstIn || PoolType == ObjectPoolType::RestoreSequence, ObjectWithMetaData, T>;

public:
    static constexpr SizeType ObjectSize  = sizeof( T );
    static constexpr SizeType ElementSize = sizeof( Element );

    ObjectPool() = default;

    ObjectPool( SizeType objectCount )
    {
        build_pool( objectCount );
    }

    ObjectPool( const ObjectPool& other )            = delete;
    ObjectPool& operator=( const ObjectPool& other ) = delete;
    ObjectPool( ObjectPool&& other )                 = delete;
    ObjectPool& operator=( ObjectPool&& other )      = delete;

    ~ObjectPool()
    {
        clear();
        destroy_pool();
    }

    void build_pool( SizeType ObjectAmount )
    {
        clear();
        destroy_pool();

        m_reservedAmount = ObjectAmount;
        size_t totalSize = ElementSize * ObjectAmount;
        assert( totalSize <= ( std::numeric_limits<SizeType>::max )() );
        m_totalMemSize = static_cast<SizeType>( totalSize );

        if ( m_totalMemSize > 0 ) {
            size_t align = alignof( Element );
            m_memBegin   = reinterpret_cast<Element*>( _aligned_malloc( m_totalMemSize, align ) );
            clear();
        }
    }

    template <typename... Args>
    T* create( Args&&... args )
    {
        void* p = allocate();
        return new ( p ) T( std::forward<Args>( args... ) );
    }

    void destroy( T* object )
    {
        object->~T();
        deallocate( object );
    }

    void* allocate() noexcept
    {
        if ( m_currentObjectCount == m_reservedAmount )
            return nullptr;

        Element* newElement = get_firstfree();

        if ( m_firstFreeOffset > m_lastAliveOffset )
            m_lastAliveOffset = m_firstFreeOffset;    // new element was allocated at the back

        if constexpr ( PoolType == ObjectPoolType::LastOutFirstIn || PoolType == ObjectPoolType::RestoreSequence ) {
            m_firstFreeOffset          = newElement->NextFreeOffset;
            newElement->NextFreeOffset = AliveTag;
        } else if constexpr ( PoolType == ObjectPoolType::Reorder ) {
            m_firstFreeOffset += ElementSize;
        }
        m_currentObjectCount++;
        assert( m_firstFreeOffset >= 0 && m_firstFreeOffset <= m_totalMemSize );
        return reinterpret_cast<void*>( newElement );
    }

    void deallocate( void* objectDeleted ) noexcept
    {
        assert( objectDeleted != nullptr );
        assert( reinterpret_cast<unsigned char*>( objectDeleted ) >= reinterpret_cast<unsigned char*>( m_memBegin ) &&
                reinterpret_cast<unsigned char*>( objectDeleted ) <= reinterpret_cast<unsigned char*>( m_memBegin ) + m_totalMemSize - ElementSize );
        assert( get_membyteoffset( reinterpret_cast<Element*>( objectDeleted ) ) % ElementSize == 0 );

        if constexpr ( PoolType == ObjectPoolType::LastOutFirstIn ) {
            Element* pContDelete = reinterpret_cast<Element*>( objectDeleted );
            assert( alive( pContDelete ) == true );
            SizeType* pNextFreeOffset = &pContDelete->NextFreeOffset;
            SizeType  offsetDeleted   = get_membyteoffset( pContDelete );

            *pNextFreeOffset  = m_firstFreeOffset;
            m_firstFreeOffset = offsetDeleted;

            if ( offsetDeleted == m_lastAliveOffset ) {
                // deleted the last element -> we need to find the new last element
                // this is done by just iterating backwards from the deleted element until an alive element is found
                while ( m_lastAliveOffset > 0 ) {
                    m_lastAliveOffset -= ElementSize;
                    if ( alive( get_lastalive() ) )
                        break;    // found the new last element
                }
            }
        } else if constexpr ( PoolType == ObjectPoolType::RestoreSequence ) {
            Element* pContDelete = reinterpret_cast<Element*>( objectDeleted );
            assert( alive( pContDelete ) == true );
            SizeType offsetDeleted = get_membyteoffset( pContDelete );

            // the deleted element might not be the front most deadspace
            // search for the right place to link back in the free element chain
            SizeType* freeOffset        = &m_firstFreeOffset;
            Element*  pFreeBefore       = get_firstfree();
            pContDelete->NextFreeOffset = m_totalMemSize;
            if ( pFreeBefore ) {
                while ( offsetDeleted > pFreeBefore->NextFreeOffset && pFreeBefore ) {
                    assert( alive( pFreeBefore ) == false );
                    pFreeBefore = get_nextfree( pFreeBefore );
                }
                freeOffset                  = &pFreeBefore->NextFreeOffset;
                pContDelete->NextFreeOffset = pFreeBefore->NextFreeOffset;
            }
            *freeOffset = offsetDeleted;

            if ( offsetDeleted == m_lastAliveOffset ) {
                // deleted the last element -> we need to find the new last element
                // this is dont by just iterating backwards from the deleted element until an alive element is found
                while ( m_lastAliveOffset > 0 ) {
                    m_lastAliveOffset -= ElementSize;
                    if ( alive( get_lastalive() ) )
                        break;    // found the new last element
                }
            }
        } else if constexpr ( PoolType == ObjectPoolType::Reorder ) {
            // move last object in the deallocated space, if the object was not already the last
            const T* last = get_lastalive();
            if ( last != objectDeleted )
                std::memcpy( objectDeleted, last, ObjectSize );

            assert( m_firstFreeOffset >= ElementSize );
            m_firstFreeOffset -= ElementSize;
            if ( m_lastAliveOffset > 0 )
                m_lastAliveOffset -= ElementSize;
        }

        assert( m_firstFreeOffset >= 0 && m_firstFreeOffset <= m_totalMemSize );
        m_currentObjectCount--;
    }

    void clear() noexcept
    {
        if ( m_memBegin ) {
            // dont forget to delete the remaining alive elements
            auto it = begin();
            while ( it != end() ) {
                it->~T();
                ++it;
            }

            std::memset( m_memBegin, 0, m_totalMemSize );
            if constexpr ( PoolType == ObjectPoolType::LastOutFirstIn || PoolType == ObjectPoolType::RestoreSequence ) {
                // setup the nextFree pointer chain, the last 0
                Element* pCont = m_memBegin;
                for ( SizeType index = 0; index < m_reservedAmount; index++ ) {
                    pCont->NextFreeOffset = ( index + 1 ) * ElementSize;
                    pCont++;
                }
            }
        }
        m_firstFreeOffset    = 0;
        m_currentObjectCount = 0;
        m_lastAliveOffset    = 0;
    }

    SizeType get_total_size() const noexcept
    {
        return m_totalMemSize;
    }

    SizeType get_current_object_count() const noexcept
    {
        return m_currentObjectCount;
    }

    SizeType get_free_object_count() const noexcept
    {
        return m_reservedAmount - m_currentObjectCount;
    }

    // returns a value between 0 and 1, indicating the consecutiveness of the object in memory
    // 1 meaning they are perfectly consecutive
    float check_health() const noexcept
    {
        if constexpr ( PoolType == ObjectPoolType::LastOutFirstIn || PoolType == ObjectPoolType::RestoreSequence ) {
            if ( m_currentObjectCount == 0 )
                return 1.0f;

            return static_cast<float>( m_currentObjectCount * ElementSize ) / ( m_lastAliveOffset + ElementSize );
        } else if constexpr ( PoolType == ObjectPoolType::Reorder ) {
            return 1.0f;
        }
    }

    class Iterator
    {
        friend class ObjectPool;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = SizeType;
        using value_type        = T;
        using pointer           = T*;    // or also value_type*
        using reference         = T&;    // or also value_type&

        Iterator( pointer ptr, SizeType index, const ObjectPool* pool ) :
            m_ptr( ptr ), m_index( index ), m_pool( pool )
        { }

        Iterator( const Iterator& other )
        {
            m_ptr   = other.m_ptr;
            m_index = other.m_index;
            m_pool  = other.m_pool;
        }

        Iterator& operator=( const Iterator& other )
        {
            m_ptr   = other.m_ptr;
            m_index = other.m_index;
            m_pool  = other.m_pool;
            return *this;
        }

        reference operator*() const
        {
            return *m_ptr;
        }

        pointer operator->()
        {
            return m_ptr;
        }

        // Prefix increment
        Iterator& operator++()
        {
            m_ptr = reinterpret_cast<T*>( m_pool->find_next( reinterpret_cast<Element*>( m_ptr ), m_index ) );
            m_index++;
            return *this;
        }

        // Postfix increment
        Iterator operator++( int )
        {
            Iterator tmp = *this;
            ++( *this );
            return tmp;
        }

        Iterator& operator+=( const SizeType& counter )
        {
            for ( SizeType count = 0; count < counter; count++ ) {
                m_ptr = reinterpret_cast<T*>( m_pool->find_next( reinterpret_cast<Element*>( m_ptr ), m_index ) );
                m_index++;
                if ( m_ptr == m_pool->end().m_ptr )
                    return *this;
            }
            return *this;
        }

        friend Iterator operator+( Iterator iter, const SizeType& count )
        {
            iter += count;
            return iter;
        }

        friend bool operator==( const Iterator& a, const Iterator& b )
        {
            return a.m_ptr == b.m_ptr;
        };

        friend bool operator!=( const Iterator& a, const Iterator& b )
        {
            return a.m_ptr != b.m_ptr;
        };

    private:
        pointer           m_ptr   = nullptr;
        SizeType          m_index = 0;
        const ObjectPool* m_pool  = nullptr;
    };

    Iterator begin() const noexcept
    {
        return Iterator( reinterpret_cast<T*>( find_begin() ), 0, this );
    }

    Iterator end() const noexcept
    {
        return Iterator( reinterpret_cast<T*>( find_end() ), m_currentObjectCount, this );
    }

    Iterator erase( Iterator iterator ) noexcept
    {
        if constexpr ( PoolType == ObjectPoolType::LastOutFirstIn || PoolType == ObjectPoolType::RestoreSequence ) {
            if ( m_currentObjectCount - iterator.m_index == 1 ) {
                reinterpret_cast<T*>( iterator.m_ptr )->~T();
                deallocate( iterator.m_ptr );
                return end();
            } else {
                SizeType index     = iterator.m_index;
                Element* pNextCont = find_next( reinterpret_cast<Element*>( iterator.m_ptr ), index );
                reinterpret_cast<T*>( iterator.m_ptr )->~T();
                deallocate( iterator.m_ptr );
                return Iterator( reinterpret_cast<T*>( pNextCont ), index, this );
            }
        } else if constexpr ( PoolType == ObjectPoolType::Reorder ) {
            // we can just return the same iterator again because the deallocate will shuffle a new element in the current position
            reinterpret_cast<T*>( iterator.m_ptr )->~T();
            deallocate( iterator.m_ptr );
            return iterator;
        }
    }

    bool empty()
    {
        return m_currentObjectCount == 0;
    }

    bool full()
    {
        return m_currentObjectCount == m_reservedAmount;
    }

private:
    void destroy_pool() noexcept
    {
        if ( m_memBegin ) {
            _aligned_free( m_memBegin );
            m_memBegin = nullptr;
        }
    }

    // returns true if the object pointer belongs to an alive object
    bool alive( const Element* element ) const noexcept
    {
        assert( element != nullptr );
        assert( reinterpret_cast<const unsigned char*>( element ) >= reinterpret_cast<const unsigned char*>( m_memBegin ) &&
                reinterpret_cast<const unsigned char*>( element ) <= reinterpret_cast<const unsigned char*>( m_memBegin ) + m_totalMemSize - ElementSize );
        assert( get_membyteoffset( element ) % ElementSize == 0 );

        if constexpr ( PoolType == ObjectPoolType::LastOutFirstIn || PoolType == ObjectPoolType::RestoreSequence ) {
            return element->NextFreeOffset == AliveTag;
        } else if constexpr ( PoolType == ObjectPoolType::Reorder ) {
            return element < find_end();
        }
    }

    Element* get_firstfree() const noexcept
    {
        if ( m_firstFreeOffset == m_totalMemSize )
            return nullptr;

        return reinterpret_cast<Element*>( reinterpret_cast<unsigned char*>( m_memBegin ) + m_firstFreeOffset );
    }

    Element* get_nextfree( Element* element ) const noexcept
    {
        return ( element->NextFreeOffset == m_totalMemSize ) ? nullptr : reinterpret_cast<Element*>( reinterpret_cast<unsigned char*>( m_memBegin ) + element->NextFreeOffset );
    }

    Element* get_lastalive() const noexcept
    {
        return reinterpret_cast<Element*>( reinterpret_cast<unsigned char*>( m_memBegin ) + m_lastAliveOffset );
    }

    SizeType get_membyteoffset( const Element* element ) const noexcept
    {
        return reinterpret_cast<const unsigned char*>( element ) - reinterpret_cast<unsigned char*>( m_memBegin );
    }

    Element* find_begin() const noexcept
    {
        if constexpr ( PoolType == ObjectPoolType::LastOutFirstIn || PoolType == ObjectPoolType::RestoreSequence ) {
            if ( m_currentObjectCount == 0 )
                return m_memBegin;

            Element* pCont = m_memBegin;
            for ( SizeType counter = 0; counter < m_reservedAmount; counter++ ) {
                if ( alive( pCont ) )
                    return pCont;
                pCont++;
            }
            assert( 0 );
        } else if constexpr ( PoolType == ObjectPoolType::Reorder ) {
            return m_memBegin;
        }
        return nullptr;
    }

    Element* find_next( Element* element, SizeType index ) const noexcept
    {
        assert( element != nullptr );
        assert( reinterpret_cast<unsigned char*>( element ) >= reinterpret_cast<unsigned char*>( m_memBegin ) &&
                reinterpret_cast<unsigned char*>( element ) <= reinterpret_cast<unsigned char*>( m_memBegin ) + m_totalMemSize - ElementSize );
        assert( get_membyteoffset( element ) % ElementSize == 0 );

        if constexpr ( PoolType == ObjectPoolType::LastOutFirstIn || PoolType == ObjectPoolType::RestoreSequence ) {
            if ( index + 1 >= m_currentObjectCount )
                return find_end();

            Element* pNextElement = element;
            for ( SizeType counter = 0; counter < m_reservedAmount; counter++ ) {
                ++pNextElement;
                if ( pNextElement == find_end() )
                    return pNextElement;

                if ( alive( pNextElement ) )
                    return pNextElement;
            }
            // never get here
            assert( 0 );
        } else if constexpr ( PoolType == ObjectPoolType::Reorder ) {
            return ++element;
        }
        return nullptr;
    }

    Element* find_end() const noexcept
    {
        if ( m_currentObjectCount == 0 )
            return m_memBegin;

        Element* last = get_lastalive();
        return ++last;
    }

private:
    SizeType m_reservedAmount     = 0;
    SizeType m_totalMemSize       = 0;
    SizeType m_currentObjectCount = 0;
    SizeType m_firstFreeOffset    = 0;
    SizeType m_lastAliveOffset    = 0;
    Element* m_memBegin           = nullptr;

    static constexpr SizeType AliveTag = 1u;
};
