#pragma once

#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class RayTracingRenderer : public DX12App
{
public:
  RayTracingRenderer(const DX12AppConfig createInfo);

  struct Vertex
  {
    f32v3 position;
  };

  struct InstanceData
  {
    f32m4 worldMatrix;
  };

  struct UiData
  {
    f32v3 m_backgroundColor;
    f32v3 m_lightDirection;
    f32   m_shadowFactor;
  };

  /// <summary>
  /// Called whenever a new frame is drawn.
  /// </summary>
  virtual void onDraw();

  /// <summary>
  /// Draw UI onto of rendered result.
  /// </summary>
  virtual void onDrawUI();

private:
  // Root signatures
  ComPtr<ID3D12RootSignature> m_globalRootSignature;

  // Rasterizing pipeline
  ComPtr<ID3D12PipelineState> m_pipelineState;

  // Camera
  gims::ExaminerController m_examinerController;

  // Acceleration structure
  ComPtr<ID3D12Resource> m_topLevelAS;
  ComPtr<ID3D12Resource>
      m_topLevelScratchResource; // scratch resource needs to be kept alive until command list execution
  std::vector<ComPtr<ID3D12Resource>> m_bottomLevelAS;

  // Ui data
  UiData m_uiData;

  // Triangle geometry
  typedef ui32             Index;
  ui32                     m_numTriangleIndices;      //! Num indices for triangle
  InstanceData             m_triangleInstanceData[3]; //! Instance data for triangle
  ComPtr<ID3D12Resource>   m_triangleIndexBuffer;
  ComPtr<ID3D12Resource>   m_triangleVertexBuffer;
  ComPtr<ID3D12Resource>   m_instanceBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_instanceBufferView;
  D3D12_VERTEX_BUFFER_VIEW m_triangleVertexBufferView;
  D3D12_INDEX_BUFFER_VIEW  m_triangleIndexBufferView;

  // Plane geometry
  ui32                     m_numPlaneIndices; //! Num indices for plane
  ComPtr<ID3D12Resource>   m_planeVertexBuffer;
  ComPtr<ID3D12Resource>   m_planeIndexBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_planeVertexBufferView;
  D3D12_INDEX_BUFFER_VIEW  m_planeIndexBufferView;

  /// <summary>
  /// Checks if ray tracing is supported on the current device
  /// </summary>
  /// <returns></returns>
  bool isRayTracingSupported();

  /// <summary>
  /// Starting method to setup the demo scene
  /// </summary>
  void createResources();

  /// <summary>
  /// Creating and setting up the root signature
  /// </summary>
  void createRootSignature();

  /// <summary>
  /// Creating the rasterizing pipeline
  /// </summary>
  void createPipeline();

  /// <summary>
  /// Creating all geometries needed for the scene
  /// </summary>
  void createGeometries();

  /// <summary>
  /// Creating buffer and buffer views for the triangle geometry
  /// </summary>
  /// <param name="device"></param>
  /// <param name="commandQueue"></param>
  void createTriangleInstances(ComPtr<ID3D12Device5> device, ComPtr<ID3D12CommandQueue> commandQueue);

  /// <summary>
  /// Creating buffer and buffer views for the plane geometry
  /// </summary>
  /// <param name="device"></param>
  /// <param name="commandQueue"></param>
  void createPlaneGeometry(ComPtr<ID3D12Device5> device, ComPtr<ID3D12CommandQueue> commandQueue);

  /// <summary>
  /// Creating the bottom and top level acceleration structures needed for ray tracing
  /// </summary>
  void createAccelerationStructures();

  /// <summary>
  /// Creating bottom level acceleration structures for geometry
  /// </summary>
  /// <returns>Scratch resources for BLAS, because they may not be deleted until actual command list execution</returns>
  std::vector<ComPtr<ID3D12Resource>> createBottomLevelAccelerationStructures();

  /// <summary>
  /// Create instance descriptions for the triangle geometry
  /// </summary>
  /// <returns>Instance Description Buffer</returns>
  ComPtr<ID3D12Resource> createTriangleInstanceDescriptions();

  /// <summary>
  /// Create top level acceleration structure
  /// </summary>
  /// <param name="instanceDescriptionBuffer"></param>
  /// <returns>Build description for top level acceleration structure</returns>
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC createTopLevelAccelerationStructure(
      ComPtr<ID3D12Resource> instanceDescriptionBuffer);

  /// <summary>
  /// Helper method to create UAV buffer
  /// </summary>
  /// <param name="bufferSize"></param>
  /// <param name="ppResource"></param>
  /// <param name="initialResourceState"></param>
  /// <param name="resourceName"></param>
  void AllocateUAVBuffer(ui64 bufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialResourceState,
                         const wchar_t* resourceName);
};
