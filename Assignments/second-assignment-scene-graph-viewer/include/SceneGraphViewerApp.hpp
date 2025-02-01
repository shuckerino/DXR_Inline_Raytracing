#pragma once
#include "Scene.hpp"
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
using namespace gims;

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
  void createRootSignature();

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

  struct UiData
  {
    f32v3 m_backgroundColor = f32v3(0.25f, 0.25f, 0.25f);
  };

  ComPtr<ID3D12PipelineState>      m_pipelineState;
  ComPtr<ID3D12RootSignature>      m_rootSignature;
  std::vector<ConstantBufferD3D12> m_constantBuffers;
  gims::ExaminerController         m_examinerController;  
  Scene                            m_scene;
  UiData                           m_uiData;
};
