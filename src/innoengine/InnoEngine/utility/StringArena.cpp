#include "InnoEngine/iepch.h"
#include "InnoEngine/utility/StringArena.h"

#include "InnoEngine/IE_Assert.h"

namespace InnoEngine
{
    StringArena::StringArena( uint32_t initial_size, float grow_factor )
    {
        IE_ASSERT( grow_factor > 1.0f );
        m_growFactor = grow_factor;
        m_size       = initial_size;
        m_data       = std::make_unique<char[]>( initial_size );
        m_currentIdx = 0;
    }

    StringArena& StringArena::operator=( StringArena& other )
    {
        if ( this == &other )
            return *this;

        if ( other.m_size > m_size )
            m_data = std::make_unique<char[]>( other.m_size );

        std::memcpy( static_cast<void*>( &m_data[ 0 ] ), static_cast<void*>( &other.m_data[ 0 ] ), other.m_size );
        m_currentIdx = other.m_currentIdx;
        m_size       = other.m_size;
        m_growFactor = other.m_growFactor;
        return *this;
    }

    StringArenaIndex StringArena::insert( std::string_view string )
    {
        if ( m_currentIdx + string.size() + 1 > m_size ) {
            IE_ASSERT( static_cast<uint64_t>( m_size ) * m_growFactor <= ( std::numeric_limits<uint32_t>::max )() );
            grow( static_cast<uint32_t>( m_size * m_growFactor ) );
        }

        std::memcpy( static_cast<void*>( &m_data[ m_currentIdx ] ), string.data(), string.size() );
        m_data[ m_currentIdx + string.size() ] = '\0';

        StringArenaIndex string_begin_idx = m_currentIdx;
        m_currentIdx += static_cast<uint32_t>( string.size() );
        return string_begin_idx;
    }

    void StringArena::clear()
    {
        m_currentIdx = 0;
    }

    void StringArena::grow( uint32_t new_size )
    {
        IE_ASSERT( new_size > m_size );
        Own<char[]> new_data = std::make_unique<char[]>( new_size );

        std::memcpy( &new_data[ 0 ], &m_data[ 0 ], m_size );
        m_data = std::move( new_data );
        m_size = new_size;
    }

    const char* StringArena::get_string( StringArenaIndex index ) const
    {
        return &m_data[ index ];
    }

    size_t StringArena::size() const
    {
        return static_cast<size_t>( m_currentIdx );
    }
}    // namespace InnoEngine
