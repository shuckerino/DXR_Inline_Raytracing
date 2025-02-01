include("../../CreateApp.cmake")
set(SOURCES "./src/EllipsisApp.cpp")
set(SHADERS "./shaders/EllipsisApp.hlsl")
create_app(T04Ellipsis "${SOURCES}" "${SHADERS}")
