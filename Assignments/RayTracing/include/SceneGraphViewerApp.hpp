#pragma once
#include "RayTracingUtils.hpp"
#include "Scene.hpp"
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
using namespace gims;

struct PointLight
{
  f32v3 position;
  f32   intensity;

  f32v3 color;
  f32   padding; // 4 bytes to align to 16 bytes
};

struct AreaLight
{
  f32v3 position;
  f32   intensity;

  f32v3 color;
  f32   width;

  f32v3 normal;
  f32   height;
};

/// <summary>
/// An app for viewing an Asset Importer Scene Graph.
/// </summary>
class SceneGraphViewerApp : public gims::DX12App
{
public:
  /// <summary>
  /// Creates the SceneGraphViewerApp and loads a scene.
  /// </summary>
  /// <param name="config">Configuration.</param>
  SceneGraphViewerApp(const DX12AppConfig config, const std::filesystem::path pathToScene);

  ~SceneGraphViewerApp() = default;

  /// <summary>
  /// Called whenever a new frame is drawn.
  /// </summary>
  virtual void onDraw();

  /// <summary>
  /// Draw UI onto of rendered result.
  /// </summary>
  virtual void onDrawUI();

private:
  /// <summary>
  /// Root signature connecting shader and GPU resources.
  /// </summary>
  void createRootSignatures();

  /// <summary>
  /// Creates the pipeline
  /// </summary>
  void createPipeline();

  /// <summary>
  /// Draws the scene.
  /// </summary>
  /// <param name="commandList">Command list to which we upload the buffer</param>
  void drawScene(const ComPtr<ID3D12GraphicsCommandList>& commandList);

  void createSceneConstantBuffer();
  void updateSceneConstantBuffer();
  void createLightConstantBuffers();
  void updateLightConstantBuffers();

  struct UiData
  {
    f32v3 m_backgroundColor = f32v3(0.25f, 0.25f, 0.25f);
    f32   m_shadowBias      = 0.0001f;
    int   m_numRays;
    f32   m_samplingOffset;
    f32   m_minT;
    f32   m_reflectionFactor;
    f32   m_shadowFactor;
    bool  m_useAreaLights;
    bool  m_useReflections;
  };

  ComPtr<ID3D12PipelineState>      m_pipelineState;
  ComPtr<ID3D12RootSignature>      m_graphicsRootSignature;
  std::vector<ConstantBufferD3D12> m_sceneConstantBuffers;
  std::vector<ConstantBufferD3D12> m_pointLightConstantBuffers;
  std::vector<ConstantBufferD3D12> m_areaLightConstantBuffers;
  std::vector<PointLight>          m_pointLights;
  std::vector<AreaLight>           m_areaLights;
  gims::ExaminerController         m_examinerController;
  Scene                            m_scene;
  UiData                           m_uiData;
  RayTracingUtils                  m_rayTracingUtils;
};
