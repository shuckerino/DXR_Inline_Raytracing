#include "ImGUIAdapter.hpp"
#include <gimslib/contrib/imgui/imgui_impl_dx12.h>
#include <gimslib/contrib/imgui/imgui_impl_win32.h>
#include <gimslib/dbg/HrException.hpp>
namespace
{
ComPtr<ID3D12DescriptorHeap> createSRVDescriptorHeap(const ComPtr<ID3D12Device>& device)
{

  ComPtr<ID3D12DescriptorHeap> result;
  D3D12_DESCRIPTOR_HEAP_DESC   desc = {};
  desc.Type                         = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors               = 1;
  desc.NodeMask                     = 0;
  desc.Flags                        = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  throwIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&result)));
  return result;
}
} // namespace

namespace gims
{
namespace impl
{
ImGUIAdapter::ImGUIAdapter(HWND hwnd, const ComPtr<ID3D12Device>& device, ui32 frameCount,
                           DXGI_FORMAT renderTargetFormat)
    : m_imguiSRVDescriptorHeap(createSRVDescriptorHeap(device))
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX12_Init(device.Get(), frameCount, renderTargetFormat, m_imguiSRVDescriptorHeap.Get(),
                      m_imguiSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                      m_imguiSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}
ImGUIAdapter::~ImGUIAdapter()
{
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}
void ImGUIAdapter::newFrame()
{
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}
void ImGUIAdapter::render()
{
  ImGui::Render();
}
void ImGUIAdapter::addToCommadList(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
  commandList->SetDescriptorHeaps(1, m_imguiSRVDescriptorHeap.GetAddressOf());
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}
} // namespace impl
} // namespace gims