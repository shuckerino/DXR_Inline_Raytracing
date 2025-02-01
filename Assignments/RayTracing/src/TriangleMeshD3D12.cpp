#include "TriangleMeshD3D12.hpp"
#include <gimslib/d3d/UploadHelper.hpp>
#include <iostream>

namespace gims
{
TriangleMeshD3D12::TriangleMeshD3D12(f32v3 const* const positions, f32v3 const* const normals,
                                     f32v3 const* const textureCoordinates, ui32 nVertices,
                                     ui32v3 const* const indexBuffer, ui32 nIndices, f32v3 const* const tangents,
                                     ui32 materialIndex)
    : m_nIndices(nIndices)
    , m_nVertices(nVertices)
    , m_vertexBufferSize(static_cast<ui32>(nVertices * sizeof(Vertex)))
    , m_indexBufferSize(static_cast<ui32>(nIndices * sizeof(ui32)))
    , m_aabb(positions, nVertices)
    , m_materialIndex(materialIndex)
{


  // Init vertex buffer
  std::vector<Vertex> vertexBuffer;
  vertexBuffer.reserve(nVertices);
  for (ui32 i = 0; i < nVertices; i++)
  {
    // const auto v = Vertex(positions[i], normals[i], textureCoordinates[i]);
    // vertexBuffer.emplace_back(m_aabb.getNormalizationTransformation() * f32v4(v.position, 1.0f));
    vertexBuffer.emplace_back(positions[i], normals[i], textureCoordinates[i], tangents[i], m_materialIndex);
  }

  std::vector<ui32> indexBufferCPU;
  indexBufferCPU.reserve(nIndices);
  for (ui32 i = 0; i < ui32(nIndices / 3); i++) // need to divide by 3, because we iterate over vec3
  {
    indexBufferCPU.emplace_back(indexBuffer[i].x);
    indexBufferCPU.emplace_back(indexBuffer[i].y);
    indexBufferCPU.emplace_back(indexBuffer[i].z);
  }

  m_vertices = vertexBuffer;
  m_indices  = indexBufferCPU;
}

void TriangleMeshD3D12::addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->DrawIndexedInstanced(m_nIndices, 1, m_startIndex, 0, 0);
}

const ComPtr<ID3D12Resource>& TriangleMeshD3D12::getVertexBuffer() const
{
  return m_vertexBuffer;
}

const ComPtr<ID3D12Resource>& TriangleMeshD3D12::getIndexBuffer() const
{
  return m_indexBuffer;
}

const AABB TriangleMeshD3D12::getAABB() const
{
  return m_aabb;
}

const ui32 TriangleMeshD3D12::getMaterialIndex() const
{
  return m_materialIndex;
}

TriangleMeshD3D12::TriangleMeshD3D12()
    : m_nIndices(0)
    , m_vertexBufferSize(0)
    , m_indexBufferSize(0)
    , m_materialIndex((ui32)-1)
    , m_vertexBufferView()
    , m_indexBufferView()
    , m_nVertices(0)
    , m_startIndex(0)
    , m_startVertex(0)
    , m_isReflective(false)
{
}

} // namespace gims
