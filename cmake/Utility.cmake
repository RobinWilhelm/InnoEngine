# set project structure to be the same as the folder structure
function(mirror_source_structure)    
    foreach(arg IN LISTS ARGN)
        get_filename_component(_source_path "${arg}" PATH)
        string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
        string(REPLACE "/" "\\" _group_path "${_group_path}")
        source_group("${_group_path}" FILES "${arg}")
    endforeach()
endfunction()