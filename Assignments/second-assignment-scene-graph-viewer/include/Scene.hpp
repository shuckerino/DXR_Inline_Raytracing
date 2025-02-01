#pragma once
#include "TriangleMeshD3D12.hpp"
#include <ConstantBufferD3D12.hpp>
#include <Texture2DD3D12.hpp>
#include <d3d12.h>
#include <gimslib/types.hpp>
#include <vector>

namespace gims
{

class SceneGraphFactory;

/// <summary>
/// Class that represents a scene graph for D3D12 rendering.
/// </summary>
class Scene
{
public:
  /// <summary>
  /// Node of the scene graph.
  /// </summary>
  struct Node
  {
    f32m4             transformation; //! Transformation to parent node.
    std::vector<ui32> meshIndices;    //! Index in the array of meshIndices, i.e., Scene::m_meshes[].
    std::vector<ui32> childIndices;   //! Index in the array of nodes, i.e.,Scene::m_nodes[].


  };

  /// <summary>
  /// Material information per mesh that will be uploaded to the GPU.
  /// </summary>
  struct MaterialConstantBuffer
  {
    f32v4 ambientColor             = f32v4(0); //! Ambient Color.
    f32v4 diffuseColor             = f32v4(0); //! Diffuse Color.
    f32v4 specularColorAndExponent = f32v4(0); //! xyz: Specular Color, w: Specular Exponent.
  };

  /// <summary>
  /// Material Information.
  /// </summary>
  struct Material
  {
    ConstantBufferD3D12          materialConstantBuffer; //! Constant buffer for the material.
    ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;      //! Descriptor Heap for the textures.
  };

  /// <summary>
  /// Default constructor.
  /// </summary>
  Scene() = default;

  /// <summary>
  /// Returns the axis aligned bounding box of the scene.
  /// </summary>
  const AABB& getAABB() const;

  /// <summary>
  /// Nodes are stored in a flat 1D array. This functions returns the Node at the respecitve index.
  /// </summary>
  /// <param name="nodeIdx">Index of the node within the array of nodes.</param>
  /// <returns></returns>
  const Node& getNode(ui32 nodeIdx) const;

  /// <summary>
  /// Nodes are stored in a flat 1D array. This functions returns the Node at the respecitve index.
  /// </summary>
  /// <param name="nodeIdx">Index of the node within the array of nodes.</param>
  /// <returns></returns>
  Node& getNode(ui32 nodeIdx);

  /// <summary>
  /// Returns the total number of nodes.
  /// </summary>
  /// <returns></returns>
  const ui32 getNumberOfNodes() const;

  /// <summary>
  /// Triangle meshes are stored in a 1D array. This functions returns the TriangleMeshD3D12 at the respective index.
  /// </summary>
  /// <param name="materialIdx">Index of the mesh.</param>
  const TriangleMeshD3D12& getMesh(ui32 meshIdx) const;

  /// <summary>
  /// Materials are stored in a 1D array. This function returns the Material at the respective index.
  /// </summary>
  /// <param name="materialIdx">The index of the material</param>
  const Material& getMaterial(ui32 materialIdx) const;

  /// <summary>
  /// Traverse the scene graph and add the draw calls, and all other neccessary commands to the command list.
  /// </summary>
  /// <param name="commandList">The command list to which the commands will be added.</param>
  /// <param name="viewMatrix">The view matrix (or camera matrix).</param>
  /// <param name="modelViewRootParameterIdx">>In your root signature, reserve 16 floats for root constants
  /// which obtain the model view matrix.</param>
  /// <param name="materialConstantsRootParameterIdx">In your root signature, the parameter index of the material
  /// constant buffer.</param>
  /// <param name="srvRootParameterIdx">In your root signature the paramer index of the Shader-Resource-View For the
  /// textures.</param>
  void addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList, const f32m4 transformation,
                        ui32 modelViewRootParameterIdx, ui32 materialConstantsRootParameterIdx,
                        ui32 srvRootParameterIdx);

  // Allow the class SceneGraphFactor access to the privatem mebers.
  friend class SceneGraphFactory;

private:
  std::vector<Node>              m_nodes;     //! The nodes of the scene.
  std::vector<TriangleMeshD3D12> m_meshes;    //! Array meshes of the scene.
  AABB                           m_aabb;      //! The axis-aligned bounding box of the scene.
  std::vector<Material>          m_materials; //! Material information for each mesh.
  std::vector<Texture2DD3D12>    m_textures;  //! Array of textures.
};
} // namespace gims
