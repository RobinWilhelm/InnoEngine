#include "imgui.h"

#include "Layer.h"

#include <optional>
#include <memory>

namespace InnoEngine
{
    class GPURenderer;
    class ImGuiPipeline;

    class DebugUI : public Layer
    {
        DebugUI() = default;

    public:
        [[nodiscard]]
        static auto create( GPURenderer* renderer ) -> std::optional<std::unique_ptr<DebugUI>>;

        // Inherited via Layer
        void update( double deltaTime ) override;
        void render( float interpFactor, GPURenderer* pRenderer ) override;
        bool handle_event( const SDL_Event& pEvent ) override;

    private:
        void imgui_begin_frame();
        void imgui_end_frame();

    private:
        std::shared_ptr<ImGuiPipeline> m_pipeline = nullptr;
    };
}    // namespace InnoEngine
