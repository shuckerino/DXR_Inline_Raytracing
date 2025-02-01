include("../../CreateApp.cmake")
set(SOURCES "./src/main.cpp" 
                                "./src/SceneGraphViewerApp.cpp" 
								"./src/AABB.cpp" 
								"./src/Scene.cpp" 
								"./src/SceneFactory.cpp" 
								"./src/TriangleMeshD3D12.cpp" 
								"./src/Texture2DD3D12.cpp" 
								"./src/ConstantBufferD3D12.cpp" 
								"./include/AABB.hpp" 
								"./include/Scene.hpp" 
								"./include/SceneFactory.hpp" 
								"./include/TriangleMeshD3D12.hpp" 								
								"./include/Texture2DD3D12.hpp" 								
								"./include/SceneGraphViewerApp.hpp"
								"./include/ConstantBufferD3D12.hpp")

set(SHADERS "./shaders/TriangleMesh.hlsl")
create_app(A1SceneGraphViewer "${SOURCES}" "${SHADERS}")
find_package(assimp CONFIG REQUIRED)
target_link_libraries(A1SceneGraphViewer PRIVATE assimp::assimp)
