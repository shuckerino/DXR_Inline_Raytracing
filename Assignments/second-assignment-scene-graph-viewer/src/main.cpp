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
    // const std::filesystem::path path = "../../../data/ww2_cityscene_-_carentan_inspired/scene.gltf";
    // const std::filesystem::path path = "../../../data/sponza/sponza.obj";
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
