#include "SceneGraphViewerApp.hpp"
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/types.hpp>
#include <iostream>

using namespace gims;

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.useVSync = false;
  config.debug    = true;
  config.title    = L"D3D12 Assimp Viewer";
  try
  {
    const std::filesystem::path path = "../../../data/NobleCraftsman/scene.gltf";
    //const std::filesystem::path path = "../../../data/dragon/scene.gltf";
    //const std::filesystem::path path = "../../../data/shadowScene/two_spheres_shadow_test.glb";
    //const std::filesystem::path path = "../../../data/test/ogerTest.glb";
    //const std::filesystem::path path = "../../../data/chessboard/scene.gltf";
    //const std::filesystem::path path = "../../../data/desk/scene.gltf";
    //const std::filesystem::path path = "../../../data/statue/scene.gltf";
    //const std::filesystem::path path = "../../../data/greek_building/scene.gltf";
    //const std::filesystem::path path = "../../../data/sponza/glTF/Sponza.gltf";
    //const std::filesystem::path path = "../../../data/sponzaV2/scene.gltf";
    
    SceneGraphViewerApp app(config, path);
    app.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
  }
  if (config.debug)
  {
    DX12Util::reportLiveObjects();
  }

  return 0;
}
