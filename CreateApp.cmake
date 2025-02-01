function(create_app target_name cpphpp_files hlsl_files)
    add_definitions(-D_UNICODE)
    add_definitions(-DUNICODE)
    add_definitions(-DNOMINMAX)
    add_definitions(-DNODRAWTEXT)
    add_definitions(-DNOBITMAP)
    add_definitions(-DNOMCX)
    add_definitions(-DNOSERVICE)
    add_definitions(-DNOHELP)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    
    source_group("Shaders" FILES ${hlsl_files})
    set_source_files_properties(${hlsl_files} PROPERTIES VS_TOOL_OVERRIDE "None")
    add_executable(${target_name} ${cpphpp_files} ${hlsl_files})


    set(INCLUDE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(${target_name} PRIVATE "${INCLUDE_DIR}" "$<INSTALL_INTERFACE:./${CMAKE_INSTALL_INCLUDEDIR}>")
    
    # Find dependencies:
    set(DEPENDENCIES_CONFIGURED glm)
        
    foreach(DEPENDENCY ${DEPENDENCIES_CONFIGURED})
      find_package(${DEPENDENCY} CONFIG REQUIRED)
    endforeach()
        
    target_link_libraries(${target_name} PRIVATE glm::glm gimslib Microsoft.Direct3D.D3D12 Microsoft.Direct3D.DXC d3d12 dxcompiler dxgi.lib dxguid.lib)
endfunction()