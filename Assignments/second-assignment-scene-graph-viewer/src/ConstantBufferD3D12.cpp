#include "ConstantBufferD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
namespace gims
{
ConstantBufferD3D12::ConstantBufferD3D12()
    : m_sizeInBytes(0)
{
}
ConstantBufferD3D12::ConstantBufferD3D12(size_t sizeInBytes, const ComPtr<ID3D12Device>& device)
    : m_sizeInBytes(sizeInBytes)
{
  // Assignment 2
  static const CD3DX12_HEAP_PROPERTIES uploadHeapProperties      = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  static const CD3DX12_RESOURCE_DESC   constantBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
  device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDescription,
                                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer));
}
const ComPtr<ID3D12Resource>& ConstantBufferD3D12::getResource() const
{
  return m_constantBuffer;
}
void ConstantBufferD3D12::upload(void const* const data)
{
  // Assignment 2
  if (m_sizeInBytes == 0)
  {
    return;
  }
  void* p;
  m_constantBuffer->Map(0, nullptr, &p);
  ::memcpy(p, data, m_sizeInBytes);
  m_constantBuffer->Unmap(0, nullptr);
}
} // namespace gims
