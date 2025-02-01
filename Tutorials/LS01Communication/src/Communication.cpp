#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
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
  float                         X = 1.0f;
  // VertexBuffer
  std::vector<Vertex> m_vertexBufferCPU = {{{0.0f, X, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                                            {{X, -X, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                                            {{-X, -X, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};
  size_t              m_vertexBufferSize = m_vertexBufferCPU.size() * sizeof(Vertex);

  // Get com pointer for GPU ressource
  ComPtr<ID3D12Resource> m_vertexBufferGPU;

  void createTriangleMesh()
  {
    UploadHelper uploadBuffer(getDevice(), m_vertexBufferSize);

    // 1. Create Description with length of vertex buffer
    const CD3DX12_RESOURCE_DESC vertexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);

    // 2. Configure heap properties
    const CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDescription,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBufferGPU));

    uploadBuffer.uploadBuffer(m_vertexBufferCPU.data(), m_vertexBufferGPU, m_vertexBufferSize, getCommandQueue());

  }

  void createRootSignature()
  {
    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootBlob, errorBlob;
    D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);


    getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_rootSignature));
  }

  void createPipeline()
  {
    const auto vertexShader =
        compileShader(L"../../../Tutorials/LS01Communication/Shaders/Communication.hlsl", L"VS_main", L"vs_6_0");
    
    const auto pixelShader =
        compileShader(L"../../../Tutorials/LS01Communication/Shaders/Communication.hlsl", L"PS_main", L"ps_6_0");

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                        = {inputElementDescs, _countof(inputElementDescs)};
    psoDesc.pRootSignature                     = m_rootSignature.Get();
    psoDesc.VS                                 = HLSLCompiler::convert(vertexShader);
    psoDesc.PS                                 = HLSLCompiler::convert(pixelShader);
    psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable      = FALSE;
    psoDesc.DepthStencilState.StencilEnable    = FALSE;
    psoDesc.SampleMask                         = UINT_MAX;
    psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                   = 1;
    psoDesc.SampleDesc.Count                   = 1;
    psoDesc.RTVFormats[0]                      = getRenderTarget()->GetDesc().Format;
    psoDesc.DSVFormat                          = getDepthStencil()->GetDesc().Format;
    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
  }

public:
  // Constructor
  VertexBuffer(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {
    createRootSignature();
    createPipeline();
    createTriangleMesh();
  }

  virtual void onDraw()
  {
    const auto commandList = getCommandList();
    const auto rtvHandle   = getRTVHandle();
    const auto dsvHandle   = getDSVHandle();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    f32v4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    commandList->ClearRenderTargetView(rtvHandle, &clearColor.x, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    commandList->RSSetViewports(1, &getViewport());
    commandList->RSSetScissorRects(1, &getRectScissor());

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes    = (ui32)m_vertexBufferSize;
    m_vertexBufferView.StrideInBytes  = sizeof(Vertex);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

    commandList->DrawInstanced(3, 1, 0, 0);
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
