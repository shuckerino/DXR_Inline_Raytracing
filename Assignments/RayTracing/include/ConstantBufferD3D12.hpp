#include <d3d12.h>
#include <gimslib/types.hpp>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

namespace gims
{
/// <summary>
/// A class that holds a constant buffer on the GPU.
/// </summary>
class ConstantBufferD3D12
{
public:
  /// <summary>
  /// Creates an empty constant buffer.
  /// </summary>
  ConstantBufferD3D12();

  /// <summary>
  /// Creates an uninitialized constant buffers of the given size.
  /// </summary>
  /// <param name="sizeInBytes">Size in bytes.</param>
  /// <param name="device">Device on which the constant buffer should be allocated.</param>
  ConstantBufferD3D12(size_t sizeInBytes, const ComPtr<ID3D12Device>& device);

  /// <summary>
  /// Upload the CPU data in data to the GPU.
  /// </summary>
  /// <typeparam name="T">Struct with data that should be uploaded to the GPU.</typeparam>
  /// <param name="data">CPU data to upload.s</param>
  /// <param name="device">Device to which we wish to upload.</param>
  template<class T>
  ConstantBufferD3D12(const T& data, const ComPtr<ID3D12Device>& device)
      : ConstantBufferD3D12(sizeof(T), device)
  {
    this->upload(&data);
  }

  const ComPtr<ID3D12Resource>& getResource() const;
  /// <summary>
  /// Uploads the provided data to the GPU buffer.
  /// </summary>
  /// <param name="data">Data to upload. Size must match the requested size. </param>
  void upload(void const* const data);

  ConstantBufferD3D12(const ConstantBufferD3D12& other)                = default;
  ConstantBufferD3D12(ConstantBufferD3D12&& other) noexcept            = default;
  ConstantBufferD3D12& operator=(const ConstantBufferD3D12& other)     = default;
  ConstantBufferD3D12& operator=(ConstantBufferD3D12&& other) noexcept = default;

private:
  ComPtr<ID3D12Resource> m_constantBuffer; //! The constant buffer on the GPU.
  size_t                 m_sizeInBytes;    //! The size of the constant buffer in bytes.
};
} // namespace gims
