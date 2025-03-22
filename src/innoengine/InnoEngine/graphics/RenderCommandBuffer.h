#pragma once
#include "InnoEngine/graphics/Font.h"
#include "InnoEngine/graphics/Texture2D.h"

#include "InnoEngine/graphics/pipelines/Sprite2DPipeline.h"
#include "InnoEngine/graphics/pipelines/Font2DPipeline.h"
#include "InnoEngine/graphics/pipelines/ImGuiPipeline.h"
#include "InnoEngine/graphics/pipelines/Primitive2DPipeline.h"

#include "InnoEngine/utility/StringArena.h"
#include "InnoEngine/graphics/Viewport.h"

#include "DirectXMath.h"
#include "Directxtk/SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include <vector>

namespace InnoEngine
{
    struct RenderCommandBuffer
    {
        bool                        Clear;
        DXSM::Color                 ClearColor;
        std::vector<DXSM::Matrix>   ViewProjectionMatrices;
        std::vector<Viewport>       Viewports;
        std::vector<Ref<Texture2D>> RenderTargets;

        TextureList TextureRegister;    // hold a reference to all texture objects we are going to use this frame

        SpriteCommandBuffer SpriteRenderCommands;
        QuadCommandBuffer   QuadRenderCommands;
        LineCommandBuffer   LineRenderCommands;
        CircleCommandBuffer CircleRenderCommands;

        StringArena       StringBuffer;    // arena like container to hold a copy of all strings we are going to render this frame
        FontList          FontRegister;    // hold a reference to all font objects we are going to use this frame
        FontCommandBuffer FontRenderCommands;

        ImGuiPipeline::CommandData ImGuiCommandBuffer;

        RenderCommandBuffer();
        RenderCommandBuffer& operator=( RenderCommandBuffer& other );

        void clear();
    };

}    // namespace InnoEngine
