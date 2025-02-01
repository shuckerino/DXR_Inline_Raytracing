#include "TriangleMeshD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>


namespace gims
{
const std::vector<D3D12_INPUT_ELEMENT_DESC> TriangleMeshD3D12::m_inputElementDescs = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

TriangleMeshD3D12::TriangleMeshD3D12(f32v3 const* const positions, f32v3 const* const normals,
                                     f32v3 const* const textureCoordinates, ui32 nVertices,
                                     ui32v3 const* const indexBuffer, ui32 nIndices, ui32 materialIndex,
                                     const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue)
    : m_nIndices(nIndices)
    , m_vertexBufferSize(static_cast<ui32>(nVertices * sizeof(Vertex)))
    , m_indexBufferSize(static_cast<ui32>(nIndices * sizeof(ui32)))
    , m_aabb(positions, nVertices)
    , m_materialIndex(materialIndex)
{
  // Assignment 2
  createVertexBufferOnCPU(positions, normals, textureCoordinates, nVertices);
  createIndexBufferOnCPU(indexBuffer, nIndices);
  uploadVertexBuffOnGPU(device, commandQueue);
  uploadIndexBufferOnGPU(device, commandQueue);
}

void TriangleMeshD3D12::addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{

   commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  // Assignment 2
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);
  std::cout << "Vertex and index buffer successfully set!" << std::endl;


  commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

const AABB TriangleMeshD3D12::getAABB() const
{
  return m_aabb;
}

const ui32 TriangleMeshD3D12::getMaterialIndex() const
{
  return m_materialIndex;
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& TriangleMeshD3D12::getInputElementDescriptors()
{
  return m_inputElementDescs;
}

TriangleMeshD3D12::TriangleMeshD3D12()
    : m_nIndices(0)
    , m_vertexBufferSize(0)
    , m_indexBufferSize(0)
    , m_materialIndex((ui32)-1)
    , m_indexBufferView()
    , m_vertexBufferView()
{
}

void TriangleMeshD3D12::createVertexBufferOnCPU(f32v3 const* const positions, f32v3 const* const normals,
                             f32v3 const* const textureCoordinates, ui32 nVertices)
{
  m_vertexBufferOnCPU.resize(nVertices);
  for (ui32 i = 0; i < nVertices; i++)
  {
    Vertex vertexToAdd            = {};
    vertexToAdd.position = positions[i];
    vertexToAdd.normal   = normals[i];
    vertexToAdd.textureCoordinate = f32v2(textureCoordinates[i].x, textureCoordinates[i].y);
    m_vertexBufferOnCPU.at(i) = vertexToAdd;
  }

  std::cout << "Vertex buffer created successfully on the CPU" << std::endl;
}

 void TriangleMeshD3D12::createIndexBufferOnCPU(ui32v3 const* const indexBuffer, ui32 nIndices)
{
  m_indexBufferOnCPU.resize(nIndices);
  for (ui32 i = 0; i < (nIndices / 3); i++)
  {
    m_indexBufferOnCPU.at(i * 3) = indexBuffer[i].x;
    m_indexBufferOnCPU.at(i * 3 + 1) = indexBuffer[i].y;
    m_indexBufferOnCPU.at(i * 3 + 2) = indexBuffer[i].z;
  }
  std::cout << "Index buffer created successfully on the CPU!" << std::endl;

  ui32 minVal = 0;
  ui32 maxVal = 0;

  for (const auto& val : m_indexBufferOnCPU)
  {
    minVal = glm::min(minVal, val);
    maxVal = glm::max(maxVal, val);
  }

  std::cout << "Min. Index: " << minVal << " Max. Index:" << maxVal << std::endl;

}

void TriangleMeshD3D12::uploadVertexBuffOnGPU(const ComPtr<ID3D12Device>&       device,
                                            const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  UploadHelper vertexBufferUploader(device, m_vertexBufferSize);
  const CD3DX12_RESOURCE_DESC vertexBufferDescription =
      CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);
  const CD3DX12_HEAP_PROPERTIES     defaultHeapProperties   = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDescription,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer));
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = ui32(m_vertexBufferSize);
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);
  vertexBufferUploader.uploadBuffer(m_vertexBufferOnCPU.data(), m_vertexBuffer, m_vertexBufferSize, commandQueue);
  std::cout << "Vertex buffer uploaded successfully!" << std::endl;
}

void TriangleMeshD3D12::uploadIndexBufferOnGPU(const ComPtr<ID3D12Device>&       device,
                                             const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  UploadHelper indexBufferUploader(device, m_indexBufferSize);
  const CD3DX12_RESOURCE_DESC indexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferSize);
  const CD3DX12_HEAP_PROPERTIES     defaultHeapProperties   = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDescription,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer));
  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = ui32(m_indexBufferSize);
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  indexBufferUploader.uploadBuffer(m_indexBufferOnCPU.data(), m_indexBuffer, m_indexBufferSize, commandQueue);
  std::cout << "Index buffer uploaded successfully!" << std::endl;
}

} // namespace gims
