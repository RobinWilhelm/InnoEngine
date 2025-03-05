#include "iepch.h"
#include "DebugUI.h"

#include "Renderer.h"
#include "Window.h"
#include "ImGuiPipeline.h"

#include "imgui_impl_sdlgpu3.h"
#include "imgui_impl_sdl3.h"

namespace InnoEngine
{
    auto DebugUI::create( GPURenderer* renderer ) -> std::optional<std::unique_ptr<DebugUI>>
    {
        IE_ASSERT( renderer != nullptr );
        std::unique_ptr<DebugUI> debugui     = std::unique_ptr<DebugUI>( new DebugUI );
        auto                     pipelineOpt = renderer->require_pipeline<ImGuiPipeline>();

        if ( pipelineOpt.has_value() == false )
            return std::nullopt;

        debugui->m_pipeline = pipelineOpt.value();
        return debugui;
    }

    void DebugUI::update( double deltaTime )
    {    
        (void)deltaTime;
        std::this_thread::sleep_for(10ms);
    }

    void DebugUI::render( float interpFactor, GPURenderer* renderer )
    {
        (void)interpFactor;
        (void)renderer;
        imgui_begin_frame();

        // draw here
        ImGui::ShowDemoWindow();

        imgui_end_frame();
    }

    bool DebugUI::handle_event( const SDL_Event& pEvent )
    {
        return ImGui_ImplSDL3_ProcessEvent( &pEvent );
    }

    void DebugUI::imgui_begin_frame()
    {
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void DebugUI::imgui_end_frame()
    {
        ImGui::Render();
        m_pipeline->collect(ImGui::GetDrawData());
    }
}    // namespace InnoEngine
