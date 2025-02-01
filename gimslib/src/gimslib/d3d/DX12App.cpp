#include "impl/ImGUIAdapter.hpp"
#include "impl/SwapChainAdapter.hpp"
#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <imgui.h>
#include <iostream>
#include <string>

extern "C"
{
  __declspec(dllexport) extern const UINT D3D12SDKVersion = 613;
}
extern "C"
{
  __declspec(dllexport) extern const char* D3D12SDKPath = ".\\";
}

using namespace gims;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == WM_CREATE)
  {
    LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
  }
  else
  {
    DX12App* pAppPointer = reinterpret_cast<DX12App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (pAppPointer != nullptr)
    {
      return pAppPointer->windowProcHandler(message, wParam, lParam);
    }
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND createWindow(std::wstring title, ui32 width, ui32 height, gims::DX12App* dx12App)
{
  WNDCLASSEX windowClass    = {0};
  windowClass.cbSize        = sizeof(WNDCLASSEX);
  windowClass.style         = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc   = WindowProc;
  windowClass.hInstance     = nullptr;
  windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  windowClass.lpszClassName = L"Gims_Dx12App";
  RegisterClassEx(&windowClass);

  RECT windowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
  AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

  return CreateWindow(windowClass.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                      windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, nullptr,
                      dx12App);
}

ComPtr<ID3D12InfoQueue> createD3D12InfoQueue(bool c_debug, ComPtr<ID3D12Device5>& device)
{
  ComPtr<ID3D12InfoQueue> result = nullptr;
  if (c_debug)
  {
    throwIfFailed(device.As(&result));
    result->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    result->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    result->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

    D3D12_MESSAGE_ID hide[] = {
        D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
        D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
        // Workarounds for debug layer issues on hybrid-graphics systems
        D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
    };
    D3D12_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.NumIDs         = static_cast<ui32>(std::size(hide));
    filter.DenyList.pIDList        = hide;
    result->AddStorageFilterEntries(&filter);
  }
  return result;
}

ComPtr<IDXGIInfoQueue> createDXGIInfoQueue(bool c_debug)
{
  ComPtr<IDXGIInfoQueue> result;
  if (c_debug)
  {
    throwIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&result)));
    result->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    result->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
    result->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);

    DXGI_INFO_QUEUE_MESSAGE_ID hide[] = {
        // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output
        //  on which the swapchain's window resides.
        80};
    DXGI_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.NumIDs        = static_cast<UINT>(std::size(hide));
    filter.DenyList.pIDList       = hide;
    result->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
  }
  return result;
}

ComPtr<IDXGIFactory4> createDXGIFactory(bool enableDebug)
{
  ComPtr<IDXGIFactory4> result;
  ComPtr<ID3D12Debug>   debugInterface = nullptr;
  if (enableDebug)
  {
    throwIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
  }

  ui32 dxgiFactoryFlags = debugInterface ? DXGI_CREATE_FACTORY_DEBUG : 0;

  throwIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&result)));

  return result;
}

ComPtr<IDXGIAdapter1> GetHardwareAdapter(IDXGIFactory1* pFactory, D3D_FEATURE_LEVEL d3d_featureLevel)
{
  ComPtr<IDXGIAdapter1> adapter;
  ComPtr<IDXGIFactory6> factory6;
  if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
  {
    for (ui32 i = 0; SUCCEEDED(
             factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
         i++)
    {
      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);
      auto isSoftware = desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;

      if (isSoftware)
      {
        continue;
      }
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), d3d_featureLevel, _uuidof(ID3D12Device), nullptr)))
      {
        break;
      }
    }
  }

  if (adapter.Get() == nullptr)
  {
    for (ui32 i = 0; SUCCEEDED(pFactory->EnumAdapters1(i, &adapter)); i++)
    {
      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);
      auto isSoftware = desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
      if (isSoftware)
      {
        continue;
      }
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), d3d_featureLevel, _uuidof(ID3D12Device), nullptr)))
      {
        break;
      }
    }
  }
  return adapter;
}

ComPtr<ID3D12Device5> createDevice(bool debug, D3D_FEATURE_LEVEL d3d_featureLevel, const ComPtr<IDXGIFactory4>& factory)
{
  ComPtr<ID3D12Device5> device;

  ComPtr<IDXGIInfoQueue> dxgiInfoQueue   = createDXGIInfoQueue(debug);
  ComPtr<IDXGIAdapter1>  hardwareAdapter = GetHardwareAdapter(factory.Get(), d3d_featureLevel);

  throwIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), d3d_featureLevel, IID_PPV_ARGS(&device)));

  auto d3d12InfoQueue = createD3D12InfoQueue(debug, device);

  return device;
}

ComPtr<ID3D12Device5> createRTDevice(ComPtr<ID3D12Device> device)
{
  ComPtr<ID3D12Device5> rtDevice;

  throwIfFailed(device->QueryInterface(IID_PPV_ARGS(&rtDevice)));

  return rtDevice;
}

ComPtr<ID3D12CommandQueue> createCommandQueue(const ComPtr<ID3D12Device>& device)
{
  ComPtr<ID3D12CommandQueue> result;

  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;

  throwIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&result)));

  return result;
}

std::vector<ComPtr<ID3D12CommandAllocator>> createCommandAllocators(const ComPtr<ID3D12Device>& device,
                                                                    const ui32                  frameCount)
{
  std::vector<ComPtr<ID3D12CommandAllocator>> result(frameCount);
  for (ui32 i = 0; i < frameCount; i++)
  {
    throwIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&result[i])));
  }
  return result;
}

std::vector<ComPtr<ID3D12GraphicsCommandList4>> createCommandLists(
    const std::vector<ComPtr<ID3D12CommandAllocator>>& commandAllocators)
{
  std::vector<ComPtr<ID3D12GraphicsCommandList4>> result(commandAllocators.size());

  for (size_t i = 0; i < result.size(); i++)
  {
    auto device = DX12Util::getDevice(commandAllocators[i]);
    throwIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i].Get(), nullptr,
                                            IID_PPV_ARGS(&result[i])));
    result[i]->Close();
  }
  return result;
}

std::vector<ComPtr<ID3D12GraphicsCommandList4>> createDXRCommandLists(
    const std::vector<ComPtr<ID3D12GraphicsCommandList>>& commandLists)
{
  std::vector<ComPtr<ID3D12GraphicsCommandList4>> result;
  result.resize(commandLists.size());

  for (ui8 i = 0; i < commandLists.size(); i++)
  {
    throwIfFailed(commandLists.at(i).As(&result[i]));
  }

  return result;
}

} // namespace

namespace gims
{
DX12App::DX12App(const DX12AppConfig config)
    : m_config(config)
    , m_hwnd(createWindow(m_config.title, m_config.width, m_config.height, this))
    , m_factory(createDXGIFactory(m_config.debug))
    , m_device(createDevice(m_config.debug, m_config.d3d_featureLevel, m_factory))
    , m_hlslCompiler()
    , m_commandQueue(createCommandQueue(m_device))
    , m_commandAllocators(createCommandAllocators(m_device, m_config.frameCount))
    , m_commandLists(createCommandLists(m_commandAllocators))
    , m_imGUIAdapter(
          std::make_unique<impl::ImGUIAdapter>(m_hwnd, m_device, m_config.frameCount, m_config.renderTargetFormat))
    , m_swapChainAdapter(
          std::make_unique<impl::SwapChainAdapter>(m_hwnd, m_factory, m_commandQueue, m_config.frameCount))
{

  ShowWindow(m_hwnd, SW_SHOWNORMAL);
}

DX12App::~DX12App()
{
}

const DX12AppConfig& DX12App::getDX12AppConfig() const
{
  return m_config;
}

i32 DX12App::run()
{
  MSG msg = {};
  while (true)
  {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
      {
        break;
      }
    }
    if (msg.message == WM_QUIT)
    {
      break;
    }
    onDrawImpl();
  }
  waitForGPU();
  return static_cast<char>(msg.wParam);
}

void DX12App::waitForGPU()
{
  m_swapChainAdapter->waitForGPU();
}

const ComPtr<ID3D12Device5>& DX12App::getDevice() const
{
  return m_device;
}

const ComPtr<ID3D12StateObject>& DX12App::getDXRStateObject() const
{
  return m_dxrStateObject;
}

const ComPtr<ID3D12GraphicsCommandList4>& DX12App::getCommandList() const
{
  return m_commandLists[m_swapChainAdapter->getFrameIndex()];
}

const ComPtr<ID3D12CommandAllocator>& DX12App::getCommandAllocator() const
{
  return m_commandAllocators[m_swapChainAdapter->getFrameIndex()];
}

const ComPtr<ID3D12Resource>& DX12App::getRenderTarget() const
{
  return m_swapChainAdapter->getRenderTarget();
}

const ComPtr<ID3D12Resource>& DX12App::getDepthStencil() const
{
  return m_swapChainAdapter->getRenderTarget();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DX12App::getRTVHandle()
{
  return m_swapChainAdapter->getRTVHandle();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DX12App::getDSVHandle()
{
  return m_swapChainAdapter->getDSVHandle();
}

const ComPtr<ID3D12CommandQueue>& DX12App::getCommandQueue() const
{
  return m_commandQueue;
}

const D3D12_VIEWPORT& DX12App::getViewport() const
{
  return m_swapChainAdapter->getViewport();
}

const D3D12_RECT& DX12App::getRectScissor() const
{
  return m_swapChainAdapter->getRectScissor();
}

ComPtr<IDxcBlob> DX12App::compileShader(const std::filesystem::path& shaderFile, const wchar_t* entryPoint,
                                        const wchar_t* targetProfile)
{
  return m_hlslCompiler.compileShader(shaderFile, targetProfile, entryPoint);
}

void DX12App::setDXRStateObject(CD3DX12_STATE_OBJECT_DESC& stateObjectDescription, ComPtr<ID3D12Device5> device)
{
  throwIfFailed(device->CreateStateObject(stateObjectDescription, IID_PPV_ARGS(&m_dxrStateObject)));
}

void DX12App::onDraw()
{
}

void DX12App::onDrawUI()
{
}

void DX12App::onResize()
{
}

void DX12App::onDrawImpl()
{
  const auto& commandList      = getCommandList();
  const auto& commandAllocator = getCommandAllocator();
  throwIfFailed(commandAllocator->Reset());
  throwIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

  const auto toBackBuffer = CD3DX12_RESOURCE_BARRIER::Transition(getRenderTarget().Get(), D3D12_RESOURCE_STATE_PRESENT,
                                                                 D3D12_RESOURCE_STATE_RENDER_TARGET);

  commandList->ResourceBarrier(1, &toBackBuffer);

  m_imGUIAdapter->newFrame();
  onDrawUI();
  m_imGUIAdapter->render();

  onDraw();

  m_imGUIAdapter->addToCommadList(getCommandList());

  const auto toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      getRenderTarget().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
  commandList->ResourceBarrier(1, &toPresentBarrier);
  throwIfFailed(commandList->Close());

  ID3D12CommandList* ppCommandLists[] = {commandList.Get()};
  getCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
  m_swapChainAdapter->nextFrame(m_config.useVSync);
}

ui32 DX12App::getFrameIndex() const
{
  return m_swapChainAdapter->getFrameIndex();
  ;
}

f32v2 DX12App::getNormalizedMouseCoordinates() const
{
  const auto mouseIntegerPos = ImGui::GetMousePos();
  return f32v2((mouseIntegerPos.x / getWidth()) * 2.0f - 1.0f, -((mouseIntegerPos.y / getHeight()) * 2.0f - 1.0f));
}

ui32 DX12App::getWidth() const
{
  return m_swapChainAdapter->getWidth();
  ;
}
ui32 DX12App::getHeight() const
{
  return m_swapChainAdapter->getHeight();
}

LRESULT DX12App::windowProcHandler(UINT message, WPARAM wParam, LPARAM lParam)
{
  ImGui_ImplWin32_WndProcHandler(m_hwnd, message, wParam, lParam);
  switch (message)
  {
    case WM_SIZE:
    {
      if (wParam == SIZE_MINIMIZED)
      {
        if (!m_windowState.minimized)
        {
          m_windowState.minimized = true;
          m_windowState.suspend   = true;
        }
      }
      else if (m_windowState.minimized)
      {
        m_windowState.minimized = false;
        m_windowState.suspend   = false;
      }
      else if (!m_windowState.sizemove)
      {
        bool resized = m_swapChainAdapter->resize(LOWORD(lParam), HIWORD(lParam), m_config.renderTargetFormat,
                                                  m_config.depthBufferFormat);
        if (resized)
        {
          onResize();
        }
      }
    }
    break;

    case WM_ENTERSIZEMOVE:

    {
      m_windowState.sizemove = true;
    }
    break;

    case WM_EXITSIZEMOVE:
    {
      m_windowState.sizemove = false;
      RECT rc;
      GetClientRect(m_hwnd, &rc);
      bool resized = m_swapChainAdapter->resize(rc.right - rc.left, rc.bottom - rc.top, m_config.renderTargetFormat,
                                                m_config.depthBufferFormat);
      if (resized)
      {
        onResize();
      }
    }
    break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;
  }
  return DefWindowProc(m_hwnd, message, wParam, lParam);
}

} // namespace gims