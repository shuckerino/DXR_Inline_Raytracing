#pragma once
#include "Scene.hpp"
#include <filesystem>
#include <unordered_map>

struct aiScene;
struct aiNode;
namespace gims
{
class SceneGraphFactory
{
public:
  static Scene createFromAssImpScene(const std::filesystem::path pathToScene, const ComPtr<ID3D12Device>& device,
                                     const ComPtr<ID3D12CommandQueue>& commandQueue);

private:
  static void createMeshes(aiScene const* const inputScene, const ComPtr<ID3D12Device>& device,
                           const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene);

  static ui32 createNodes(aiScene const* const inputScene, Scene& outputScene, aiNode const* const startNode);

  static void computeSceneAABB(Scene& scene, AABB& aabb, ui32 nodeIdx, f32m4 transformation);

  static void createTextures(const std::unordered_map<std::filesystem::path, ui32>& textureFileNameToTextureIndex,
                             std::filesystem::path parentPath, const ComPtr<ID3D12Device>& device,
                             const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene);

  static void createMaterials(aiScene const* const                            inputScene,
                              std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex,
                              const ComPtr<ID3D12Device>& device, Scene& outputScene);
};
} // namespace gims
