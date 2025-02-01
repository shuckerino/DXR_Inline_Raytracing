include("../../CreateApp.cmake")
set(SOURCES "./src/RayTracingRenderer.cpp" "./include/RayTracingRenderer.hpp")
set(SHADERS "./shaders/RayTracing.hlsl")
create_app(T17TriangleRayTracing "${SOURCES}" "${SHADERS}")

