#pragma once
#include <d3d12.h>
#include <gimslib/types.hpp>
#include <wrl.h>

using Microsoft::WRL::ComPtr;
using namespace gims;
namespace gims
{
namespace impl
{
class ImGUIAdapter
{
public:
  ImGUIAdapter(HWND hwnd, const ComPtr<ID3D12Device>& device, ui32 frameCount, DXGI_FORMAT renderTargetFormat);

  ~ImGUIAdapter();

  void newFrame();
  void render();
  void addToCommadList(const ComPtr<ID3D12GraphicsCommandList>& commandList);

private:
  ComPtr<ID3D12DescriptorHeap> m_imguiSRVDescriptorHeap;
};
} // namespace impl
} // namespace gims