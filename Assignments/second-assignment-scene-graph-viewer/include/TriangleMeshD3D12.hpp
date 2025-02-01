#pragma once
#include "AABB.hpp"
#include <d3d12.h>
#include <gimslib/types.hpp>
#include <vector>
#include <wrl.h>
#include <iostream>
using Microsoft::WRL::ComPtr;

struct Vertex
{
  gims::f32v3 position;
  gims::f32v3 normal;
  gims::f32v2 textureCoordinate;
};

namespace gims
{

/// <summary>
/// A D3D12 GPU triangle mesh.
/// </summary>
class TriangleMeshD3D12
{
public:
  /// <summary>
  /// Constructor that creates a D3D12 GPU Triangle mesh from positions, normals, and texture coordiantes.
  /// </summary>
  /// <param name="positions">Array of 3D positions. There must be nVertices elements in this array.</param>
  /// <param name="normals">Array of 3D normal vector. There must be nVertices elements in this array.</param>
  /// <param name="textureCoordinates">Array of 3D texture Coordinates. This class ignores the third component of each
  /// texture coordinate. There must be nVertices elements in this array.</param>
  /// <param name="nVertices">Number of vertices.</param>
  /// <param name="indexBuffer">Index buffer for triangle list. Triples of integer indices form a triangle.</param>
  /// <param name="nIndices">Number of indices (NOT the number triangles!)</param>
  /// <param name="materialIndex">Material index.</param>
  /// <param name="device">Device on which the GPU buffers should be created.</param>
  /// <param name="commandQueue">Command queue used to copy the data from the GPU to the GPU.</param>
  TriangleMeshD3D12(f32v3 const* const positions, f32v3 const* const normals, f32v3 const* const textureCoordinates,
                    ui32 nVertices, ui32v3 const* const indexBuffer, ui32 nIndices, ui32 materialIndex,
                    const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue);

  /// <summary>
  /// Adds the commands neccessary for rendering this triangle mesh to the provided commandList.
  /// </summary>
  /// <param name="commandList">The command list</param>
  void addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

  /// <summary>
  /// Returns the axis-aligned bounding-box of the mesh.
  /// </summary>
  /// <returns></returns>
  const AABB getAABB() const;

  /// <summary>
  /// Gets the matrix index of this mesh.
  /// </summary>
  /// <returns><The material index of the mesh./returns>
  const ui32 getMaterialIndex() const;

  /// <summary>
  /// Returns the input element descriptors required for the pipeline.
  /// </summary>
  /// <returns>The input element descriptor.</returns>
  static const std::vector<D3D12_INPUT_ELEMENT_DESC>& getInputElementDescriptors();

  TriangleMeshD3D12();
  TriangleMeshD3D12(const TriangleMeshD3D12& other)                = default;
  TriangleMeshD3D12(TriangleMeshD3D12&& other) noexcept            = default;
  TriangleMeshD3D12& operator=(const TriangleMeshD3D12& other)     = default;
  TriangleMeshD3D12& operator=(TriangleMeshD3D12&& other) noexcept = default;

private:
  ui32                   m_nIndices;         //! Number of indices in the index buffer.
  ui32                   m_vertexBufferSize; //! Vertex buffer size in bytes.
  ui32                   m_indexBufferSize;  //! Index buffer size in bytes.
  AABB                   m_aabb;             //! Axis aligned bounding box of the mesh.
  ui32                   m_materialIndex;    //! Material index of the mesh.
  ComPtr<ID3D12Resource> m_vertexBuffer;     //! The vertex buffer on the GPU.
  ComPtr<ID3D12Resource> m_indexBuffer;      //! The index buffer on the GPU.
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
  D3D12_INDEX_BUFFER_VIEW m_indexBufferView;


  //! Input element descriptor defining the vertex format.
  static const std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputElementDescs;

  std::vector<Vertex> m_vertexBufferOnCPU;
  std::vector<ui32> m_indexBufferOnCPU;

  void createVertexBufferOnCPU(f32v3 const* const positions, f32v3 const* const normals,
                               f32v3 const* const textureCoordinates, ui32 nVertices);

  void createIndexBufferOnCPU(ui32v3 const* const indexBuffer, ui32 nIndices);
  void uploadVertexBuffOnGPU(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue);
  void uploadIndexBufferOnGPU(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue);

};

} // namespace gims
