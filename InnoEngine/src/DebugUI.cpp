#include "iepch.h"
#include "DebugUI.h"

#include "CoreAPI.h"
#include "Application.h"
#include "Renderer.h"
#include "Window.h"
#include "ImGuiPipeline.h"
#include "Profiler.h"

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

        // simulate some work
        std::this_thread::sleep_for(5ms);
    }

    void DebugUI::render( float interpFactor, GPURenderer* renderer )
    {
        (void)interpFactor;
        (void)renderer;
        imgui_begin_frame();

        // draw here
        //ImGui::ShowDemoWindow();

        if(ImGui::Begin("Debug")) {

           ImGui::Text("FPS: %.0f", CoreAPI::get_application()->get_fps());  
           ImGui::Text("Frametime: %.2fms", CoreAPI::get_application()->get_timing(ProfileElements::TotalFrame) * 1000);

           ImGui::Text("Mainthread: %.2fms", CoreAPI::get_application()->get_timing(ProfileElements::MainThreadTotal) * 1000);
           ImGui::Text("Updatethread: %.2fms", CoreAPI::get_application()->get_timing(ProfileElements::UpdateThreadTotal) * 1000);

           ImGui::Text("Update: %.2fms", CoreAPI::get_application()->get_timing(ProfileElements::Update) * 1000);
           ImGui::Text("Render: %.2fms", CoreAPI::get_application()->get_timing(ProfileElements::Render) * 1000);
        }
        ImGui::End();
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
