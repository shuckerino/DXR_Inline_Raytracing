#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/types.hpp>
#include <imgui.h>
#include <iostream>
using namespace gims;


class BasicFramework : public DX12App
{
private:
  struct UiData
  {
    f32v3 m_backgroundColor = {0.0f, 0.0f, 0.0f};    
  };

  UiData m_uiData;

public:
  BasicFramework(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {
  }

  virtual void onDraw()
  {
    const auto commandList = getCommandList();
    const auto rtvHandle   = getRTVHandle();
    const auto dsvHandle   = getDSVHandle();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    
    f32v4 clearColor = {m_uiData.m_backgroundColor[0], m_uiData.m_backgroundColor[1], m_uiData.m_backgroundColor[2], 1.0f};
    commandList->ClearRenderTargetView(rtvHandle, &clearColor.x, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
  }

  virtual void onDrawUI()
  {
    ImGui::Begin("Information");
    ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
    ImGui::End();    
    ImGui::Begin("Configuration");
    ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor[0]);
    ImGui::End();        
  }


};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 01 Basic Frame Work";
  config.useVSync = false;  
  try
  {
    BasicFramework app(config);
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
