# Requires shadercross CLI installed from SDL_shadercross
sourcedir="../src/innoengine/InnoEngine/Shader"
assetdir="../sandbox_environment/assets"

for filepath in "${sourcedir}"/*.vert.hlsl; do
    if [ -f "$filepath" ]; then
		filename="${filepath##*/}"	
		echo "compiling $filepath"
        echo "$filename"
        shadercross "$filepath" -o "${assetdir}/shaders/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filepath" -o "${assetdir}/shaders/MSL/${filename/.hlsl/.msl}"
        shadercross "$filepath" -o "${assetdir}/shaders/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filepath in "${sourcedir}"/*.frag.hlsl; do
    if [ -f "$filepath" ]; then
		filename="${filepath##*/}"		
		echo "compiling $filepath"
        shadercross "$filepath" -o "${assetdir}/shaders/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filepath" -o "${assetdir}/shaders/MSL/${filename/.hlsl/.msl}"
        shadercross "$filepath" -o "${assetdir}/shaders/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filepath in "${sourcedir}"/*.comp.hlsl; do
    if [ -f "$filepath" ]; then
		filename="${filepath##*/}"
		echo "compiling $filepath"
        shadercross "$filepath" -o "${assetdir}/shaders/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filepath" -o "${assetdir}/shaders/MSL/${filename/.hlsl/.msl}"
        shadercross "$filepath" -o "${assetdir}/shaders/DXIL/${filename/.hlsl/.dxil}"
    fi
done

echo "Done"
read