#pragma once
#include "InnoEngine/graphics/Font.h"
#include "InnoEngine/graphics/Texture2D.h"

#include "InnoEngine/graphics/pipelines/Sprite2DPipeline.h"
#include "InnoEngine/graphics/pipelines/Font2DPipeline.h"
#include "InnoEngine/graphics/pipelines/ImGuiPipeline.h"

#include "InnoEngine/utility/StringArena.h"

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

        Stats LastFrameStats;

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

        RenderCommandBuffer();
        RenderCommandBuffer& operator=( RenderCommandBuffer& other );
       

        void clear();
        const Stats& get_stats() const;
    };

}    // namespace InnoEngine
