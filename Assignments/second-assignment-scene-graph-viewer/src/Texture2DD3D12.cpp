#include "Texture2DD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>

using namespace gims;
namespace
{

ComPtr<ID3D12Resource> createTexture(void const* const data, ui32 textureWidth, ui32 textureHeight,
                                     const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  ComPtr<ID3D12Resource> textureResource;
  (void)data;
  (void)textureWidth;
  (void)textureHeight;
  (void)device;
  (void)commandQueue;
  // Assignment 8
  return textureResource;
}
} // namespace

namespace gims
{
Texture2DD3D12::Texture2DD3D12(std::filesystem::path path, const ComPtr<ID3D12Device>& device,
                               const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  const auto fileName     = path.generic_string();
  const auto fileNameCStr = fileName.c_str();
  i32        textureWidth, textureHeight, textureComp;

  std::unique_ptr<ui8, void (*)(void*)> image(stbi_load(fileNameCStr, &textureWidth, &textureHeight, &textureComp, 4),
                                              &stbi_image_free);
  if (image.get() == nullptr)
  {
    throw std::exception("Error loading texture.");
  }

  m_textureResource = createTexture(image.get(), textureWidth, textureHeight, device, commandQueue);
}
Texture2DD3D12::Texture2DD3D12(ui8v4 const* const data, ui32 width, ui32 height, const ComPtr<ID3D12Device>& device,
                               const ComPtr<ID3D12CommandQueue>& commandQueue)

{
  m_textureResource = createTexture(data, width, height, device, commandQueue);
}

void Texture2DD3D12::addToDescriptorHeap(const ComPtr<ID3D12Device>&         device,
                                         const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, i32 descriptorIndex) const
{
  (void)device;
  (void)descriptorHeap;
  (void)descriptorIndex;
  // Assignment 8
}
} // namespace gims
