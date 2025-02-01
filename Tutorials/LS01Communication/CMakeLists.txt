include("../../CreateApp.cmake")
set(SOURCES "./src/Communication.cpp")
set(SHADERS "./shaders/Communication.hlsl")
create_app(LS01Communication "${SOURCES}" "${SHADERS}")

