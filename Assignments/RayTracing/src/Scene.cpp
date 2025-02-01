#include "Scene.hpp"
#include <unordered_map>

using namespace gims;

namespace
{
void addToCommandListImpl(Scene& scene, ui32 nodeIdx, f32m4 accuModelView,
                          const ComPtr<ID3D12GraphicsCommandList>& commandList, ui32 modelViewRootParameterIdx)
{
  if (nodeIdx >= scene.getNumberOfNodes())
  {
    return;
  }

  const auto& currentNode         = scene.getNode(nodeIdx);
  const auto  worldTransformation = currentNode.worldSpaceTransformation;
  accuModelView = accuModelView * currentNode.transformation;

  // draw meshes
  for (ui32 m = 0; m < (ui32)currentNode.meshIndices.size(); m++)
  {
    const auto& meshToDraw          = scene.getMesh(currentNode.meshIndices[m]);
    const auto& meshMaterial        = scene.getMaterial(meshToDraw.getMaterialIndex());
    int         isReflectiveFlag    = meshToDraw.m_isReflective;
    int         meshDescriptorIndex = meshMaterial.m_descriptorIndex - 2; // -2 because of vertex and index buffer

    commandList->SetGraphicsRoot32BitConstants(modelViewRootParameterIdx, 16, &accuModelView, 0);
    commandList->SetGraphicsRoot32BitConstants(modelViewRootParameterIdx, 16, &worldTransformation, 16);
    commandList->SetGraphicsRoot32BitConstants(modelViewRootParameterIdx, 1, &isReflectiveFlag, 32);
    commandList->SetGraphicsRoot32BitConstants(modelViewRootParameterIdx, 1, &meshDescriptorIndex, 33);

    commandList->SetGraphicsRootConstantBufferView(
        2, meshMaterial.materialConstantBuffer.getResource()->GetGPUVirtualAddress());

    // draw call
    meshToDraw.addToCommandList(commandList);
  }

  for (ui32 c = 0; c < (ui32)currentNode.childIndices.size(); c++)
  {
    addToCommandListImpl(scene, currentNode.childIndices[c], accuModelView, commandList, modelViewRootParameterIdx);
  }
}
} // namespace
namespace gims
{
const Scene::Node& Scene::getNode(ui32 nodeIdx) const
{
  return m_nodes[nodeIdx];
}

Scene::Node& Scene::getNode(ui32 nodeIdx)
{
  return m_nodes[nodeIdx];
}

const ui32 Scene::getNumberOfNodes() const
{
  return static_cast<ui32>(m_nodes.size());
}

const ui32 Scene::getNumberOfMeshes() const
{
  return static_cast<ui32>(m_meshes.size());
}

const TriangleMeshD3D12& Scene::getMesh(ui32 meshIdx) const
{
  return m_meshes[meshIdx];
}

const Scene::Material& Scene::getMaterial(ui32 materialIdx) const
{
  return m_materials[materialIdx];
}

const AABB& Scene::getAABB() const
{
  return m_aabb;
}

void Scene::addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList, const f32m4 modelView,
                             ui32 modelViewRootParameterIdx)
{
  addToCommandListImpl(*this, 0, modelView, commandList, modelViewRootParameterIdx);
}
} // namespace gims
