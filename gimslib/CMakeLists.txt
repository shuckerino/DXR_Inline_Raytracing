
# Defintions
add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)
add_definitions(-DNOMINMAX)
add_definitions(-DNODRAWTEXT)
add_definitions(-DNOBITMAP)
add_definitions(-DNOMCX)
add_definitions(-DNOSERVICE)
add_definitions(-DNOHELP)
add_definitions(-DWIN32_LEAN_AND_MEAN)

set(gimslib_PROJECT_SOURCE 
						"./src/gimslib/d3d/DX12App.cpp"												
						"./src/gimslib/d3d/HLSLCompiler.cpp"
						"./src/gimslib/d3d/DX12Util.cpp"
						"./src/gimslib/d3d/UploadHelper.cpp"
						"./src/gimslib/d3d/impl/ImGUIAdapter.cpp"
						"./src/gimslib/d3d/impl/ImGUIAdapter.hpp"
						"./src/gimslib/d3d/impl/SwapChainAdapter.cpp"
						"./src/gimslib/d3d/impl/SwapChainAdapter.hpp"						
						"./src/gimslib/dbg/HrException.cpp"
						"./src/gimslib/io/CograBinaryMeshFile.cpp"
						"./src/gimslib/ui/ExaminerController.cpp"
						"./src/gimslib/ui/PitchShiftControl.cpp"
						"./src/gimslib/ui/TrackballControl.cpp"											
						"./src/gimslib/sys/Event.cpp"
						"./src/gimslib/contrib/imgui/imgui_impl_dx12.cpp"
						"./src/gimslib/contrib/imgui/imgui_impl_win32.cpp"
						"./src/gimslib/contrib/stb/stb_image.cpp"
                        "./include/gimslib/types.hpp"
						"./include/gimslib/d3d/DX12App.hpp"												
						"./include/gimslib/d3d/HLSLCompiler.hpp"
						"./include/gimslib/d3d/DX12Util.hpp"
						"./include/gimslib/d3d/UploadHelper.hpp"
						"./include/gimslib/dbg/HrException.hpp"
						"./include/gimslib/io/CograBinaryMeshFile.hpp"
						"./include/gimslib/ui/ExaminerController.hpp"
						"./include/gimslib/ui/PitchShiftControl.hpp"
						"./include/gimslib/ui/TrackballControl.hpp"											
						"./include/gimslib/sys/Event.hpp"						
						"./include/gimslib/contrib/imgui/imgui_impl_dx12.h"
						"./include/gimslib/contrib/imgui/imgui_impl_win32.h"
						"./include/gimslib/contrib/stb/stb_image.h"
						
   )

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/" FILES ${gimslib_PROJECT_SOURCE})

add_library(gimslib ${gimslib_PROJECT_SOURCE})


# Includes
set(gimslib_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(gimslib PUBLIC "$<BUILD_INTERFACE:${gimslib_INCLUDE_DIR}>" "$<INSTALL_INTERFACE:./${CMAKE_INSTALL_INCLUDEDIR}>")

# Find dependencies:
set(gimslib_DEPENDENCIES_CONFIGURED    
	glm
	imgui
    CACHE STRING "")

foreach(DEPENDENCY ${gimslib_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies:
target_link_libraries(gimslib PRIVATE glm::glm imgui::imgui Microsoft.Direct3D.D3D12 Microsoft.Direct3D.DXC d3d12 dxcompiler dxgi.lib dxguid.lib)



set_target_properties (gimslib PROPERTIES FOLDER gimslib)