#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <iostream>
using namespace gims;

struct Vertex
{
  f32v3 position;
  f32v4 color;
};

class VertexBuffer : public DX12App
{
private:
  ComPtr<ID3D12PipelineState> m_pipelineState;
  ComPtr<ID3D12RootSignature> m_rootSignature;
  float                       X                  = 1.0f;
  std::vector<Vertex>         m_vertexBufferCPU  = {{{0.0f, X, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                                                    {{X, -X, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                                                    {{-X, -X, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};
  size_t                      m_vertexBufferSize = m_vertexBufferCPU.size() * sizeof(Vertex);

  ComPtr<ID3D12Resource> m_vertexBufferGPU;

  void uploadVertexBuffer()
  {
    UploadHelper uploadBuffer(getDevice(), m_vertexBufferSize);

    // Create resource description
    CD3DX12_RESOURCE_DESC vertexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);

    // Configure heap properties
    CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDescription,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBufferGPU));

    uploadBuffer.uploadBuffer(m_vertexBufferCPU.data(), m_vertexBufferGPU, m_vertexBufferSize, getCommandQueue());
  }

  void createRootSignature()
  {
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    // serialize root signature
    ComPtr<ID3DBlob> rootBlob, errorBlob;
    D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    // create on GPU
    getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_rootSignature));
  }

  void createPipeline()
  {
  }

public:
  VertexBuffer(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {
    createRootSignature();
    createPipeline();
    uploadVertexBuffer();
  }

  virtual void onDraw()
  {
  }
};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Triangle with CPU upload";
  config.useVSync = true;
  try
  {
    VertexBuffer app(config);
    app.run();
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
