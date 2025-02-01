#include "SceneGraphViewerApp.hpp"
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

SceneGraphViewerApp::SceneGraphViewerApp(const DX12AppConfig config, const std::filesystem::path pathToScene)
    : DX12App(config)
    , m_examinerController(true)
    , m_scene(SceneGraphFactory::createFromAssImpScene(pathToScene, getDevice(), getCommandQueue()))
{
  m_examinerController.setTranslationVector(f32v3(0, -0.25f, 1.5));
  m_uiData.m_useNormalMapping = false;
  createRootSignature();
  createSceneConstantBuffer();
  createPipeline();
}

#pragma region Init

void SceneGraphViewerApp::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER   rootParameter[4] = {};
  CD3DX12_DESCRIPTOR_RANGE range            = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0};
  rootParameter[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // scene constant buffer
  rootParameter[1].InitAsConstants(16, 1, D3D12_ROOT_SIGNATURE_FLAG_NONE); // mv matrix
  rootParameter[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL); // materials
  rootParameter[3].InitAsDescriptorTable(1, &range); // textures

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
                                   IID_PPV_ARGS(&m_rootSignature));
}

void SceneGraphViewerApp::createPipeline()
{
  waitForGPU();
  const auto inputElementDescs = TriangleMeshD3D12::getInputElementDescriptors();

  const auto vertexShader =
      compileShader(L"../../../Assignments/A1SceneGraphViewer/Shaders/TriangleMesh.hlsl", L"VS_main", L"vs_6_0");
  const auto pixelShader =
      compileShader(L"../../../Assignments/A1SceneGraphViewer/Shaders/TriangleMesh.hlsl", L"PS_main", L"ps_6_0");

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {inputElementDescs.data(), (ui32)inputElementDescs.size()};
  psoDesc.pRootSignature                     = m_rootSignature.Get();
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
  ImGui::Begin("Information", nullptr, imGuiFlags);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();
  ImGui::Begin("Configuration", nullptr, imGuiFlags);
  ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor[0]);
  ImGui::Checkbox("Use normal mapping (not really working)", &m_uiData.m_useNormalMapping);
  ImGui::SliderFloat("Light x position", &m_uiData.m_lightDirection.x, -4.0f, 4.0f);
  ImGui::SliderFloat("Light y position", &m_uiData.m_lightDirection.y, -4.0f, 4.0f);
  ImGui::SliderFloat("Light z position", &m_uiData.m_lightDirection.z, -4.0f, 4.0f);
  ImGui::End();
}

void SceneGraphViewerApp::drawScene(const ComPtr<ID3D12GraphicsCommandList>& cmdLst)
{
  const auto cameraMatrix = m_examinerController.getTransformationMatrix();
  updateSceneConstantBuffer();

  //  Assignment 6 (normalize scene)
  const auto cameraAndNormalization = cameraMatrix * m_scene.getAABB().getNormalizationTransformation();

  cmdLst->SetPipelineState(m_pipelineState.Get());

  cmdLst->SetGraphicsRootSignature(m_rootSignature.Get());

  // set constant buffer
  const auto sceneCb = constantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  cmdLst->SetGraphicsRootConstantBufferView(0, sceneCb);

  m_scene.addToCommandList(cmdLst, cameraAndNormalization, 1, 2, 3);
}

#pragma endregion

#pragma region Constant Buffer

namespace
{
struct SceneConstantBuffer
{
  f32m4 projectionMatrix;
  f32v3 lightDirection;
  ui8   flags;
};

} // namespace

void SceneGraphViewerApp::createSceneConstantBuffer()
{
  const SceneConstantBuffer cb         = {};
  const auto                frameCount = getDX12AppConfig().frameCount;
  constantBuffers.resize(frameCount);
  for (ui32 i = 0; i < frameCount; i++)
  {
    constantBuffers[i] = ConstantBufferD3D12(cb, getDevice());
  }
}

void SceneGraphViewerApp::updateSceneConstantBuffer()
{
  SceneConstantBuffer cb;
  cb.lightDirection = m_uiData.m_lightDirection;
  cb.flags = m_uiData.m_useNormalMapping & 0x1;
  cb.projectionMatrix =
      glm::perspectiveFovLH_ZO<f32>(glm::radians(45.0f), (f32)getWidth(), (f32)getHeight(), 0.01f, 1000.0f);
  // std::cout << "Projection is " << glm::to_string(cb.projectionMatrix) << std::endl;
  constantBuffers[getFrameIndex()].upload(&cb);
}

#pragma endregion
