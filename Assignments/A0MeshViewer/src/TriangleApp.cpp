#include "TriangleApp.h"
#include <d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/sys/Event.hpp>
#include <imgui.h>
#include <iostream>
#include <vector>

using namespace gims;

namespace
{
f32m4 getNormalizationTransformation(f32v3 const* const positions, ui32 nPositions)
{
  if (nPositions == 0)
    return f32m4(1.0f);

  f32v3 minPos = positions[0];
  f32v3 maxPos = positions[0];

  // We need to translate the vertices to the center of the cuboid
  // Therefore we first need to calculate the center of mass of the vertices
  f32v3 centerOfMass(0.0f, 0.0f, 0.0f);
  for (ui32 i = 0; i < nPositions; i++)
  {
    minPos = glm::min(minPos, positions[i]);
    maxPos = glm::max(maxPos, positions[i]);
    centerOfMass += positions[i];
  }

  // cast to float to avoid integer division
  centerOfMass /= static_cast<float>(nPositions);
  f32m4 translation = glm::translate(glm::mat4(1.0f), -centerOfMass);

  // find largest dimension and scale accordingly
  const f32v3 distance         = maxPos - minPos;
  const f32   largestDimension = glm::max(distance.x, glm::max(distance.y, distance.z));
  const f32   scalingFactor    = (largestDimension != 0) ? (1.0f / largestDimension) : 1.0f;
  f32m4       scale_matrix     = glm::scale(glm::mat4(1.0f), f32v3(scalingFactor));

  // add 180 degree rotation around y-axis so bunny faces camera
  const f32m4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

  // Combine transformations
  f32m4 normalizationMatrix = rotationMatrix * scale_matrix * translation;

  return normalizationMatrix;
}
} // namespace

#pragma region Ctor

MeshViewer::MeshViewer(const DX12AppConfig config)
    : DX12App(config)
    , m_examinerController(true)
{
  // set ui data to defaults
  m_uiData.m_viewPortHeight          = static_cast<f32>(getHeight());
  m_uiData.m_viewPortWidth           = static_cast<f32>(getWidth());
  m_uiData.m_backgroundColor         = f32v4(0.6f, 0.6f, 0.6f, 1.0f);
  m_uiData.m_wireFrameColor          = f32v4(0.0f, 0.0f, 0.0f, 1.0f);
  m_uiData.m_ambientColor            = f32v4(0.0f, 0.0f, 0.0f, 1.0f);
  m_uiData.m_diffuseColor            = f32v4(1.0f, 1.0f, 1.0f, 1.0f);
  m_uiData.m_specularColor           = f32v3(1.0f, 1.0f, 1.0f);
  m_uiData.m_pointCloudColor         = f32v3(1.0f, 1.0f, 1.0f);
  m_uiData.m_exponent                = 128;
  m_uiData.m_wireFrameEnabled        = false;
  m_uiData.m_backFaceCullingEnabled  = false;
  m_uiData.m_twoSidedLightingEnabled = false;
  m_uiData.m_useTextureEnabled       = false;
  m_uiData.m_useFlatShading          = false;
  m_uiData.m_usePointCloud           = false;

  createRootSignature();

  createPipeline(false, false, false); // normal without back face culling
  createPipeline(true, false, false);  // normal with back face culling
  createPipeline(false, true, false);  // wire frame without back face culling
  createPipeline(true, true, false);   // wire frame with back face culling
  createPipeline(false, false, true);  // point cloud

  createConstantBufferForEachSwapchainFrame();
  m_examinerController.setTranslationVector(f32v3(0, 0, 3));

  // Load the model data from file
  CograBinaryMeshFile cbm("../../../data/bunny.cbm");

  // Test print some stats
  cbm.printAttributes(std::cout);
  cbm.printConstant(std::cout);
  std::cout << cbm.getNumTriangles();

  initializeVertexBuffer(&cbm);
  uploadVertexBufferToGPU();

  initializeIndexBuffer(&cbm);
  uploadIndexBufferToGPU();
  createTexture();
}

#pragma endregion

#pragma region Create

void MeshViewer::createRootSignature()
{
  // Root signature needs 2 parameters
  CD3DX12_ROOT_PARAMETER rootParameters[2] = {};

  // 1. Parameter: Shader Resource View for Texture
  CD3DX12_DESCRIPTOR_RANGE range {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0}; // 1 descriptor, register 0
  rootParameters[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);

  // 2. Parameter: Constant Buffer
  rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

  // Configure static sampler
  D3D12_STATIC_SAMPLER_DESC sampler = {};
  sampler.Filter                    = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler.AddressU                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.AddressV                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.AddressW                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.MipLODBias                = 0;
  sampler.MaxAnisotropy             = 0;
  sampler.ComparisonFunc            = D3D12_COMPARISON_FUNC_NEVER;
  sampler.BorderColor               = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler.MinLOD                    = 0.0f;
  sampler.MaxLOD                    = D3D12_FLOAT32_MAX;
  sampler.ShaderRegister            = 0;
  sampler.RegisterSpace             = 0;
  sampler.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

  // Now create the root signature with the two parameters and static sampler
  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
  descRootSignature.Init(_countof(rootParameters), rootParameters, 1, &sampler,
                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
}

void MeshViewer::createConstantBufferForEachSwapchainFrame()
{
  // here we need to create n constant buffers with n being the number of swap chain frames
  const auto numSwapchainFrames = getDX12AppConfig().frameCount;
  m_constantBuffers.resize(numSwapchainFrames);

  for (size_t i = 0; i < numSwapchainFrames; i++)
  {
    const auto constantBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer));
    const auto uploadHeapProperties      = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    getDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDescription,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(&m_constantBuffers[i]));
  }
}

void MeshViewer::createTexture()
{
  i32 textureWidth, textureHeight, textureComp;

  stbi_set_flip_vertically_on_load(1);
  std::unique_ptr<ui8, void (*)(void*)> image(
      stbi_load("../../../data/bunny.png", &textureWidth, &textureHeight, &textureComp, 4), &stbi_image_free);

  D3D12_RESOURCE_DESC textureDescription = {};
  textureDescription.MipLevels           = 1;
  textureDescription.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDescription.Width               = textureWidth;
  textureDescription.Height              = textureHeight;
  textureDescription.Flags               = D3D12_RESOURCE_FLAG_NONE;
  textureDescription.DepthOrArraySize    = 1;
  textureDescription.SampleDesc.Count    = 1;
  textureDescription.SampleDesc.Quality  = 0;
  textureDescription.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  // default heap properties (because frequently accessed)
  const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  throwIfFailed(getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDescription,
                                                     D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_texture)));

  // upload texture to GPU memory
  UploadHelper uploadHelper(getDevice(), GetRequiredIntermediateSize(m_texture.Get(), 0, 1));
  uploadHelper.uploadTexture(image.get(), m_texture, textureWidth, textureHeight, getCommandQueue());
  //uploadHelper.uploadDefaultBuffer();

  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors             = 1;
  desc.NodeMask                   = 0;
  desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  throwIfFailed(getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srv)));

  D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
  shaderResourceViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  shaderResourceViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  shaderResourceViewDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
  shaderResourceViewDesc.Texture2D.MipLevels             = 1;
  shaderResourceViewDesc.Texture2D.MostDetailedMip       = 0;
  shaderResourceViewDesc.Texture2D.ResourceMinLODClamp   = 0.0f;
  getDevice()->CreateShaderResourceView(m_texture.Get(), &shaderResourceViewDesc,
                                        m_srv->GetCPUDescriptorHandleForHeapStart());
}

#pragma region Pipelines

void MeshViewer::createPipeline(bool enableBackFaceCulling, bool enableWireFrameMode, bool enablePointCloudMode)
{
  ComPtr<IDxcBlob> vertexShader;
  ComPtr<IDxcBlob> pixelShader;

  if (enableWireFrameMode)
  {
    vertexShader =
        compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"VS_WireFrame_main", L"vs_6_0");

    pixelShader =
        compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"PS_WireFrame_main", L"ps_6_0");

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc   = {};
    psoDesc.InputLayout                          = {inputElementDescs, _countof(inputElementDescs)};
    psoDesc.pRootSignature                       = m_rootSignature.Get();
    psoDesc.VS                                   = HLSLCompiler::convert(vertexShader);
    psoDesc.PS                                   = HLSLCompiler::convert(pixelShader);
    psoDesc.RasterizerState                      = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode             = enableBackFaceCulling ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FillMode             = D3D12_FILL_MODE_WIREFRAME;
    psoDesc.BlendState                           = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState                    = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask                           = UINT_MAX;
    psoDesc.PrimitiveTopologyType                = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                     = 1;
    psoDesc.SampleDesc.Count                     = 1;
    psoDesc.RTVFormats[0]                        = getDX12AppConfig().renderTargetFormat;
    psoDesc.DSVFormat                            = getDX12AppConfig().depthBufferFormat;
    psoDesc.DepthStencilState.DepthEnable        = TRUE;
    psoDesc.DepthStencilState.DepthFunc          = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DepthStencilState.DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.StencilEnable      = FALSE;
    psoDesc.RasterizerState.DepthBias            = -1;
    psoDesc.RasterizerState.DepthBiasClamp       = -100.0f;
    psoDesc.RasterizerState.SlopeScaledDepthBias = -1.0f;

    if (enableBackFaceCulling)
    {
      throwIfFailed(getDevice()->CreateGraphicsPipelineState(
          &psoDesc, IID_PPV_ARGS(&m_wireFramePipelineStateWithBackFaceCulling)));
    }
    else
    {
      throwIfFailed(
          getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_wireFramePipelineStateWithNoCulling)));
    }
  }
  else if (enablePointCloudMode)
  {
    vertexShader =
        compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"VS_PointCloud", L"vs_6_0");

    pixelShader =
        compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"PS_PointCloud", L"ps_6_0");

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                        = {inputElementDescs, _countof(inputElementDescs)};
    psoDesc.pRootSignature                     = m_rootSignature.Get();
    psoDesc.VS                                 = HLSLCompiler::convert(vertexShader);
    psoDesc.PS                                 = HLSLCompiler::convert(pixelShader);
    psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_BACK;
    psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask                         = UINT_MAX;
    psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    psoDesc.NumRenderTargets                   = 1;
    psoDesc.SampleDesc.Count                   = 1;
    psoDesc.RTVFormats[0]                      = getDX12AppConfig().renderTargetFormat;
    psoDesc.DSVFormat                          = getDX12AppConfig().depthBufferFormat;
    psoDesc.DepthStencilState.DepthEnable      = TRUE;
    psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.StencilEnable    = FALSE;

    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pointCloudPipelineState)));
  }
  else
  {
    vertexShader = compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"VS_main", L"vs_6_0");

    pixelShader = compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"PS_main", L"ps_6_0");

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                        = {inputElementDescs, _countof(inputElementDescs)};
    psoDesc.pRootSignature                     = m_rootSignature.Get();
    psoDesc.VS                                 = HLSLCompiler::convert(vertexShader);
    psoDesc.PS                                 = HLSLCompiler::convert(pixelShader);
    psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode           = enableBackFaceCulling ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
    psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask                         = UINT_MAX;
    psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                   = 1;
    psoDesc.SampleDesc.Count                   = 1;
    psoDesc.RTVFormats[0]                      = getDX12AppConfig().renderTargetFormat;
    psoDesc.DSVFormat                          = getDX12AppConfig().depthBufferFormat;
    psoDesc.DepthStencilState.DepthEnable      = TRUE;
    psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.StencilEnable    = FALSE;

    if (enableBackFaceCulling)
    {
      throwIfFailed(
          getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateWithBackFaceCulling)));
    }
    else
    {
      throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateWithNoCulling)));
    }
  }
}
#pragma endregion

#pragma endregion

#pragma region Initialize

void MeshViewer::initializeVertexBuffer(const CograBinaryMeshFile* cbm)
{
  const auto numVertices         = cbm->getNumVertices();
  const auto vertexPointer       = cbm->getPositionsPtr();
  const auto normalPointer       = reinterpret_cast<f32v3*>(cbm->getAttributePtr(0));
  const auto texture_coordinates = reinterpret_cast<f32v2*>(cbm->getAttributePtr(1));

  if (numVertices % 3 != 0)
  {
    std::cerr << "The loaded vertex data is invalid. Please check for errors..." << std::endl;
    exit(1);
  }
  // convert simple float vector to vertex vector
  for (size_t i = 0; i < numVertices; i++)
  {
    Vertex v;
    v.position = {vertexPointer[i * 3], vertexPointer[i * 3 + 1], vertexPointer[i * 3 + 2]};
    v.normal   = normalPointer[i];
    v.texcoord = texture_coordinates[i];
    m_VertexBufferCPU.push_back(v);
  }

  m_vertexBufferSize = m_VertexBufferCPU.size() * sizeof(Vertex);
}

void MeshViewer::initializeIndexBuffer(const CograBinaryMeshFile* cbm)
{
  const auto numIndices         = cbm->getNumTriangles() * 3;
  const auto indexBufferPointer = cbm->getTriangleIndices();
  for (ui32 i = 0; i < numIndices; i++)
  {
    m_indexBuffer.push_back(indexBufferPointer[i]);
  }
  m_indexBufferSize = m_indexBuffer.size() * sizeof(ui32);
}

#pragma endregion

#pragma region Upload Methods

void MeshViewer::uploadVertexBufferToGPU()
{
  UploadHelper uploadHelper(getDevice(), m_vertexBufferSize);

  // 1. Create vertex buffer description (resource description)
  const auto vertexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);

  // 2. Create default heap properties (because frequently accessed by GPU)
  const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDescription,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBufferGPU));

  uploadHelper.uploadBuffer(m_VertexBufferCPU.data(), m_vertexBufferGPU, m_vertexBufferSize, getCommandQueue());
  m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = static_cast<ui32>(m_vertexBufferSize);
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);
}

void MeshViewer::uploadIndexBufferToGPU()
{
  UploadHelper uploadHelper(getDevice(), m_indexBufferSize);

  // 1. Create index buffer description
  const auto indexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferSize);

  // 2. Create upload heap properties (map memory, write from CPU and read from GPU)
  const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  getDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDescription,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBufferGPU));

  uploadHelper.uploadBuffer(m_indexBuffer.data(), m_indexBufferGPU, m_indexBufferSize, getCommandQueue());
  m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = static_cast<ui32>(m_indexBufferSize);
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;
}

#pragma endregion

#pragma region Per frame update

void MeshViewer::updateConstantBuffer()
{
  ConstantBuffer cb;
  // std::cout << "Wireframe color is" << to_string(m_uiData.m_wireFrameColor) << std::endl;
  //  cb.wireFrameColor = f32v4(m_uiData.m_wireFrameColor, 1.0f);
  updateUIData(&cb);

  // get vector of positions of vertices to pass to "getNormalizationTransformation"
  std::vector<f32v3> vertexPositions = std::vector<f32v3>();
  vertexPositions.resize(m_VertexBufferCPU.size());
  for (const Vertex& v : m_VertexBufferCPU)
  {
    vertexPositions.emplace_back(v.position);
  }

  f32m4 modelMatrix =
      getNormalizationTransformation(vertexPositions.data(), static_cast<ui32>(m_VertexBufferCPU.size()));

  f32m4 viewMatrix = m_examinerController.getTransformationMatrix();

  f32m4      projMatrix = glm::perspectiveFovLH_ZO(glm::radians(30.0f), static_cast<f32>(getWidth()),
                                                   static_cast<f32>(getHeight()), 0.05f, 1000.0f);
  const auto mv         = viewMatrix * modelMatrix;
  const auto mvp        = projMatrix * mv;

  // MVP matrix
  cb.mv  = mv;
  cb.mvp = mvp;

  const auto& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];
  void*       p;
  currentConstantBuffer->Map(0, nullptr, &p);
  ::memcpy(p, &cb, sizeof(cb));
  currentConstantBuffer->Unmap(0, nullptr);
}

void MeshViewer::updateUIData(ConstantBuffer* cb)
{
  cb->wireFrameColor               = f32v4(m_uiData.m_wireFrameColor, 1.0f);
  cb->ambientColor                 = f32v4(m_uiData.m_ambientColor, 1.0f);
  cb->diffuseColor                 = f32v4(m_uiData.m_diffuseColor, 1.0f);
  cb->specularColor_and_Exponent.x = m_uiData.m_specularColor.x;
  cb->specularColor_and_Exponent.y = m_uiData.m_specularColor.y;
  cb->specularColor_and_Exponent.z = m_uiData.m_specularColor.z;
  cb->specularColor_and_Exponent.w = static_cast<f32>(m_uiData.m_exponent);
  cb->pointCloudColor              = f32v4(m_uiData.m_pointCloudColor, 1.0f);
  cb->flags = (m_uiData.m_twoSidedLightingEnabled ? 1 : 0) | (m_uiData.m_useTextureEnabled ? 1 : 0) << 1 |
              (m_uiData.m_useFlatShading ? 1 : 0) << 2;
}

#pragma endregion

MeshViewer::~MeshViewer()
{
}

#pragma region OnDraw

void MeshViewer::onDraw()
{
  // do not draw if window is minimized
  if (m_uiData.m_viewPortHeight == 0 || m_uiData.m_viewPortWidth == 0)
  {
    return;
  }
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
  commandList->SetGraphicsRootSignature(
      m_rootSignature.Get()); // root signature needs to be set before accessing it in "SetGraphicsRootDescriptorTable"

  // texture
  commandList->SetDescriptorHeaps(1, m_srv.GetAddressOf());
  commandList->SetGraphicsRootDescriptorTable(0, m_srv->GetGPUDescriptorHandleForHeapStart());

  const float clearColor[] = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z,
                              1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  if (m_uiData.m_usePointCloud)
  {
    commandList->SetPipelineState(m_pointCloudPipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
  }
  else // normal draw call
  {
    commandList->SetPipelineState(m_uiData.m_backFaceCullingEnabled ? m_pipelineStateWithBackFaceCulling.Get()
                                                                    : m_pipelineStateWithNoCulling.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);
  commandList->SetGraphicsRootConstantBufferView(1, m_constantBuffers[getFrameIndex()]->GetGPUVirtualAddress());
  updateConstantBuffer();

  commandList->DrawIndexedInstanced(static_cast<ui32>(m_indexBuffer.size()), 1, 0, 0, 0);

  // check if wire frame should also be drawn (only if NOT in point cloud mode)
  if (!m_uiData.m_usePointCloud && m_uiData.m_wireFrameEnabled)
  {
    commandList->SetPipelineState(m_uiData.m_backFaceCullingEnabled ? m_wireFramePipelineStateWithBackFaceCulling.Get()
                                                                    : m_wireFramePipelineStateWithNoCulling.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->DrawIndexedInstanced(static_cast<ui32>(m_indexBuffer.size()), 1, 0, 0, 0);
  }
}

void MeshViewer::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;

  m_uiData.m_viewPortHeight = f32(getHeight());
  m_uiData.m_viewPortWidth  = f32(getWidth());

  // Configuration window
  ImGui::Begin("Configuration", nullptr, imGuiFlags);
  ImGui::Text("Frame time: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::Text("Viewport Width: %f", m_uiData.m_viewPortWidth);
  ImGui::Text("Viewport Height: %f", m_uiData.m_viewPortHeight);
  ImGui::Text("Aspect Ratio: %f", m_uiData.m_viewPortWidth / m_uiData.m_viewPortHeight);

  ImGui::Checkbox("Back-Face Culling", &m_uiData.m_backFaceCullingEnabled);
  ImGui::Checkbox("Overlay Wire Frame", &m_uiData.m_wireFrameEnabled);
  ImGui::ColorEdit3("Wire Frame Color", &m_uiData.m_wireFrameColor.x);
  ImGui::Checkbox("Two-Sided Lighting", &m_uiData.m_twoSidedLightingEnabled);
  ImGui::Checkbox("Flat Shading", &m_uiData.m_useFlatShading);
  ImGui::Checkbox("Use Texture", &m_uiData.m_useTextureEnabled);
  ImGui::Checkbox("Use Point Cloud", &m_uiData.m_usePointCloud);
  ImGui::ColorEdit3("Point Cloud Color", &m_uiData.m_pointCloudColor.x);
  ImGui::ColorEdit3("Ambient", &m_uiData.m_ambientColor.x);
  ImGui::ColorEdit3("Diffuse", &m_uiData.m_diffuseColor.x);
  ImGui::ColorEdit3("Specular", &m_uiData.m_specularColor.x);
  ImGui::SliderInt("Exponent", &m_uiData.m_exponent, 1, 256);
  ImGui::End();
}
#pragma endregion
