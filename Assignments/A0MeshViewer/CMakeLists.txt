include("../../CreateApp.cmake")
set(SOURCES "./src/main.cpp" "./src/TriangleApp.cpp" "./include/TriangleApp.h")
set(SHADERS "./shaders/TriangleMesh.hlsl")
create_app(A0MeshViewer "${SOURCES}" "${SHADERS}")

