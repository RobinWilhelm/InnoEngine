#pragma once
#include "InnoEngine/Font.h"
#include "InnoEngine/Texture2D.h"

#include "InnoEngine/Sprite2DPipeline.h"
#include "InnoEngine/Font2DPipeline.h"
#include "InnoEngine/ImGuiPipeline.h"

#include "InnoEngine/StringArena.h"

#include "DirectXMath.h"
#include "Directxtk/SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include <vector>

namespace InnoEngine
{
    struct RenderCommandBuffer
    {
        struct Stats
        {
            size_t TotalCommands   = 0;
            size_t TotalDrawCalls  = 0;
            size_t TotalBufferSize = 0;
        };

        Stats m_lastFrameStats;

        bool         Clear;
        DXSM::Color  ClearColor;
        DXSM::Matrix CameraMatrix;

        TextureList TextureRegister;    // hold a reference to all texture objects we are going to use this frame

        SpriteCommandBuffer SpriteRenderCommands;

        StringArena       StringBuffer;    // arena like container to hold a copy of all strings we are going to render this frame
        FontList          FontRegister;    // hold a reference to all font objects we are going to use this frame
        FontCommandBuffer FontRenderCommands;

        ImGuiPipeline::CommandData ImGuiCommandBuffer;

        size_t SpriteDrawCalls;
        size_t FontDrawCalls;
        size_t ImGuiDrawCalls;

        RenderCommandBuffer()
        {
            SpriteRenderCommands.reserve( 20000 );
            FontRenderCommands.reserve( 20000 );
        }

        RenderCommandBuffer& operator=( RenderCommandBuffer& other )
        {
            if ( this == &other )
                return *this;

            Clear        = other.Clear;
            ClearColor   = other.ClearColor;
            CameraMatrix = other.CameraMatrix;

            SpriteRenderCommands.resize( other.SpriteRenderCommands.size() );
            std::memcpy( static_cast<void*>( SpriteRenderCommands.data() ),
                         static_cast<void*>( other.SpriteRenderCommands.data() ),
                         other.SpriteRenderCommands.size() * sizeof( Sprite2DPipeline::Command ) );

            ImGuiCommandBuffer = other.ImGuiCommandBuffer;

            FontRenderCommands.resize( other.FontRenderCommands.size() );
            std::memcpy( static_cast<void*>( FontRenderCommands.data() ),
                         static_cast<void*>( other.FontRenderCommands.data() ),
                         other.FontRenderCommands.size() * sizeof( Font2DPipeline::Command ) );

            StringBuffer = other.StringBuffer;

            FontRegister    = other.FontRegister;
            TextureRegister = other.TextureRegister;

            return *this;
        }

        void clear()
        {
            m_lastFrameStats = Stats();

            m_lastFrameStats.TotalBufferSize += sizeof( DXSM::Color );
            m_lastFrameStats.TotalBufferSize += sizeof( DXSM::Matrix );
            m_lastFrameStats.TotalBufferSize += TextureRegister.size() * sizeof( Ref<Texture2D> );

            m_lastFrameStats.TotalCommands += SpriteRenderCommands.size();
            m_lastFrameStats.TotalBufferSize += SpriteRenderCommands.size() * sizeof( Sprite2DPipeline::Command );

            m_lastFrameStats.TotalBufferSize += FontRegister.size() * sizeof( Ref<Font> );
            m_lastFrameStats.TotalCommands += FontRenderCommands.size();
            m_lastFrameStats.TotalBufferSize += FontRenderCommands.size() * sizeof( Font2DPipeline::Command );
            m_lastFrameStats.TotalBufferSize += StringBuffer.size();

            for ( const auto& rcmd : ImGuiCommandBuffer.RenderCommandLists ) {
                m_lastFrameStats.TotalCommands += rcmd.CommandBuffer.size();
                m_lastFrameStats.TotalBufferSize += rcmd.CommandBuffer.size() * sizeof( ImDrawCmd );
                m_lastFrameStats.TotalBufferSize += rcmd.IndexBuffer.size() * sizeof( ImDrawIdx );
                m_lastFrameStats.TotalBufferSize += rcmd.VertexBuffer.size() * sizeof( ImDrawVert );
            }

            m_lastFrameStats.TotalDrawCalls = SpriteDrawCalls + FontDrawCalls + ImGuiDrawCalls;

            Clear      = false;
            ClearColor = DXSM::Color( 0.0f, 0.0f, 0.0f, 0.0f );
            SpriteRenderCommands.clear();
            ImGuiCommandBuffer.RenderCommandLists.clear();

            FontRenderCommands.clear();
            StringBuffer.clear();

            FontRegister.clear();
            TextureRegister.clear();

            SpriteDrawCalls = 0;
            FontDrawCalls   = 0;
            ImGuiDrawCalls  = 0;
        }

        const Stats& get_stats() const
        {
            return m_lastFrameStats;
        }
    };

}    // namespace InnoEngine
