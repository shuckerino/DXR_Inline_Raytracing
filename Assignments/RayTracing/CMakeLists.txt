include("../../CreateApp.cmake")
set(SOURCES "./src/main.cpp" 
                                "./src/SceneGraphViewerApp.cpp" 
								"./src/AABB.cpp" 
								"./src/Scene.cpp" 
								"./src/SceneFactory.cpp" 
								"./src/TriangleMeshD3D12.cpp" 
								"./src/Texture2DD3D12.cpp" 
								"./src/ConstantBufferD3D12.cpp" 
								"./src/RayTracingUtils.cpp" 
								"./include/RayTracingUtils.hpp" 
								"./include/AABB.hpp" 
								"./include/Scene.hpp" 
								"./include/SceneFactory.hpp" 
								"./include/TriangleMeshD3D12.hpp" 								
								"./include/Texture2DD3D12.hpp" 								
								"./include/SceneGraphViewerApp.hpp"
								"./include/ConstantBufferD3D12.hpp")

set(SHADERS "./shaders/RayTracing.hlsl")
create_app(RayTracing "${SOURCES}" "${SHADERS}")
find_package(assimp CONFIG REQUIRED)
target_link_libraries(RayTracing PRIVATE assimp::assimp)
