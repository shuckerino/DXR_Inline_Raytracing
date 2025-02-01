#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h>
#include <gimslib/sys/Event.hpp>
#include <gimslib/types.hpp>
#include <vector>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
using namespace gims;

namespace gims
{
namespace impl
{
class SwapChainAdapter
{
public:
  SwapChainAdapter(const HWND hwnd, const ComPtr<IDXGIFactory4>& factory,
                   const ComPtr<ID3D12CommandQueue>& commandQueue, ui32 frameCount);

  ~SwapChainAdapter();

  bool resize(ui32 width, ui32 height, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthBufferFormat);
  void waitForGPU();
  void nextFrame(bool useVSync);

  ui32                          getFrameIndex() const;
  ui32                          getWidth() const;
  ui32                          getHeight() const;
  const ComPtr<ID3D12Resource>& getRenderTarget() const;
  const ComPtr<ID3D12Resource>& getDepthStencil() const;
  CD3DX12_CPU_DESCRIPTOR_HANDLE getRTVHandle();
  CD3DX12_CPU_DESCRIPTOR_HANDLE getDSVHandle();
  const D3D12_VIEWPORT&         getViewport() const;
  const D3D12_RECT&             getRectScissor() const;

private:
  const ComPtr<IDXGIFactory4>&        m_factory;
  const HWND                          m_hwnd;
  const ComPtr<ID3D12Device>          m_device;
  const ComPtr<ID3D12CommandQueue>&   m_commandQueue;
  ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
  ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
  ui32                                m_frameCount;
  ui32                                m_frameIndex;
  std::vector<ui64>                   m_fenceValues;
  D3D12_VIEWPORT                      m_viewport;
  D3D12_RECT                          m_rectScissor;
  ComPtr<ID3D12Fence>                 m_fence;
  Event                               m_fenceEvent;
  ComPtr<IDXGISwapChain4>             m_swapChain;
  std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
  ComPtr<ID3D12Resource>              m_depthStencil;
};
} // namespace impl
} // namespace gims