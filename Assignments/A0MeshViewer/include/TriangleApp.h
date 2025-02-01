#pragma once
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>

using namespace gims;
class MeshViewer : public gims::DX12App
{
public:
  MeshViewer(const DX12AppConfig config);

  ~MeshViewer();

  virtual void onDraw();
  virtual void onDrawUI();

private:
  struct UiData
  {
    f32v3 m_backgroundColor;
    f32v3 m_wireFrameColor;
    f32v3 m_ambientColor;
    f32v3 m_diffuseColor;
    f32v3 m_specularColor;
    f32v3 m_pointCloudColor;
    i32   m_exponent;
    f32   m_viewPortWidth;
    f32   m_viewPortHeight;
    bool  m_wireFrameEnabled;
    bool  m_backFaceCullingEnabled;
    bool  m_twoSidedLightingEnabled;
    bool  m_useTextureEnabled;
    bool  m_useFlatShading;
    bool  m_usePointCloud;
  };
  struct Vertex
  {
    f32v3 position;
    f32v3 normal;
    f32v2 texcoord; // Texture coordinates
  };

  struct ConstantBuffer
  {
    f32m4 mvp;
    f32m4 mv;
    f32v4 specularColor_and_Exponent;
    f32v4 ambientColor;
    f32v4 diffuseColor;
    f32v4 wireFrameColor;
    f32v4 pointCloudColor;
    ui32  flags;
  };

  gims::ExaminerController m_examinerController;
  f32m4                    m_normalizationTransformation;
  std::vector<Vertex>      m_VertexBufferCPU;
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
  size_t                   m_vertexBufferSize;
  ComPtr<ID3D12Resource>   m_vertexBufferGPU;

  ComPtr<ID3D12RootSignature> m_rootSignature;

  // all pipeline states
  ComPtr<ID3D12PipelineState> m_pipelineStateWithNoCulling, m_wireFramePipelineStateWithNoCulling,
      m_pipelineStateWithBackFaceCulling, m_wireFramePipelineStateWithBackFaceCulling, m_pointCloudPipelineState;

  std::vector<ComPtr<ID3D12Resource>> m_constantBuffers;

  std::vector<ui32>       m_indexBuffer;
  ComPtr<ID3D12Resource>  m_indexBufferGPU;
  size_t                  m_indexBufferSize;
  D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

  ComPtr<ID3D12Resource>       m_texture;
  ComPtr<ID3D12DescriptorHeap> m_srv;

  UiData m_uiData;

  void createRootSignature();
  void createConstantBufferForEachSwapchainFrame();
  void createTexture();
  void createPipeline(bool enableBackFaceCulling, bool enableWireFrameMode, bool enablePointCloudMode);

  void initializeVertexBuffer(const CograBinaryMeshFile* cbm);
  void initializeIndexBuffer(const CograBinaryMeshFile* cbm);

  void uploadVertexBufferToGPU();
  void uploadIndexBufferToGPU();

  void updateConstantBuffer();
  void updateUIData(ConstantBuffer* cb);
};
