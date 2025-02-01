include("../../CreateApp.cmake")
set(SOURCES "./src/TwoTrianglesApp.cpp")
set(SHADERS "./shaders/TwoTriangles.hlsl")
create_app(T03TwoTriangles "${SOURCES}" "${SHADERS}")



