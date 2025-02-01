#pragma once
#include <d3d12.h>

#include <D3Dcompiler.h>

#include <d3dx12/d3dx12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <gimslib/types.hpp>

#include <filesystem>
#include <gimslib/d3d/HLSLCompiler.hpp>
#include <shellapi.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <wrl.h>

using Microsoft::WRL::ComPtr;
namespace gims
{
struct DX12AppConfig
{
#ifdef _DEBUG
#define TrueIfBuildConfigIsDebug true
#else
#define TrueIfBuildConfigIsDebug false
#endif
  std::wstring      title              = L"Window";                  //! Window title.
  ui32              width              = 640;                        //! Width of the drawing aera.
  ui32              height             = 480;                        //! Height of the drawing aera.
  bool              debug              = TrueIfBuildConfigIsDebug;   //! Create debug context;
  ui32              frameCount         = 3;                          //! Number of swapchain-frames in flight.
  D3D_FEATURE_LEVEL d3d_featureLevel   = D3D_FEATURE_LEVEL_11_0;     //! Features for D3D12.
  DXGI_FORMAT       renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM; //! Format for frames.
  DXGI_FORMAT       depthBufferFormat  = DXGI_FORMAT_D32_FLOAT;      //! Format for depth buffer.
  bool              useVSync           = true;                       //! True, to enable vertical synchronization.
};

namespace impl
{
class ImGUIAdapter;
class SwapChainAdapter;
} // namespace impl

class DX12App
{
public:
  DX12App(const DX12AppConfig createInfo);
  virtual ~DX12App();
  const DX12AppConfig& getDX12AppConfig() const;

  i32  run();
  void waitForGPU();

  virtual void onDraw();
  virtual void onDrawUI();
  virtual void onResize();

  ui32  getFrameIndex() const;
  f32v2 getNormalizedMouseCoordinates() const;
  ui32  getWidth() const;
  ui32  getHeight() const;

  const ComPtr<ID3D12Device5>&              getDevice() const;
  const ComPtr<ID3D12CommandQueue>&         getCommandQueue() const;
  const ComPtr<ID3D12GraphicsCommandList4>& getCommandList() const;
  const ComPtr<ID3D12StateObject>&          getDXRStateObject() const;
  const ComPtr<ID3D12CommandAllocator>&     getCommandAllocator() const;
  const ComPtr<ID3D12Resource>&             getRenderTarget() const;
  const ComPtr<ID3D12Resource>&             getDepthStencil() const;
  CD3DX12_CPU_DESCRIPTOR_HANDLE             getRTVHandle();
  CD3DX12_CPU_DESCRIPTOR_HANDLE             getDSVHandle();
  const D3D12_VIEWPORT&                     getViewport() const;
  const D3D12_RECT&                         getRectScissor() const;

  void setDXRStateObject(CD3DX12_STATE_OBJECT_DESC& stateObjectDescription, ComPtr<ID3D12Device5> device);

  ComPtr<IDxcBlob> compileShader(const std::filesystem::path& shaderFile, const wchar_t* entryPoint,
                                 const wchar_t* targetProfile);

  LRESULT windowProcHandler(UINT message, WPARAM wParam, LPARAM lParam);

private:
  struct WindowState
  {
    bool sizemove  = false;
    bool suspend   = false;
    bool minimized = false;
  };

  DX12AppConfig                                   m_config;
  HWND                                            m_hwnd;
  ComPtr<IDXGIFactory4>                           m_factory;
  ComPtr<ID3D12Device5>                           m_device;
  gims::HLSLCompiler                              m_hlslCompiler;
  ComPtr<ID3D12CommandQueue>                      m_commandQueue;
  std::vector<ComPtr<ID3D12CommandAllocator>>     m_commandAllocators;
  std::vector<ComPtr<ID3D12GraphicsCommandList4>> m_commandLists;
  std::unique_ptr<impl::ImGUIAdapter>             m_imGUIAdapter;
  std::unique_ptr<impl::SwapChainAdapter>         m_swapChainAdapter;
  WindowState                                     m_windowState;
  ComPtr<ID3D12StateObject>                       m_dxrStateObject; // used for ray tracing

  void onDrawImpl();
};

} // namespace gims
