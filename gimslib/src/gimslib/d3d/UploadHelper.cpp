#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
namespace gims
{

UploadHelper::UploadHelper(const ComPtr<ID3D12Device>& device, size_t maxSize)
    : m_device(device)
    , m_maxSize(maxSize)
{
  throwIfFailed(
      m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_uploadCommandAllocator)));
  throwIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_uploadCommandAllocator.Get(), nullptr,
                                            IID_PPV_ARGS(&m_uploadCommandList)));

  const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  const auto uploadBufferDesc     = CD3DX12_RESOURCE_DESC::Buffer(maxSize);
  throwIfFailed(m_device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                  IID_PPV_ARGS(&m_uploadBuffer)));
}

UploadHelper::~UploadHelper()
{
}

void UploadHelper::uploadBuffer(const void* const src, ComPtr<ID3D12Resource>& dst, size_t size,
                                const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  void* cpuMappedUploadBuffer = nullptr;
  throwIfFailed(m_uploadBuffer->Map(0, nullptr, &cpuMappedUploadBuffer));
  throwIfNullptr(cpuMappedUploadBuffer);
  ::memcpy(cpuMappedUploadBuffer, src, size);
  m_uploadBuffer->Unmap(0, nullptr);
  m_uploadCommandList->CopyBufferRegion(dst.Get(), 0, m_uploadBuffer.Get(), 0, size);

  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(dst.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                      D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
  m_uploadCommandList->ResourceBarrier(1, &barrier);
  m_uploadCommandList->Close();
  executeUploadSync(commandQueue);
}

void UploadHelper::uploadTexture(const void* const imageData, ComPtr<ID3D12Resource> texture, i32 textureWidth,
                                 i32 textureHeight, const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  D3D12_SUBRESOURCE_DATA textureData = {};
  textureData.pData                  = imageData;
  textureData.RowPitch               = textureWidth * 4;
  textureData.SlicePitch             = textureData.RowPitch * textureHeight;
  UpdateSubresources(m_uploadCommandList.Get(), texture.Get(), m_uploadBuffer.Get(), 0, 0, 1, &textureData);
  const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  m_uploadCommandList->ResourceBarrier(1, &barrier);
  m_uploadCommandList->Close();
  executeUploadSync(commandQueue);
}

void UploadHelper::uploadDefaultBuffer(const void* const src, ComPtr<ID3D12Resource>& dst, size_t size,
                                       const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  void* cpuMappedUploadBuffer = nullptr;
  throwIfFailed(m_uploadBuffer->Map(0, nullptr, &cpuMappedUploadBuffer));
  throwIfNullptr(cpuMappedUploadBuffer);
  ::memcpy(cpuMappedUploadBuffer, src, size);
  m_uploadBuffer->Unmap(0, nullptr);

  D3D12_SUBRESOURCE_DATA subResourceData = {};
  subResourceData.pData                  = src;
  subResourceData.RowPitch               = size;
  subResourceData.SlicePitch             = subResourceData.RowPitch;

  const auto barrier0 =
      CD3DX12_RESOURCE_BARRIER::Transition(dst.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  const auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(dst.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                             D3D12_RESOURCE_STATE_GENERIC_READ);

  m_uploadCommandList->ResourceBarrier(1, &barrier0);
  UpdateSubresources<1>(m_uploadCommandList.Get(), dst.Get(), m_uploadBuffer.Get(), 0, 0, 1, &subResourceData);
  m_uploadCommandList->ResourceBarrier(1, &barrier1);

  m_uploadCommandList->Close();
  executeUploadSync(commandQueue);
}

void UploadHelper::uploadConstantBuffer(const void* const src, const ComPtr<ID3D12Resource>& dst, size_t size)
{
  void* mappedConstantBuffer;
  dst->Map(0, nullptr, (void**)&mappedConstantBuffer);
  ::memcpy(mappedConstantBuffer, src, size);
  dst->Unmap(0, nullptr);
}

void UploadHelper::executeUploadSync(const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  ComPtr<ID3D12Fence> uploadFence;
  m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence));

  ID3D12CommandList* commandLists[] = {m_uploadCommandList.Get()};
  commandQueue->ExecuteCommandLists(std::extent<decltype(commandLists)>::value, commandLists);
  commandQueue->Signal(uploadFence.Get(), 1);
  DX12Util::waitForFence(uploadFence, 1);
  m_uploadCommandList->Reset(m_uploadCommandAllocator.Get(), nullptr);
}

size_t UploadHelper::maxSize() const
{
  return m_maxSize;
}

} // namespace gims