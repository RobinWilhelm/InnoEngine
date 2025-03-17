#pragma once
#include "InnoEngine/BaseTypes.h"

#include <string>
#include <string_view>

namespace InnoEngine
{
    using StringArenaIndex = uint32_t;

    class StringArena
    {
    public:
        StringArena( uint32_t initial_size = 10000, float grow_factor = 2.0f );


        StringArena& operator=(StringArena& other);

        StringArenaIndex insert( std::string_view string );
        void             clear();
        void             grow( uint32_t new_size );
        const char*      get_string(StringArenaIndex index) const ;
        size_t size() const;

    private:
        Own<char[]>      m_data       = nullptr;
        StringArenaIndex m_currentIdx = 0;
        uint32_t         m_size       = 0;
        float            m_growFactor = 0.0f;
    };
}    // namespace InnoEngine
