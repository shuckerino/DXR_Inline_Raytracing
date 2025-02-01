#include "SwapChainAdapter.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/dbg/HrException.hpp>

namespace
{
ComPtr<ID3D12Fence> createD3DFence(const ComPtr<ID3D12Device>& device)
{
  ComPtr<ID3D12Fence> result;
  throwIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&result)));
  return result;
}

ComPtr<IDXGISwapChain4> createSwapChain(ui32 width, ui32 height, ui32 frameCount, DXGI_FORMAT renderTargetFormat,
                                        const ComPtr<IDXGIFactory4>&      factory,
                                        const ComPtr<ID3D12CommandQueue>& commandQueue, HWND hwnd)
{
  ComPtr<IDXGISwapChain4> result;
  DXGI_SWAP_CHAIN_DESC1   swapChainDesc = {};
  swapChainDesc.BufferCount             = frameCount;
  swapChainDesc.Width                   = width;
  swapChainDesc.Height                  = height;
  swapChainDesc.Format                  = renderTargetFormat;
  swapChainDesc.BufferUsage             = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect              = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count        = 1;
  swapChainDesc.Flags                   = 0u;

  ComPtr<IDXGISwapChain1> swapChain;
  throwIfFailed(
      factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain));
  throwIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
  throwIfFailed(swapChain.As(&result));
  return result;
}

std::vector<ComPtr<ID3D12Resource>> createRenderTargets(const ComPtr<ID3D12Device>&         device,
                                                        const ComPtr<ID3D12DescriptorHeap>& rtvHeap,
                                                        const ComPtr<IDXGISwapChain3>& swapChain, const ui32 frameCount,
                                                        ui32 rtvDescriptorSize)
{
  std::vector<ComPtr<ID3D12Resource>> result(frameCount);

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
  for (ui32 i = 0; i < frameCount; i++)
  {
    throwIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&result[i])));
    device->CreateRenderTargetView(result[i].Get(), nullptr, rtvHandle);
    rtvHandle.Offset(1, rtvDescriptorSize);
  }

  return result;
}

ComPtr<ID3D12Resource> createDepthStencil(const ComPtr<ID3D12Device>&         device,
                                          const ComPtr<ID3D12DescriptorHeap>& dsvHeap, ui32 width, ui32 height,
                                          DXGI_FORMAT depthBufferFormat)
{
  ComPtr<ID3D12Resource>        result;
  D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
  depthStencilDesc.Format                        = depthBufferFormat;
  depthStencilDesc.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;
  depthStencilDesc.Flags                         = D3D12_DSV_FLAG_NONE;

  D3D12_CLEAR_VALUE depthOptimizedClearValue    = {};
  depthOptimizedClearValue.Format               = depthBufferFormat;
  depthOptimizedClearValue.DepthStencil.Depth   = 1.0f;
  depthOptimizedClearValue.DepthStencil.Stencil = 0;

  const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(D3D12_HEAP_TYPE_DEFAULT);
  const CD3DX12_RESOURCE_DESC   depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
      depthBufferFormat, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

  throwIfFailed(device->CreateCommittedResource(&depthStencilHeapProps, D3D12_HEAP_FLAG_NONE, &depthStencilTextureDesc,
                                                D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue,
                                                IID_PPV_ARGS(&result)));

  device->CreateDepthStencilView(result.Get(), &depthStencilDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
  return result;
}

ComPtr<ID3D12DescriptorHeap> createRTVDescriptorHeap(const ComPtr<ID3D12Device>& device, ui32 frameCount)
{
  ComPtr<ID3D12DescriptorHeap> result;

  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.NumDescriptors             = frameCount;
  desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  throwIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&result)));

  return result;
}

ComPtr<ID3D12DescriptorHeap> createDSVDescriptorHeap(const ComPtr<ID3D12Device>& device)
{
  ComPtr<ID3D12DescriptorHeap> result;

  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.NumDescriptors             = 1;
  desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  throwIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&result)));

  return result;
}



} // namespace

namespace gims
{
namespace impl
{
SwapChainAdapter::SwapChainAdapter(const HWND hwnd, const ComPtr<IDXGIFactory4>& factory,
                                   const ComPtr<ID3D12CommandQueue>& commandQueue, ui32 frameCount)
    : m_hwnd(hwnd)
    , m_factory(factory)
    , m_device(DX12Util::getDevice(commandQueue))
    , m_commandQueue(commandQueue)
    , m_rtvDescriptorHeap(createRTVDescriptorHeap(m_device, frameCount))
    , m_dsvDescriptorHeap(createDSVDescriptorHeap(m_device))
    , m_frameCount(frameCount)
    , m_frameIndex(0)
    , m_fenceValues(frameCount, 0)
    , m_viewport({})
    , m_rectScissor({})
    , m_fence(createD3DFence(m_device))
    , m_fenceEvent()

{
  m_fenceValues[m_frameIndex]++;
}

SwapChainAdapter::~SwapChainAdapter()
{
  waitForGPU();  
}

void SwapChainAdapter::waitForGPU()
{
  throwIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));
  throwIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent.getHandle()));
  m_fenceEvent.wait();
  m_fenceValues[m_frameIndex]++;
}

void SwapChainAdapter::nextFrame(bool useVSync)
{
  HRESULT hr;
  hr = m_swapChain->Present(useVSync ? 1 : 0, 0);

  gims::DX12Util::throwOnDeviceLost(m_device, hr, "presenting");
  const auto currentFenceValue = m_fenceValues[m_frameIndex];
  throwIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));
  m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
  DX12Util::waitForFence(m_fence, m_fenceValues[m_frameIndex], m_fenceEvent.getHandle());
  m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

const ComPtr<ID3D12Resource>& SwapChainAdapter::getDepthStencil() const
{
  return m_depthStencil;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SwapChainAdapter::getRTVHandle()
{
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                                          getFrameIndex(),
                                          m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
  return rtvHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SwapChainAdapter::getDSVHandle()
{
  CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
  return dsvHandle;
}


ui32 SwapChainAdapter::getFrameIndex() const
{
  return m_frameIndex;
}

ui32 SwapChainAdapter::getWidth() const
{
  return static_cast<ui32>(m_viewport.Width);
}

ui32 SwapChainAdapter::getHeight() const
{
  return static_cast<ui32>(m_viewport.Height);
}

const D3D12_VIEWPORT& SwapChainAdapter::getViewport() const
{
  return m_viewport;
}

const D3D12_RECT& SwapChainAdapter::getRectScissor() const
{
  return m_rectScissor;
}

const ComPtr<ID3D12Resource>& SwapChainAdapter::getRenderTarget() const
{
  return m_renderTargets[m_frameIndex];
}

bool SwapChainAdapter::resize(ui32 width, ui32 height, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthBufferFormat)
{
  if (m_swapChain)
  {
    if ((width == getWidth() && height == getHeight()))
    {
      return false;
    }

    waitForGPU();
    for (ui32 i = 0; i < m_frameCount; i++)
    {
      m_renderTargets[i].Reset();
      m_fenceValues[i] = m_fenceValues[m_frameIndex];
    }
    const auto hr = m_swapChain->ResizeBuffers(m_frameCount, width, height, renderTargetFormat, 0u);
    gims::DX12Util::throwOnDeviceLost(m_device, hr, "resizing");
  }
  else
  {
    m_swapChain = createSwapChain(width, height, m_frameCount, renderTargetFormat, m_factory, m_commandQueue, m_hwnd);
  }
  m_frameIndex    = m_swapChain->GetCurrentBackBufferIndex();
  m_renderTargets = createRenderTargets(m_device, m_rtvDescriptorHeap, m_swapChain, m_frameCount,
                                        m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
  m_depthStencil  = createDepthStencil(m_device, m_dsvDescriptorHeap, width, height, depthBufferFormat);
  m_viewport      = {0.0f, 0.0f, static_cast<f32>(width), static_cast<f32>(height), 0.0f, 1.0f};
  m_rectScissor   = {0, 0, static_cast<i32>(width), static_cast<i32>(height)};
  return true;
}

} // namespace impl
} // namespace gims