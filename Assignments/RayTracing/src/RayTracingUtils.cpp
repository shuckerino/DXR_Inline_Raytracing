#include "RayTracingUtils.hpp"
#include "SceneGraphViewerApp.hpp" // Full definition needed here

namespace
{
#pragma region Helper functions

inline void uploadDefaultBuffer(ComPtr<ID3D12Device5>& device, void* dataSrc, UINT64 datasize,
                                ComPtr<ID3D12Resource>& dataDst, ComPtr<ID3D12CommandQueue> commandQueue,
                                const wchar_t* resourceName = nullptr)
{
  const auto bufferDescription     = CD3DX12_RESOURCE_DESC::Buffer(datasize);
  const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDescription,
                                  D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&dataDst));

  UploadHelper helper(device, datasize);
  helper.uploadDefaultBuffer(dataSrc, dataDst, datasize, commandQueue);

  if (resourceName)
  {
    dataDst->SetName(resourceName);
  }
}

inline void allocateUAVBuffer(ComPtr<ID3D12Device5> device, ui64 bufferSize, ID3D12Resource** ppResource,
                              D3D12_RESOURCE_STATES initialResourceState, const wchar_t* resourceName)
{
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Alignment           = 0;
  desc.Width               = bufferSize;
  desc.Height              = 1;
  desc.DepthOrArraySize    = 1;
  desc.MipLevels           = 1;
  desc.Format              = DXGI_FORMAT_UNKNOWN;
  desc.SampleDesc.Count    = 1;
  desc.SampleDesc.Quality  = 0;
  desc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  throwIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &desc,
                                                initialResourceState, nullptr, IID_PPV_ARGS(ppResource)));
  (*ppResource)->SetName(resourceName);
}

#pragma endregion

} // namespace

#pragma region Init

RayTracingUtils RayTracingUtils::createRayTracingUtils(ComPtr<ID3D12Device5> device, Scene& scene,
                                                       ComPtr<ID3D12GraphicsCommandList4> commandList,
                                                       ComPtr<ID3D12CommandAllocator>     commandAllocator,
                                                       ComPtr<ID3D12CommandQueue>         commandQueue,
                                                       SceneGraphViewerApp&               app)
{
  RayTracingUtils rayTracingUtils(device);

  rayTracingUtils.createAccelerationStructures(device, scene, commandList, commandAllocator, commandQueue, app);

  return rayTracingUtils;
}

RayTracingUtils::RayTracingUtils(ComPtr<ID3D12Device5> device)
{
  if (isRayTracingSupported(device) == false)
  {
    throw std::runtime_error("Ray tracing not supported on this device");
  }
}

bool RayTracingUtils::isRayTracingSupported(ComPtr<ID3D12Device5> device)
{
  D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
  throwIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
  if (options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
  {
    std::cout << "Ray tracing not supported on this device" << std::endl;
    return false;
  }
  else
  {
    std::cout << "Ray tracing supported on your device" << std::endl;
    return true;
  }
}

#pragma region Build acceleration structures

void RayTracingUtils::createAccelerationStructures(ComPtr<ID3D12Device5> device, Scene& scene,
                                                   ComPtr<ID3D12GraphicsCommandList4> commandList,
                                                   ComPtr<ID3D12CommandAllocator>     commandAllocator,
                                                   ComPtr<ID3D12CommandQueue> commandQueue, SceneGraphViewerApp& app)
{
  // Reset the command list for the acceleration structure construction.
  commandList->Reset(commandAllocator.Get(), nullptr);
  const ui32 numMeshes = scene.getNumberOfMeshes();
  const ui32 numNodes  = scene.getNumberOfNodes();

  // Build all BLAS for scene
  std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
  std::vector<ComPtr<ID3D12Resource>>         scratchResources; // Keep scratch resources alive
  instanceDescs.reserve(numMeshes);

  // get global index and vertex buffer
  const auto& globalVertexBuffer = scene.m_globalVertexBufferResource;
  const auto& globalIndexBuffer  = scene.m_globalIndexBufferResource;

  for (ui16 i = 0; i < numNodes; i++)
  {
    const auto& currentNode = scene.getNode(i);
    if (currentNode.meshIndices.empty())
    {
      continue;
    }

    for (ui8 m = 0; m < currentNode.meshIndices.size(); m++)
    {
      const auto& currentMesh              = scene.getMesh(currentNode.meshIndices.at(m));
      const ui64  indexBufferOffsetInBytes = currentMesh.m_startIndex * sizeof(ui32);

      //  Create geometry description for each mesh
      D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
      geometryDesc.Type                           = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
      geometryDesc.Triangles.IndexBuffer  = globalIndexBuffer->GetGPUVirtualAddress() + indexBufferOffsetInBytes;
      geometryDesc.Triangles.IndexCount   = currentMesh.m_nIndices;
      geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
      geometryDesc.Triangles.Transform3x4 = 0;
      geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
      geometryDesc.Triangles.VertexCount  = currentMesh.m_nVertices;
      geometryDesc.Triangles.VertexBuffer.StartAddress  = globalVertexBuffer->GetGPUVirtualAddress();
      geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
      geometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

      // Create BLAS for each mesh
      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
      bottomLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
      bottomLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
      bottomLevelInputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
      bottomLevelInputs.NumDescs       = 1;
      bottomLevelInputs.pGeometryDescs = &geometryDesc;

      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
      device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
      throwIfZero(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

      // Create scratch buffer
      ComPtr<ID3D12Resource> scratchResource;
      allocateUAVBuffer(device, bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &scratchResource,
                        D3D12_RESOURCE_STATE_COMMON, L"BLAS_ScratchResource");
      scratchResources.push_back(scratchResource);

      ComPtr<ID3D12Resource> blasResource;
      allocateUAVBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &blasResource,
                        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"BottomLevelAccelerationStructure");
      m_bottomLevelAS.push_back(blasResource);
      const ui8 index = static_cast<ui8>(m_bottomLevelAS.size() - 1);

      // Bottom Level Acceleration Structure desc
      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
      bottomLevelBuildDesc.Inputs                                             = bottomLevelInputs;
      bottomLevelBuildDesc.ScratchAccelerationStructureData                   = scratchResource->GetGPUVirtualAddress();
      bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAS.at(index)->GetGPUVirtualAddress();

      commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
      auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAS.at(index).Get());
      commandList->ResourceBarrier(1, &uavBarrier);

      // transpose to match the row-major order of DirectX
      const auto worldSpaceTransformation = glm::transpose(currentNode.worldSpaceTransformation);

      // create instance description for each BLAS
      D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
      instanceDesc.Transform[0][0]                = worldSpaceTransformation[0][0];
      instanceDesc.Transform[0][1]                = worldSpaceTransformation[0][1];
      instanceDesc.Transform[0][2]                = worldSpaceTransformation[0][2];
      instanceDesc.Transform[0][3]                = worldSpaceTransformation[0][3];

      instanceDesc.Transform[1][0] = worldSpaceTransformation[1][0];
      instanceDesc.Transform[1][1] = worldSpaceTransformation[1][1];
      instanceDesc.Transform[1][2] = worldSpaceTransformation[1][2];
      instanceDesc.Transform[1][3] = worldSpaceTransformation[1][3];

      instanceDesc.Transform[2][0]       = worldSpaceTransformation[2][0];
      instanceDesc.Transform[2][1]       = worldSpaceTransformation[2][1];
      instanceDesc.Transform[2][2]       = worldSpaceTransformation[2][2];
      instanceDesc.Transform[2][3]       = worldSpaceTransformation[2][3];
      instanceDesc.InstanceMask          = 1;
      instanceDesc.InstanceID          = currentMesh.m_startIndex;
      instanceDesc.AccelerationStructure = m_bottomLevelAS.at(index)->GetGPUVirtualAddress();
      instanceDescs.push_back(instanceDesc);
    }
  }

  // upload instance descriptions
  ComPtr<ID3D12Resource> instanceDescsBuffer;
  uploadDefaultBuffer(device, instanceDescs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size(),
                      instanceDescsBuffer, commandQueue, L"InstanceDescs");

  // create TLAS
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
  topLevelInputs.DescsLayout                                          = D3D12_ELEMENTS_LAYOUT_ARRAY;
  topLevelInputs.Flags         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  topLevelInputs.NumDescs      = static_cast<UINT>(instanceDescs.size());
  topLevelInputs.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  topLevelInputs.InstanceDescs = instanceDescsBuffer->GetGPUVirtualAddress();

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
  device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
  throwIfZero(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

  ComPtr<ID3D12Resource> topLevelScratchResource;
  allocateUAVBuffer(device, topLevelPrebuildInfo.ScratchDataSizeInBytes, &topLevelScratchResource,
                    D3D12_RESOURCE_STATE_COMMON, L"TLAS_ScratchResource");

  allocateUAVBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAS,
                    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"TopLevelAccelerationStructure");

  // Top Level Acceleration Structure desc
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
  topLevelBuildDesc.Inputs                                             = topLevelInputs;
  topLevelBuildDesc.DestAccelerationStructureData                      = m_topLevelAS->GetGPUVirtualAddress();
  topLevelBuildDesc.ScratchAccelerationStructureData = topLevelScratchResource->GetGPUVirtualAddress();

  // Build TLAS.
  commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
  auto tlasBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_topLevelAS.Get());
  commandList->ResourceBarrier(1, &tlasBarrier);

  // Start acceleration structure construction
  commandList->Close(); // needs to be closed before execution
  ID3D12CommandList* commandLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(1, commandLists);

  // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
  app.waitForGPU();
}

#pragma endregion

#pragma endregion