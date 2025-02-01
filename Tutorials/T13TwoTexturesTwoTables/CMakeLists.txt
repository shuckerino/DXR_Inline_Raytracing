include("../../CreateApp.cmake")
set(SOURCES "./src/TwoTexturesTwoTablesApp.cpp")
set(SHADERS "./shaders/TwoTextures.hlsl")
create_app(T13TwoTexturesTwoTables "${SOURCES}" "${SHADERS}")

