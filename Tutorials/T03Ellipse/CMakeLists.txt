include("../../CreateApp.cmake")
set(SOURCES "./src/EllipseApp.cpp")
set(SHADERS "./shaders/Ellipse.hlsl")
create_app(T03Ellipse "${SOURCES}" "${SHADERS}")

