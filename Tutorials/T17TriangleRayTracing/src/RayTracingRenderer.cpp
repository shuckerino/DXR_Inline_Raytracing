#include "RayTracingRenderer.hpp"
#include <imgui.h>

using namespace gims;

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

void RayTracingRenderer::AllocateUAVBuffer(ui64 bufferSize, ID3D12Resource** ppResource,
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

  throwIfFailed(getDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &desc,
                                                     initialResourceState, nullptr, IID_PPV_ARGS(ppResource)));
  (*ppResource)->SetName(resourceName);
}

#pragma endregion

#pragma region Init

#pragma region Rasterizing

void RayTracingRenderer::createPipeline()
{
  const auto vertexShader =
      compileShader(L"../../../Tutorials/T17TriangleRayTracing/Shaders/RayTracing.hlsl", L"VS_main", L"vs_6_3");

  const auto pixelShader =
      compileShader(L"../../../Tutorials/T17TriangleRayTracing/Shaders/RayTracing.hlsl", L"PS_main", L"ps_6_8");

  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      // Per-instance data
      {"INSTANCE_DATA", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
      {"INSTANCE_DATA", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
      {"INSTANCE_DATA", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
      {"INSTANCE_DATA", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  };

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {inputElementDescs, _countof(inputElementDescs)};
  psoDesc.pRootSignature                     = m_globalRootSignature.Get();
  psoDesc.VS                                 = HLSLCompiler::convert(vertexShader);
  psoDesc.PS                                 = HLSLCompiler::convert(pixelShader);
  psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState.DepthEnable      = TRUE;
  psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.StencilEnable    = FALSE;
  psoDesc.SampleMask                         = UINT_MAX;
  psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                   = 1;
  psoDesc.SampleDesc.Count                   = 1;
  psoDesc.RTVFormats[0]                      = getDX12AppConfig().renderTargetFormat;
  psoDesc.DSVFormat                          = getDX12AppConfig().depthBufferFormat;
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

#pragma endregion

RayTracingRenderer::RayTracingRenderer(const DX12AppConfig createInfo)
    : DX12App(createInfo)
    , m_examinerController(true)
{
  if (isRayTracingSupported() == false)
  {
    throw std::runtime_error("Ray tracing not supported on this device");
  }

  m_uiData.m_backgroundColor = f32v3(0.25f, 0.25f, 0.25f);
  m_uiData.m_lightDirection  = f32v3(0.462f, 0.3f, 0.9f);
  m_uiData.m_shadowFactor = 0.5f;
  m_examinerController.setTranslationVector(f32v3(0, -0.4, 4));

  createResources();
  createPipeline();

  ComPtr<ID3D12Debug> debugController;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
  {
    debugController->EnableDebugLayer();
  }
}

bool RayTracingRenderer::isRayTracingSupported()
{
  D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
  throwIfFailed(getDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
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

void RayTracingRenderer::createResources()
{
  createRootSignature();
  createGeometries();
  createAccelerationStructures();
}

void RayTracingRenderer::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER rootParameters[2];
  rootParameters[0].InitAsShaderResourceView(0); // TLAS
  rootParameters[1].InitAsConstants(21, 0, 0,
                                    D3D12_SHADER_VISIBILITY_ALL); // root constants (mvp, light direction, shadowFactor, flags)

  CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc;
  globalRootSignatureDesc.Init(ARRAYSIZE(rootParameters), rootParameters, 0, nullptr,
                               D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_globalRootSignature));

  std::cout << "Created global root signature" << std::endl;
}

#pragma region Create Geometry

void RayTracingRenderer::createGeometries()
{
  auto device       = getDevice();
  auto commandQueue = getCommandQueue();

  createTriangleInstances(device, commandQueue);
  createPlaneGeometry(device, commandQueue);
}

void RayTracingRenderer::createTriangleInstances(ComPtr<ID3D12Device5> device, ComPtr<ID3D12CommandQueue> commandQueue)
{
  float depthValue       = 0.0f;  // depth value for triangle
  float offset           = 0.25f; // scales the triangle size
  float triangleDistance = 0.7f;  // distance between the triangles instances

  Vertex triangleVertices[] = {
      f32v3(0.0f, offset, depthValue),
      f32v3(-offset, 0.0f, depthValue),
      f32v3(offset, 0.0f, depthValue),
  };

  Index triangleIndices[] = {
      0,
      1,
      2,
  };

  InstanceData data;

  // First instance (no transformation)
  data.worldMatrix =
      f32m4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  m_triangleInstanceData[0] = data;

  // Second instance (translation to the left)
  data.worldMatrix = f32m4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -triangleDistance,
                           0.0f, 0.0f, 1.0f);
  m_triangleInstanceData[1] = data;

  // Third instance (translation to the right)
  data.worldMatrix =
      f32m4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, triangleDistance, 0.0f, 0.0f, 1.0f);
  m_triangleInstanceData[2] = data;

  const ui32 vertexBufferSize = sizeof(triangleVertices);
  const ui32 indexBufferSize  = sizeof(triangleIndices);
  m_numTriangleIndices        = _countof(triangleIndices);

  uploadDefaultBuffer(device, triangleVertices, sizeof(triangleVertices), m_triangleVertexBuffer, commandQueue);
  uploadDefaultBuffer(device, triangleIndices, sizeof(triangleIndices), m_triangleIndexBuffer, commandQueue);
  uploadDefaultBuffer(device, m_triangleInstanceData, sizeof(m_triangleInstanceData), m_instanceBuffer, commandQueue);

  // create views
  m_triangleVertexBufferView.BufferLocation = m_triangleVertexBuffer->GetGPUVirtualAddress();
  m_triangleVertexBufferView.SizeInBytes    = static_cast<ui32>(vertexBufferSize);
  m_triangleVertexBufferView.StrideInBytes  = sizeof(Vertex);

  m_triangleIndexBufferView.BufferLocation = m_triangleIndexBuffer->GetGPUVirtualAddress();
  m_triangleIndexBufferView.SizeInBytes    = static_cast<ui32>(indexBufferSize);
  m_triangleIndexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  m_instanceBufferView.BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
  m_instanceBufferView.SizeInBytes    = sizeof(m_triangleInstanceData);
  m_instanceBufferView.StrideInBytes  = sizeof(InstanceData);
}

void RayTracingRenderer::createPlaneGeometry(ComPtr<ID3D12Device5> device, ComPtr<ID3D12CommandQueue> commandQueue)
{
  float  planeSize             = 3.0f;
  float  triangleHoverDistance = 0.1f;
  Vertex planeVertices[]       = {
      f32v3(-planeSize, -triangleHoverDistance, -planeSize), // Bottom-left corner
      f32v3(planeSize, -triangleHoverDistance, -planeSize),  // Bottom-right corner
      f32v3(-planeSize, -triangleHoverDistance, planeSize),  // Top-left corner
      f32v3(planeSize, -triangleHoverDistance, planeSize),   // Top-right corner
  };

  Index planeIndices[] = {
      0, 1, 2, 2, 1, 3,
  };

  const ui32 vertexBufferSize = sizeof(planeVertices);
  const ui32 indexBufferSize  = sizeof(planeIndices);
  m_numPlaneIndices           = _countof(planeIndices);

  uploadDefaultBuffer(device, planeVertices, sizeof(planeVertices), m_planeVertexBuffer, commandQueue);
  uploadDefaultBuffer(device, planeIndices, sizeof(planeIndices), m_planeIndexBuffer, commandQueue);

  // create views
  m_planeVertexBufferView.BufferLocation = m_planeVertexBuffer->GetGPUVirtualAddress();
  m_planeVertexBufferView.SizeInBytes    = static_cast<ui32>(vertexBufferSize);
  m_planeVertexBufferView.StrideInBytes  = sizeof(Vertex);

  m_planeIndexBufferView.BufferLocation = m_planeIndexBuffer->GetGPUVirtualAddress();
  m_planeIndexBufferView.SizeInBytes    = static_cast<ui32>(indexBufferSize);
  m_planeIndexBufferView.Format         = DXGI_FORMAT_R32_UINT;
}

#pragma endregion

#pragma region Create Acceleration Structure

void RayTracingRenderer::createAccelerationStructures()
{
  // Reset the command list for the acceleration structure construction.
  getCommandList()->Reset(getCommandAllocator().Get(), nullptr);
  auto device = getDevice();

  // Scratch resources need to stay in scope until command list execution
  auto blasScratchResources = createBottomLevelAccelerationStructures();

  ComPtr<ID3D12Resource> instanceDescsBuffer = createTriangleInstanceDescriptions();

  auto topLevelBuildDescription = createTopLevelAccelerationStructure(instanceDescsBuffer);

  // Build TLAS.
  getCommandList()->BuildRaytracingAccelerationStructure(&topLevelBuildDescription, 0, nullptr);

  getCommandList()->Close();
  ID3D12CommandList* commandLists[] = {getCommandList().Get()};
  getCommandQueue()->ExecuteCommandLists(1, commandLists);

  // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
  waitForGPU();
}

std::vector<ComPtr<ID3D12Resource>> RayTracingRenderer::createBottomLevelAccelerationStructures()
{
  // create 3 instances of BLAS with the same geometry description
  ui8 numBlas = 3;
  m_bottomLevelAS.resize(numBlas);
  std::vector<ComPtr<ID3D12Resource>> bottomLevelScratchResources(numBlas);
  for (ui8 i = 0; i < numBlas; i++)
  {
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type                           = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer          = m_triangleIndexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount   = static_cast<UINT>(m_triangleIndexBuffer->GetDesc().Width) / sizeof(Index);
    geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount  = static_cast<UINT>(m_triangleVertexBuffer->GetDesc().Width) / sizeof(Vertex);
    geometryDesc.Triangles.VertexBuffer.StartAddress  = m_triangleVertexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
    geometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS  bottomLevelInputs       = {};
    bottomLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    bottomLevelInputs.NumDescs       = 1;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;

    getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    throwIfZero(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    AllocateUAVBuffer(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &bottomLevelScratchResources[i],
                      D3D12_RESOURCE_STATE_COMMON, L"BLASScratchResource");

    AllocateUAVBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAS[i],
                      D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"BottomLevelAccelerationStructure");

    // Bottom Level Acceleration Structure desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    {
      bottomLevelBuildDesc.Inputs                           = bottomLevelInputs;
      bottomLevelBuildDesc.ScratchAccelerationStructureData = bottomLevelScratchResources[i]->GetGPUVirtualAddress();
      bottomLevelBuildDesc.DestAccelerationStructureData    = m_bottomLevelAS[i]->GetGPUVirtualAddress();
    }

    getCommandList()->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
    auto uav = CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAS[i].Get());
    getCommandList()->ResourceBarrier(1, &uav);
  }

  return bottomLevelScratchResources;
}

ComPtr<ID3D12Resource> RayTracingRenderer::createTriangleInstanceDescriptions()
{
  auto                           device           = getDevice();
  D3D12_RAYTRACING_INSTANCE_DESC instanceDescs[3] = {};

  for (ui8 i = 0; i < _countof(instanceDescs); i++)
  {
    instanceDescs[i].InstanceID                          = i;
    instanceDescs[i].InstanceContributionToHitGroupIndex = 0;
    instanceDescs[i].Flags                               = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDescs[i].AccelerationStructure               = m_bottomLevelAS[i]->GetGPUVirtualAddress();
    instanceDescs[i].InstanceMask                        = 1;

    const auto worldSpaceTransformation = glm::transpose(m_triangleInstanceData[i].worldMatrix); // match row major
    instanceDescs[i].Transform[0][0]    = worldSpaceTransformation[0][0];
    instanceDescs[i].Transform[0][1]    = worldSpaceTransformation[0][1];
    instanceDescs[i].Transform[0][2]    = worldSpaceTransformation[0][2];
    instanceDescs[i].Transform[0][3]    = worldSpaceTransformation[0][3];

    instanceDescs[i].Transform[1][0] = worldSpaceTransformation[1][0];
    instanceDescs[i].Transform[1][1] = worldSpaceTransformation[1][1];
    instanceDescs[i].Transform[1][2] = worldSpaceTransformation[1][2];
    instanceDescs[i].Transform[1][3] = worldSpaceTransformation[1][3];

    instanceDescs[i].Transform[2][0] = worldSpaceTransformation[2][0];
    instanceDescs[i].Transform[2][1] = worldSpaceTransformation[2][1];
    instanceDescs[i].Transform[2][2] = worldSpaceTransformation[2][2];
    instanceDescs[i].Transform[2][3] = worldSpaceTransformation[2][3];
  }

  // Allocate upload buffer for instance descriptors.
  ComPtr<ID3D12Resource> instanceDescsBuffer;
  uploadDefaultBuffer(device, instanceDescs, sizeof(instanceDescs), instanceDescsBuffer, getCommandQueue(),
                      L"InstanceDescs");

  return instanceDescsBuffer;
}

D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC RayTracingRenderer::createTopLevelAccelerationStructure(
    ComPtr<ID3D12Resource> instanceDescriptionBuffer)
{
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
  topLevelInputs.DescsLayout                                          = D3D12_ELEMENTS_LAYOUT_ARRAY;
  topLevelInputs.Flags    = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  topLevelInputs.NumDescs = 3;
  topLevelInputs.Type     = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
  getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
  throwIfZero(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

  AllocateUAVBuffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_topLevelScratchResource,
                    D3D12_RESOURCE_STATE_COMMON, L"TLASScratchResource");

  AllocateUAVBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAS,
                    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"TopLevelAccelerationStructure");

  // Top Level Acceleration Structure desc
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
  {
    topLevelInputs.InstanceDescs                       = instanceDescriptionBuffer->GetGPUVirtualAddress();
    topLevelBuildDesc.Inputs                           = topLevelInputs;
    topLevelBuildDesc.DestAccelerationStructureData    = m_topLevelAS->GetGPUVirtualAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = m_topLevelScratchResource->GetGPUVirtualAddress();
  }

  return topLevelBuildDesc;
}

#pragma endregion

#pragma endregion

#pragma region OnDraw

void RayTracingRenderer::onDraw()
{
  if (!ImGui::GetIO().WantCaptureMouse)
  {
    bool pressed  = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right);
    if (pressed || released)
    {
      bool left = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
      m_examinerController.click(pressed, left == true ? 1 : 2,
                                 ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl),
                                 getNormalizedMouseCoordinates());
    }
    else
    {
      m_examinerController.move(getNormalizedMouseCoordinates());
    }
  }

  const auto commandList = getCommandList();
  const auto rtvHandle   = getRTVHandle();
  const auto dsvHandle   = getDSVHandle();
  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  f32v4 clearColor = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z, 1.0f};
  commandList->ClearRenderTargetView(rtvHandle, &clearColor.x, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  commandList->SetPipelineState(m_pipelineState.Get());
  commandList->SetGraphicsRootSignature(m_globalRootSignature.Get());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Bind TLAS used for ray tracing
  commandList->SetGraphicsRootShaderResourceView(0, m_topLevelAS->GetGPUVirtualAddress());

  // Bind root constants
  f32m4 viewMatrix       = m_examinerController.getTransformationMatrix();
  f32m4 projectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(30.0f), static_cast<f32>(getWidth()),
                                                    static_cast<f32>(getHeight()), 0.05f, 1000.0f);
  f32m4 mvpMatrix        = projectionMatrix * viewMatrix;
  commandList->SetGraphicsRoot32BitConstants(1, 16, &mvpMatrix, 0);
  commandList->SetGraphicsRoot32BitConstants(1, 3, &m_uiData.m_lightDirection, 16); // set light direction
  const auto shadowIntensity = 1.0f - m_uiData.m_shadowFactor;
  commandList->SetGraphicsRoot32BitConstants(1, 1, &shadowIntensity, 19); // set shadow intensity

  // First draw call (draw plane)
  ui8 drawPlane = 1;
  commandList->SetGraphicsRoot32BitConstants(1, 1, &drawPlane, 20);
  commandList->IASetVertexBuffers(0, 1, &m_planeVertexBufferView);
  commandList->IASetVertexBuffers(1, 1, &m_instanceBufferView); // not needed for plane, but still needs to be bound for
                                                                // its draw call, else it does not render
  commandList->IASetIndexBuffer(&m_planeIndexBufferView);
  commandList->DrawIndexedInstanced(m_numPlaneIndices, 1, 0, 0, 0);

  // Second draw call (draw triangle instances)
  drawPlane = 0;
  commandList->SetGraphicsRoot32BitConstants(1, 1, &drawPlane, 20);
  commandList->IASetVertexBuffers(0, 1, &m_triangleVertexBufferView);
  commandList->IASetIndexBuffer(&m_triangleIndexBufferView);
  commandList->DrawIndexedInstanced(3, 3, 0, 0, 0);
}

void RayTracingRenderer::onDrawUI()
{
  ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_None);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor.x);
  ImGui::SliderFloat3("Light Direction", &m_uiData.m_lightDirection.x, -1.0f, 1.0f);
  ImGui::SliderFloat("Shadow intensity", &m_uiData.m_shadowFactor, 0.0f, 1.0f);
  ImGui::End();
}

#pragma endregion

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 17 RayTracing";
  config.useVSync = true;
  try
  {
    RayTracingRenderer renderer(config);
    renderer.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
  }
  if (config.debug)
  {
    DX12Util::reportLiveObjects();
  }
  return 0;
}
