#include "SceneGraphViewerApp.hpp"
#include "RayTracingUtils.hpp"
#include "SceneFactory.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <gimslib/sys/Event.hpp>
#include <imgui.h>
#include <iostream>
#include <vector>
using namespace gims;

#define MAX_LIGHTS   8
#define MAX_TEXTURES 30

#define SCENE_CB_ROOT_INDEX         0
#define CONSTANTS_ROOT_INDEX        1
#define MATERIAL_CB_ROOT_INDEX      2
#define DESCRIPTOR_TABLE_ROOT_INDEX 3
#define TLAS_ROOT_INDEX             4
#define POINT_LIGHT_CB_ROOT_INDEX   5
#define AREA_LIGHT_CB_ROOT_INDEX    6

SceneGraphViewerApp::SceneGraphViewerApp(const DX12AppConfig config, const std::filesystem::path pathToScene)
    : DX12App(config)
    , m_examinerController(true)
    , m_scene(SceneGraphFactory::createFromAssImpScene(pathToScene, getDevice(), getCommandQueue()))
    , m_rayTracingUtils(RayTracingUtils::createRayTracingUtils(getDevice(), m_scene, getCommandList(),
                                                               getCommandAllocator(), getCommandQueue(), (*this)))
{
  m_examinerController.setTranslationVector(f32v3(0, -0.25f, 1.5));

  m_uiData.m_shadowBias       = 0.375f;
  m_uiData.m_numRays          = 16;
  m_uiData.m_samplingOffset   = 0.01f;
  m_uiData.m_minT             = 0.0001f;
  m_uiData.m_reflectionFactor = 0.5f;
  m_uiData.m_shadowFactor = 1.0f;
  m_uiData.m_useAreaLights    = false;
  m_uiData.m_useReflections   = false;

  createRootSignatures();
  createSceneConstantBuffer();
  createLightConstantBuffers();
  createPipeline();
}

#pragma region Init

void SceneGraphViewerApp::createRootSignatures()
{
  CD3DX12_ROOT_PARAMETER   rootParameter[7] = {};
  CD3DX12_DESCRIPTOR_RANGE descriptorRange  = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_TEXTURES + 2,
                                               1}; // vertex-b, index-b, textures
  rootParameter[SCENE_CB_ROOT_INDEX].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
  rootParameter[CONSTANTS_ROOT_INDEX].InitAsConstants(34, 1, D3D12_ROOT_SIGNATURE_FLAG_NONE); // mv matrix, etc
  rootParameter[MATERIAL_CB_ROOT_INDEX].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  rootParameter[DESCRIPTOR_TABLE_ROOT_INDEX].InitAsDescriptorTable(1, &descriptorRange);
  rootParameter[TLAS_ROOT_INDEX].InitAsShaderResourceView(0);
  rootParameter[POINT_LIGHT_CB_ROOT_INDEX].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  rootParameter[AREA_LIGHT_CB_ROOT_INDEX].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_PIXEL);

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

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
  rootSignatureDesc.Init(_countof(rootParameter), rootParameter, 1, &sampler,
                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_graphicsRootSignature));
}

void SceneGraphViewerApp::createPipeline()
{
  waitForGPU();

  const auto vertexShader =
      compileShader(L"../../../Assignments/RayTracing/Shaders/RayTracing.hlsl", L"VS_main", L"vs_6_8");
  const auto pixelShader =
      compileShader(L"../../../Assignments/RayTracing/Shaders/RayTracing.hlsl", L"PS_main", L"ps_6_8");

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.pRootSignature                     = m_graphicsRootSignature.Get();
  psoDesc.VS                                 = HLSLCompiler::convert(vertexShader);
  psoDesc.PS                                 = HLSLCompiler::convert(pixelShader);
  psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode           = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DSVFormat                          = getDX12AppConfig().depthBufferFormat;
  psoDesc.DepthStencilState.DepthEnable      = TRUE;
  psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable    = FALSE;
  psoDesc.SampleMask                         = UINT_MAX;
  psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                   = 1;
  psoDesc.RTVFormats[0]                      = getDX12AppConfig().renderTargetFormat;
  psoDesc.SampleDesc.Count                   = 1;
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

#pragma endregion

#pragma region OnDraw

void SceneGraphViewerApp::onDraw()
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

  const float clearColor[] = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z,
                              1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  drawScene(commandList);
}

void SceneGraphViewerApp::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  if (ImGui::Begin("Controls", nullptr, imGuiFlags))
  {
    ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
    ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor[0]);
    ImGui::SliderInt("Number of rays per pixel", &m_uiData.m_numRays, 1, 64);
    ImGui::SliderFloat("Shadow bias", &m_uiData.m_shadowBias, 0.0f, 5.0f);
    ImGui::SliderFloat("Random sampling offset", &m_uiData.m_samplingOffset, 0.0f, 1.0f);
    ImGui::SliderFloat("MinT", &m_uiData.m_minT, 0.0f, 1.0f);
    ImGui::SliderFloat("Reflection Factor", &m_uiData.m_reflectionFactor, 0.0f, 1.0f);
    ImGui::SliderFloat("Shadow Factor", &m_uiData.m_shadowFactor, 0.0f, 1.0f);
    ImGui::Checkbox("Use Area Lights", &m_uiData.m_useAreaLights);
    ImGui::Checkbox("Use Reflections", &m_uiData.m_useReflections);

    static i8   selectedLightIndex   = -1;
    static bool isPointLightSelected = true;

    // List Point Lights
    if (ImGui::TreeNode("Point Lights"))
    {
      for (i8 i = 0; i < m_pointLights.size(); ++i)
      {
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Selectable(("Light " + std::to_string(i + 1)).c_str(),
                              selectedLightIndex == i && isPointLightSelected))
        {
          selectedLightIndex   = i;
          isPointLightSelected = true;
        }
        ImGui::PopID();
      }
      ImGui::TreePop();
    }

    // List Area Lights
    if (ImGui::TreeNode("Area Lights"))
    {
      for (i8 i = 0; i < m_areaLights.size(); ++i)
      {
        ImGui::PushID(static_cast<int>(i + 100)); // Use a unique ID range for area lights
        if (ImGui::Selectable(("Light " + std::to_string(i + 1)).c_str(),
                              selectedLightIndex == i && !isPointLightSelected))
        {
          selectedLightIndex   = i;
          isPointLightSelected = false;
        }
        ImGui::PopID();
      }
      ImGui::TreePop();
    }

    // Add Lights
    if (ImGui::Button("Add Point Light (max. 8)"))
    {
      if (m_pointLights.size() < MAX_LIGHTS)
      {
        PointLight newLight = {};
        newLight.position   = {0.0f, 0.0f, 0.0f};
        newLight.color      = {1.0f, 1.0f, 1.0f};
        newLight.intensity  = 50.0f;
        m_pointLights.push_back(newLight);
        selectedLightIndex   = static_cast<i8>(m_pointLights.size() - 1);
        isPointLightSelected = true;
      }
    }

    if (ImGui::Button("Add Area Light (max. 8)"))
    {
      if (m_areaLights.size() < MAX_LIGHTS)
      {
        AreaLight newLight = {};
        newLight.position  = {0.0f, 0.0f, 0.0f};
        newLight.color     = {1.0f, 1.0f, 1.0f};
        newLight.intensity = 50.0f;
        newLight.normal    = {0.0f, -1.0f, 0.0f};
        newLight.width     = 1.0f;
        newLight.height    = 1.0f;
        m_areaLights.push_back(newLight);
        selectedLightIndex   = static_cast<i8>(m_areaLights.size() - 1);
        isPointLightSelected = false;
      }
    }

    // Remove Selected Light
    if (selectedLightIndex >= 0)
    {
      if (isPointLightSelected && selectedLightIndex < m_pointLights.size())
      {
        if (ImGui::Button("Remove Selected Point Light"))
        {
          m_pointLights.erase(m_pointLights.begin() + selectedLightIndex);
          selectedLightIndex = static_cast<ui8>(m_pointLights.size()) - 1;
        }
      }
      else if (!isPointLightSelected && selectedLightIndex < m_areaLights.size())
      {
        if (ImGui::Button("Remove Selected Area Light"))
        {
          m_areaLights.erase(m_areaLights.begin() + selectedLightIndex);
          selectedLightIndex = static_cast<ui8>(m_areaLights.size()) - 1;
        }
      }
    }

    // Edit Selected Light
    if (selectedLightIndex >= 0)
    {
      if (isPointLightSelected && selectedLightIndex < m_pointLights.size())
      {
        PointLight& light = m_pointLights[selectedLightIndex];
        ImGui::Text("Editing Point Light %d", selectedLightIndex + 1);
        ImGui::DragFloat3("Position", reinterpret_cast<float*>(&light.position), 0.5f, -100.0f, 100.0f);
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 100.0f);
        ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&light.color));
      }
      else if (!isPointLightSelected && selectedLightIndex < m_areaLights.size())
      {
        AreaLight& light = m_areaLights[selectedLightIndex];
        ImGui::Text("Editing Area Light %d", selectedLightIndex + 1);
        ImGui::DragFloat3("Position", reinterpret_cast<float*>(&light.position), 0.5f, -100.0f, 100.0f);
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 100.0f);
        ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&light.color));
        ImGui::DragFloat3("Normal", reinterpret_cast<float*>(&light.normal), 0.1f, -1.0f, 1.0f);
        ImGui::SliderFloat("Width", &light.width, 0.0f, 100.0f);
        ImGui::SliderFloat("Height", &light.height, 0.0f, 100.0f);
      }
    }
  }
  ImGui::End();
}

void SceneGraphViewerApp::drawScene(const ComPtr<ID3D12GraphicsCommandList>& cmdLst)
{
  auto device = getDevice();

  updateSceneConstantBuffer();
  updateLightConstantBuffers();

  cmdLst->SetPipelineState(m_pipelineState.Get());
  cmdLst->SetGraphicsRootSignature(m_graphicsRootSignature.Get());

  // set constant buffers
  const auto sceneCb = m_sceneConstantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  cmdLst->SetGraphicsRootConstantBufferView(SCENE_CB_ROOT_INDEX, sceneCb);
  const auto pointlightCb = m_pointLightConstantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  cmdLst->SetGraphicsRootConstantBufferView(POINT_LIGHT_CB_ROOT_INDEX, pointlightCb);
  const auto arealightCb = m_areaLightConstantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  cmdLst->SetGraphicsRootConstantBufferView(AREA_LIGHT_CB_ROOT_INDEX, arealightCb);

  // ray tracing
  cmdLst->SetGraphicsRootShaderResourceView(TLAS_ROOT_INDEX, m_rayTracingUtils.m_topLevelAS->GetGPUVirtualAddress());

  // set global descriptor heap
  cmdLst->SetDescriptorHeaps(1, m_scene.m_globalDescriptorHeap.GetAddressOf());

  // for reflection mapping we need to make all vertices and indices available (also textures)
  CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(
      m_scene.m_globalDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0,
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

  cmdLst->SetGraphicsRootDescriptorTable(DESCRIPTOR_TABLE_ROOT_INDEX, descriptorHandle);

  cmdLst->IASetIndexBuffer(&m_scene.m_indexBufferView);

  const f32m4 cameraAndNormalization =
      m_examinerController.getTransformationMatrix() * m_scene.getAABB().getNormalizationTransformation();

  m_scene.addToCommandList(cmdLst, cameraAndNormalization, CONSTANTS_ROOT_INDEX);
}

#pragma endregion

#pragma region Constant Buffer

namespace
{
struct SceneConstantBuffer
{
  f32m4 projectionMatrix;
  f32m4 inverseViewMatrix;
  f32   shadowBias;
  f32v3 environmentColor;
  int   numRays;
  f32   samplingOffset;
  f32   minT;
  f32   reflectionFactor;
  f32   shadowFactor;
  int   flags;
};

struct PointLightConstantBuffer
{
  PointLight pointLights[MAX_LIGHTS];
  ui32       numPointLights;
};

struct AreaLightConstantBuffer
{
  AreaLight areaLights[MAX_LIGHTS];
  ui32      numAreaLights;
};

} // namespace

#pragma region Scene constant buffer

void SceneGraphViewerApp::createSceneConstantBuffer()
{
  const SceneConstantBuffer cb         = {};
  const auto                frameCount = getDX12AppConfig().frameCount;
  m_sceneConstantBuffers.resize(frameCount);
  for (ui32 i = 0; i < frameCount; i++)
  {
    m_sceneConstantBuffers[i] = ConstantBufferD3D12(cb, getDevice());
  }
}

void SceneGraphViewerApp::updateSceneConstantBuffer()
{
  SceneConstantBuffer cb;

  cb.shadowBias       = m_uiData.m_shadowBias;
  cb.numRays          = m_uiData.m_numRays;
  cb.reflectionFactor = m_uiData.m_reflectionFactor;
  cb.shadowFactor     = m_uiData.m_shadowFactor;
  cb.flags            = (ui8)m_uiData.m_useAreaLights | ((ui8)m_uiData.m_useReflections << 1);
  cb.samplingOffset   = m_uiData.m_samplingOffset;
  cb.minT             = m_uiData.m_minT;
  cb.environmentColor = m_uiData.m_backgroundColor;
  cb.projectionMatrix =
      glm::perspectiveFovLH_ZO<f32>(glm::radians(45.0f), (f32)getWidth(), (f32)getHeight(), 0.01f, 1000.0f);
  cb.inverseViewMatrix = glm::inverse(m_examinerController.getTransformationMatrix());
  m_sceneConstantBuffers[getFrameIndex()].upload(&cb);
}

#pragma endregion

#pragma region Light Constant Buffer

void SceneGraphViewerApp::createLightConstantBuffers()
{
  const auto frameCount = getDX12AppConfig().frameCount;

  {
    const PointLightConstantBuffer cb = {};
    m_pointLightConstantBuffers.resize(frameCount);
    for (ui32 i = 0; i < frameCount; i++)
    {
      m_pointLightConstantBuffers[i] = ConstantBufferD3D12(cb, getDevice());
    }
  }

  {
    const AreaLightConstantBuffer cb = {};
    m_areaLightConstantBuffers.resize(frameCount);
    for (ui32 i = 0; i < frameCount; i++)
    {
      m_areaLightConstantBuffers[i] = ConstantBufferD3D12(cb, getDevice());
    }
  }

  PointLight p1;
  p1.position  = f32v3(-20.0f, 45.0f, -54.0f);
  p1.color     = f32v3(1.0f, 1.0f, 1.0f);
  p1.intensity = 20.0f;

  PointLight p2;
  p2.position  = f32v3(32.0f, 15.0f, -21.0f);
  p2.color     = f32v3(1.0f, 1.0f, 1.0f);
  p2.intensity = 20.0f;

  m_pointLights.push_back(p1);
  m_pointLights.push_back(p2);

  AreaLight a1;
  a1.position  = f32v3(0.0f, 100.0f, 0.0f);
  a1.normal    = f32v3(0.0f, 1.0f, 0.0f);
  a1.color     = f32v3(1.0f, 1.0f, 1.0f);
  a1.intensity = 50.0f;
  a1.width     = 4.0f;
  a1.height    = 4.0f;

  m_areaLights.push_back(a1);
}

void SceneGraphViewerApp::updateLightConstantBuffers()
{
  {
    PointLightConstantBuffer cb;

    // update point lights
    cb.numPointLights = static_cast<ui32>(m_pointLights.size());
    for (ui8 i = 0; i < m_pointLights.size(); i++)
    {
      cb.pointLights[i] = m_pointLights.at(i);
    }
    m_pointLightConstantBuffers[getFrameIndex()].upload(&cb);
  }

  {
    AreaLightConstantBuffer cb;
    // update area lights
    cb.numAreaLights = static_cast<ui32>(m_areaLights.size());
    for (ui8 i = 0; i < m_areaLights.size(); i++)
    {
      cb.areaLights[i] = m_areaLights.at(i);
    }
    m_areaLightConstantBuffers[getFrameIndex()].upload(&cb);
  }
}

#pragma endregion

#pragma endregion
