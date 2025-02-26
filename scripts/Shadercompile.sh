# Requires shadercross CLI installed from SDL_shadercross
sourcedir="../src/Shader"
assetdir="../assets"

for filepath in "${sourcedir}"/*.vert.hlsl; do
    if [ -f "$filepath" ]; then
		filename="${filepath##*/}"	
		echo "compiling $filepath"
        shadercross "$filepath" -o "${assetdir}/Shaders/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filepath" -o "${assetdir}/Shaders/MSL/${filename/.hlsl/.msl}"
        shadercross "$filepath" -o "${assetdir}/Shaders/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filepath in "${sourcedir}"/*.frag.hlsl; do
    if [ -f "$filepath" ]; then
		filename="${filepath##*/}"		
		echo "compiling $filepath"
        shadercross "$filepath" -o "${assetdir}/Shaders/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filepath" -o "${assetdir}/Shaders/MSL/${filename/.hlsl/.msl}"
        shadercross "$filepath" -o "${assetdir}/Shaders/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filepath in "${sourcedir}"/*.comp.hlsl; do
    if [ -f "$filepath" ]; then
		filename="${filepath##*/}"
		echo "compiling $filepath"
        shadercross "$filepath" -o "${assetdir}/Shaders/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filepath" -o "${assetdir}/Shaders/MSL/${filename/.hlsl/.msl}"
        shadercross "$filepath" -o "${assetdir}/Shaders/DXIL/${filename/.hlsl/.dxil}"
    fi
done

echo "Done"
read