include("../../CreateApp.cmake")
set(SOURCES "./src/TriangleApp.cpp")
set(SHADERS "./shaders/Triangle.hlsl")
create_app(T02Triangle "${SOURCES}" "${SHADERS}")

