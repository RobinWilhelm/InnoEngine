
set(IE_PCH "src/iepch.h")
set(IE_PCH_SOURCE "src/iepch.cpp") 

set(InnoEngine_Sources 
	"${IE_PCH}"	
	"${IE_PCH_SOURCE}"
	"src/EntryPoint.cpp"  
	"src/Application.h"
	"src/Application.cpp"
	"src/Renderer.h"
	"src/Renderer.cpp"
	"src/Sprite2DPipeline.h"
	"src/Sprite2DPipeline.cpp"
	"src/Sprite.h"
	"src/Sprite.cpp"
	"src/Scene.h"
	"src/Scene.cpp"
	"src/GPUPipeline.h"
	"src/GPUPipeline.cpp"
	"src/RenderCommandBuffer.h"
	"src/RenderCommandBuffer.cpp"
	"src/Window.h"
	"src/Window.cpp"
	"src/AssetManager.h"
	"src/AssetManager.cpp"
	"src/AssetRepository.h"
	"src/Shader.h"
	"src/Shader.cpp"
	"src/ObjectPool.h"
	"src/Asset.h"
	"src/Asset.cpp"
	"src/AssetView.h"
	"src/CoreAPI.h"
	"src/OrthographicCamera.h"
	"src/OrthographicCamera.cpp"
	"src/Log.h"
	"src/Assert.h"
)

set(InnoEngine_Shader_Sources  
	"src/Shader/TextureXColor.frag.hlsl"
	"src/Shader/SpriteBatch.vert.hlsl"
	"src/Shader/GPUPipelineBase.vertincl.hlsl"
)


PREPEND(InnoEngine_Sources)
PREPEND(InnoEngine_Shader_Sources)

set(InnoEngine_Sources ${InnoEngine_Sources}  PARENT_SCOPE)
set(InnoEngine_Shader_Sources ${InnoEngine_Shader_Sources}  PARENT_SCOPE)

PREPEND(IE_PCH)
set(IE_PCH ${IE_PCH}  PARENT_SCOPE)
PREPEND(IE_PCH_SOURCE)
set(IE_PCH_SOURCE ${IE_PCH_SOURCE}  PARENT_SCOPE)