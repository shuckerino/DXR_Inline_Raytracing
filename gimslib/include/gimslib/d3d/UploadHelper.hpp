#pragma once
#include <d3d12.h>
#include <wrl.h>
namespace gims
{
using Microsoft::WRL::ComPtr;

class UploadHelper
{
public:
  UploadHelper(const ComPtr<ID3D12Device>& device, size_t maxSize);

  ~UploadHelper();

  void uploadBuffer(const void* const src, ComPtr<ID3D12Resource>& dst, size_t size,
                    const ComPtr<ID3D12CommandQueue>& commandQueue);

  void uploadTexture(const void* const imageData, ComPtr<ID3D12Resource> texture, i32 textureWidth, i32 textureHeight,
                     const ComPtr<ID3D12CommandQueue>& commandQueue);

  void uploadDefaultBuffer(const void* const src, ComPtr<ID3D12Resource>& dst, size_t size,
                           const ComPtr<ID3D12CommandQueue>& commandQueue);

  static void uploadConstantBuffer(const void* const src, const ComPtr<ID3D12Resource>& dst, size_t size);

  size_t maxSize() const;


private:
  void executeUploadSync(const ComPtr<ID3D12CommandQueue>& commandQueue);

  const ComPtr<ID3D12Device>        m_device;
  size_t                            m_maxSize;
  ComPtr<ID3D12Resource>            m_uploadBuffer;
  ComPtr<ID3D12CommandAllocator>    m_uploadCommandAllocator;
  ComPtr<ID3D12GraphicsCommandList> m_uploadCommandList;
};

} // namespace gims