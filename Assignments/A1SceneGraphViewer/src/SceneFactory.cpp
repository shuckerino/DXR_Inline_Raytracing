#include "SceneFactory.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <iostream>
using namespace gims;

namespace
{
/// <summary>
/// Converts the index buffer required for D3D12 rendering from an aiMesh.
/// </summary>
/// <param name="mesh">The ai mesh containing an index buffer.</param>
/// <returns></returns>
std::vector<ui32v3> getTriangleIndicesFromAiMesh(aiMesh const* const mesh)
{
  std::vector<ui32v3> result;
  if (!mesh->HasFaces())
    return result;

  result.reserve(mesh->mNumFaces);
  for (ui32 i = 0; i < mesh->mNumFaces; i++)
  {
    const aiFace& currentFace = mesh->mFaces[i];
    if (currentFace.mNumIndices == 3)
    {
      result.emplace_back(currentFace.mIndices[0], currentFace.mIndices[1], currentFace.mIndices[2]);
    }
    else
    {
      std::cout << "Not 3 indices" << std::endl;
    }
  }

  return result;
}

ui8 getDefaultTextureIndexForTextureType(aiTextureType aiTextureTypeValue)
{
  if (aiTextureTypeValue == aiTextureType_AMBIENT)
    return 1;
  if (aiTextureTypeValue == aiTextureType_DIFFUSE)
    return 0;
  if (aiTextureTypeValue == aiTextureType_SPECULAR)
    return 0;
  if (aiTextureTypeValue == aiTextureType_EMISSIVE)
    return 1;
  if (aiTextureTypeValue == aiTextureType_HEIGHT)
    return 2;
  return 0;
}

void addTextureToDescriptorHeap(const ComPtr<ID3D12Device>& device, aiTextureType aiTextureTypeValue,
                                i32 offsetInDescriptors, aiMaterial const* const inputMaterial,
                                std::vector<Texture2DD3D12>& m_textures, ComPtr<ID3D12DescriptorHeap> descriptorHeap,
                                std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex)
{
  // default texture
  if (inputMaterial->GetTextureCount(aiTextureTypeValue) == 0)
  {
    m_textures[getDefaultTextureIndexForTextureType(aiTextureTypeValue)].addToDescriptorHeap(device, descriptorHeap,
                                                                                             offsetInDescriptors);
  }
  else // custom texture
  {
    aiString path;
    inputMaterial->GetTexture(aiTextureTypeValue, 0, &path);
    m_textures[textureFileNameToTextureIndex[path.C_Str()]].addToDescriptorHeap(device, descriptorHeap,
                                                                                offsetInDescriptors);
  }
}

std::unordered_map<std::filesystem::path, ui32> textureFilenameToIndex(aiScene const* const inputScene)
{
  std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex;

  ui32 textureIdx = 3;
  for (ui32 mIdx = 0; mIdx < inputScene->mNumMaterials; mIdx++)
  {
    for (ui32 textureType = aiTextureType_NONE; textureType < aiTextureType_UNKNOWN; textureType++)
    {
      for (ui32 i = 0; i < inputScene->mMaterials[mIdx]->GetTextureCount((aiTextureType)textureType); i++)
      {
        aiString path;
        inputScene->mMaterials[mIdx]->GetTexture((aiTextureType)textureType, i, &path);

        const auto texturePathCstr = path.C_Str();
        const auto textureIter     = textureFileNameToTextureIndex.find(texturePathCstr);
        if (textureIter == textureFileNameToTextureIndex.end())
        {
          textureFileNameToTextureIndex.emplace(texturePathCstr, static_cast<ui32>(textureIdx));
          textureIdx++;
        }
      }
    }
  }
  return textureFileNameToTextureIndex;
}
/// <summary>
/// Reads the color from the Asset Importer specific (pKey, type, idx) triple.
/// Use the Asset Importer Macros AI_MATKEY_COLOR_AMBIENT, AI_MATKEY_COLOR_DIFFUSE, etc. which map to these arguments
/// correctly.
///
/// If that key does not exist a null vector is returned.
/// </summary>
/// <param name="pKey">Asset importer specific parameter</param>
/// <param name="type"></param>
/// <param name="idx"></param>
/// <param name="material">The material from which we wish to extract the color.</param>
/// <returns>Color or 0 vector if no color exists.</returns>
f32v4 getColor(char const* const pKey, unsigned int type, unsigned int idx, aiMaterial const* const material)
{
  aiColor3D color;
  if (material->Get(pKey, type, idx, color) == aiReturn_SUCCESS)
  {
    return f32v4(color.r, color.g, color.b, 0.0f);
  }
  else
  {
    return f32v4(0.0f);
  }
}

} // namespace

namespace gims
{
Scene SceneGraphFactory::createFromAssImpScene(const std::filesystem::path       pathToScene,
                                               const ComPtr<ID3D12Device>&       device,
                                               const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  Scene outputScene;

  const auto absolutePath = std::filesystem::weakly_canonical(pathToScene);
  if (!std::filesystem::exists(absolutePath))
  {
    throw std::exception((absolutePath.string() + std::string(" does not exist.")).c_str());
  }

  const auto arguments = aiPostProcessSteps::aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                         aiProcess_GenUVCoords | aiProcess_ConvertToLeftHanded | aiProcess_OptimizeMeshes |
                         aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality |
                         aiProcess_FindInvalidData | aiProcess_FindDegenerates | aiProcess_CalcTangentSpace;

  Assimp::Importer imp;
  imp.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
  auto inputScene = imp.ReadFile(absolutePath.string(), arguments);
  if (!inputScene)
  {
    throw std::exception((absolutePath.string() + std::string(" can't be loaded. with Assimp.")).c_str());
  }
  const auto textureFileNameToTextureIndex = textureFilenameToIndex(inputScene);

  createMeshes(inputScene, device, commandQueue, outputScene);

  createNodes(inputScene, outputScene, inputScene->mRootNode);

  std::cout << outputScene.m_nodes.size() << std::endl;

  computeSceneAABB(outputScene, outputScene.m_aabb, 0, glm::identity<f32m4>());
  createTextures(textureFileNameToTextureIndex, absolutePath.parent_path(), device, commandQueue, outputScene);
  createMaterials(inputScene, textureFileNameToTextureIndex, device, outputScene);

  return outputScene;
}

/// <summary>
/// Method calling TriangleMeshD3D12::TriangleMeshD3D12 for each mesh in inputScene
/// </summary>
/// <param name="inputScene"></param>
/// <param name="device"></param>
/// <param name="commandQueue"></param>
/// <param name="outputScene"></param>
void SceneGraphFactory::createMeshes(aiScene const* const inputScene, const ComPtr<ID3D12Device>& device,
                                     const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene)
{
  for (ui32 i = 0; i < inputScene->mNumMeshes; i++)
  {
    const aiMesh* currentMesh = inputScene->mMeshes[i];

    // get positions and normals
    const ui32         numVertices = currentMesh->mNumVertices;
    std::vector<f32v3> positions;
    positions.reserve(numVertices);
    std::vector<f32v3> normals;
    normals.reserve(numVertices);
    std::vector<f32v3> textureCoords;
    textureCoords.reserve(numVertices);
    std::vector<f32v3> tangents;
    tangents.reserve(numVertices);
    for (ui32 n = 0; n < numVertices; n++)
    {
      const aiVector3D& currentPos = currentMesh->mVertices[n];
      positions.emplace_back(currentPos.x, currentPos.y, currentPos.z);

      if (currentMesh->HasNormals())
      {
        const aiVector3D& currentNormal = currentMesh->mNormals[n];
        normals.emplace_back(currentNormal.x, currentNormal.y, currentNormal.z);
      }
      else
      {
        normals.emplace_back(0.0f, 0.0f, 0.0f); // default normal if missing
      }

      if (currentMesh->HasTextureCoords(0))
      {
        const aiVector3D& currentTexCoord = currentMesh->mTextureCoords[0][n];
        textureCoords.emplace_back(currentTexCoord.x, currentTexCoord.y, 0.0f);
      }
      else
      {
        textureCoords.emplace_back(0.0f, 0.0f, 0.0f); // default UV if missing
      }

      if (currentMesh->HasTangentsAndBitangents())
      {
        const aiVector3D& currentTangent = currentMesh->mTangents[i];
        tangents.emplace_back(currentTangent.x, currentTangent.y, currentTangent.z);
      }
      else
      {
        tangents.emplace_back(0.0f, 0.0f, 0.0f);
      }
    }

    // get triangle indices
    const std::vector<ui32v3> indexBuffer = getTriangleIndicesFromAiMesh(currentMesh);
    const ui32                numIndices  = 3 * static_cast<ui32>(indexBuffer.size()); // mul by 3, because of vec3

    std::cout << "NumVertices: " << numVertices << std::endl;
    std::cout << "NumIndices: " << numIndices << std::endl;

    //// manual calculation of tangents -> not needed, assimp already provides
    // std::vector<f32v3> tangents;
    // tangents.reserve(indexBuffer.size());

    //// calculations for normal mapping
    // for (const auto& triangleFace : indexBuffer)
    //{
    //   f32v3 edge1    = positions[triangleFace[1]] - positions[triangleFace[0]];
    //   f32v3 edge2    = positions[triangleFace[2]] - positions[triangleFace[0]];
    //   f32v2 deltaUV1 = textureCoords[triangleFace[1]] - textureCoords[triangleFace[0]];
    //   f32v2 deltaUV2 = textureCoords[triangleFace[2]] - textureCoords[triangleFace[0]];

    //  float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    //  f32 tangent1_x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    //  f32 tangent1_y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    //  f32 tangent1_z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    //  tangents.emplace_back(tangent1_x, tangent1_y, tangent1_z);
    //}

    // create internal mesh
    TriangleMeshD3D12 createdMesh = TriangleMeshD3D12::TriangleMeshD3D12(
        positions.data(), normals.data(), textureCoords.data(), numVertices, indexBuffer.data(), numIndices,
        tangents.data(), currentMesh->mMaterialIndex, device, commandQueue);

    outputScene.m_meshes.push_back(createdMesh);
  }
}

ui32 SceneGraphFactory::createNodes(aiScene const* const inputScene, Scene& outputScene, aiNode const* const assimpNode)
{
  (void)inputScene;

  // create node and add to list
  outputScene.m_nodes.emplace_back();
  const auto   currentNodeIndex = static_cast<ui32>(outputScene.m_nodes.size() - 1);
  Scene::Node& currentNode      = outputScene.m_nodes.back();

  // set transformation to parent
  const aiMatrix4x4 parentRelativeTransformation = assimpNode->mTransformation;
  glm::mat4         convertedAssimpTrafo         = aiMatrix4x4ToGlm(parentRelativeTransformation);
  currentNode.transformation                     = convertedAssimpTrafo;

  // set mesh indices
  for (ui32 i = 0; i < assimpNode->mNumMeshes; i++)
  {
    currentNode.meshIndices.push_back(assimpNode->mMeshes[i]);
  }

  // traverse children
  aiNode** childrenOfInputNode = assimpNode->mChildren;
  for (ui32 i = 0; i < assimpNode->mNumChildren; i++)
  {
    const ui32 childNodeIndex = createNodes(inputScene, outputScene, childrenOfInputNode[i]);
    outputScene.m_nodes.at(currentNodeIndex).childIndices.emplace_back(childNodeIndex);
  }

  return currentNodeIndex;
}

void SceneGraphFactory::computeSceneAABB(Scene& scene, AABB& accuAABB, ui32 nodeIdx, f32m4 accuTransformation)
{
  // get current node
  const auto& currentNode = scene.getNode(nodeIdx);

  // update transformation
  accuTransformation = accuTransformation * currentNode.transformation;

  // merge aabb for each mesh
  for (const auto& meshIndex : currentNode.meshIndices)
  {
    const auto transformedMeshAABB = scene.m_meshes[meshIndex].getAABB().getTransformed(accuTransformation);
    accuAABB                       = accuAABB.getUnion(transformedMeshAABB);
  }
  for (ui32 i = 0; i < (ui32)currentNode.childIndices.size(); i++)
  {
    computeSceneAABB(scene, accuAABB, currentNode.childIndices[i], accuTransformation);
  }
}

void SceneGraphFactory::createTextures(
    const std::unordered_map<std::filesystem::path, ui32>& textureFileNameToTextureIndex,
    std::filesystem::path parentPath, const ComPtr<ID3D12Device>& device,
    const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene)
{
  outputScene.m_textures.resize(ui32(textureFileNameToTextureIndex.size() + 3));
  // create default textures
  const auto white             = gims::ui8v4(255, 255, 255, 255);
  const auto black             = gims::ui8v4(0, 0, 0, 255);
  const auto blue              = gims::ui8v4(0, 0, 255, 255);
  outputScene.m_textures.at(0) = Texture2DD3D12(&white, 1, 1, device, commandQueue); // white
  outputScene.m_textures.at(1) = Texture2DD3D12(&black, 1, 1, device, commandQueue); // black
  outputScene.m_textures.at(2) = Texture2DD3D12(&blue, 1, 1, device, commandQueue);  // blue

  // create every texture in "textureFileNameToTextureIndex"
  for (const auto& entry : textureFileNameToTextureIndex)
  {
    const std::filesystem::path pathToFilename = parentPath / entry.first;
    const Texture2DD3D12        createdTexture(pathToFilename, device, commandQueue);
    outputScene.m_textures.at(entry.second) = createdTexture;
  }
}

void SceneGraphFactory::createMaterials(aiScene const* const                            inputScene,
                                        std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex,
                                        const ComPtr<ID3D12Device>& device, Scene& outputScene)
{
  // iterate over materials in the scene
  for (ui32 i = 0; i < inputScene->mNumMaterials; i++)
  {
    aiMaterial*                   currentMaterial = inputScene->mMaterials[i];
    Scene::MaterialConstantBuffer mcb;

    // fill mcb by extracting data from assimp
    f32v4 emissiveFactors = getColor(AI_MATKEY_COLOR_EMISSIVE, currentMaterial);
    mcb.ambientColor      = getColor(AI_MATKEY_COLOR_AMBIENT, currentMaterial) + emissiveFactors;
    mcb.diffuseColor      = getColor(AI_MATKEY_COLOR_DIFFUSE, currentMaterial);
    ai_real specularExponent;
    aiGetMaterialFloat(currentMaterial, AI_MATKEY_SHININESS, &specularExponent);
    f32v4 specularColor          = getColor(AI_MATKEY_COLOR_SPECULAR, currentMaterial);
    mcb.specularColorAndExponent = f32v4(specularColor.r, specularColor.g, specularColor.b, specularExponent);

    // create constant buffer
    ConstantBufferD3D12 materialConstantBuffer(mcb, device);

    // Create descriptor heap for the textures
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors             = 5; // 5 descriptors for different texture types
    heapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> textureDescriptorHeap;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&textureDescriptorHeap));
    CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // create material and add to scene
    outputScene.m_materials.emplace_back(materialConstantBuffer, textureDescriptorHeap);

    ui8 descriptorIndex = 0;

    addTextureToDescriptorHeap(device, aiTextureType_AMBIENT, descriptorIndex++, currentMaterial,
                               outputScene.m_textures, textureDescriptorHeap, textureFileNameToTextureIndex);
    addTextureToDescriptorHeap(device, aiTextureType_DIFFUSE, descriptorIndex++, currentMaterial,
                               outputScene.m_textures, textureDescriptorHeap, textureFileNameToTextureIndex);
    addTextureToDescriptorHeap(device, aiTextureType_SPECULAR, descriptorIndex++, currentMaterial,
                               outputScene.m_textures, textureDescriptorHeap, textureFileNameToTextureIndex);
    addTextureToDescriptorHeap(device, aiTextureType_EMISSIVE, descriptorIndex++, currentMaterial,
                               outputScene.m_textures, textureDescriptorHeap, textureFileNameToTextureIndex);
    addTextureToDescriptorHeap(device, aiTextureType_HEIGHT, descriptorIndex++, currentMaterial, outputScene.m_textures,
                               textureDescriptorHeap, textureFileNameToTextureIndex);

    // log for debug
    std::cout << "Created material: " << currentMaterial->GetName().C_Str() << std::endl;
    std::cout << "Ambient Color: " << glm::to_string(mcb.ambientColor) << std::endl;
    std::cout << "Diffuse Color: " << glm::to_string(mcb.diffuseColor) << std::endl;
    std::cout << "Specular Color with exponent: " << glm::to_string(mcb.specularColorAndExponent) << std::endl;
  }

  // Assignment 9
  (void)textureFileNameToTextureIndex;
  // Assignment 10
}

} // namespace gims
