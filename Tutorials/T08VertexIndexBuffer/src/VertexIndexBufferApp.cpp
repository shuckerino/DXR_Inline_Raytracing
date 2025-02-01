#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <iostream>
using namespace gims;

class VertexIndexBufferApp : public DX12App
{
private:
  ComPtr<ID3D12PipelineState> m_pipelineState;  
  ComPtr<ID3D12RootSignature> m_rootSignature;
  ComPtr<ID3D12Resource>      m_vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW    m_vertexBufferView;
  ComPtr<ID3D12Resource>      m_indexBuffer;
  D3D12_INDEX_BUFFER_VIEW     m_indexBufferView;

  ui32 m_counter;

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
        compileShader(L"../../../Tutorials/T08VertexIndexBuffer/Shaders/VertexIndexBuffer.hlsl", L"VS_main", L"vs_6_0");

    const auto pixelShader =
        compileShader(L"../../../Tutorials/T08VertexIndexBuffer/Shaders/VertexIndexBuffer.hlsl", L"PS_main", L"ps_6_0");

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout                      = {inputElementDescs, _countof(inputElementDescs)};
    psoDesc.pRootSignature                   = m_rootSignature.Get();
    psoDesc.VS                               = HLSLCompiler::convert(vertexShader);
    psoDesc.PS                               = HLSLCompiler::convert(pixelShader);
    psoDesc.RasterizerState                  = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState                       = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState                = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask                       = UINT_MAX;
    psoDesc.PrimitiveTopologyType            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                 = 1;
    psoDesc.SampleDesc.Count                 = 1;
    psoDesc.RTVFormats[0]                    = getRenderTarget()->GetDesc().Format;
    psoDesc.DSVFormat                        = getDepthStencil()->GetDesc().Format;
    psoDesc.DepthStencilState.DepthEnable    = FALSE;
    psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_ALWAYS;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.StencilEnable  = FALSE;

    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
  }

  void createTriangleMesh()
  {
    struct Vertex
    {
      f32v3 position;
    };

    std::vector<Vertex> vertexBufferCPU = {{{0.0f, 0.25f, 0.0f}}, {{0.25f, -0.25f, 0.0f}}, {{-0.25f, -0.25f, 0.0f}}};
    std::vector<ui32>   indexBufferCPU  = {0, 1, 2};

    const auto vertexBufferCPUSizeInBytes = vertexBufferCPU.size() * sizeof(Vertex);
    const auto indexBufferCPUSizeInBytes  = indexBufferCPU.size() * sizeof(ui32);

    UploadHelper uploadBuffer(getDevice(), std::max(vertexBufferCPUSizeInBytes, indexBufferCPUSizeInBytes));

    const auto vertexBufferDesc      = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferCPUSizeInBytes);
    const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer));
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes    = (ui32)vertexBufferCPUSizeInBytes;
    m_vertexBufferView.StrideInBytes  = sizeof(Vertex);

    uploadBuffer.uploadBuffer(vertexBufferCPU.data(), m_vertexBuffer, vertexBufferCPUSizeInBytes, getCommandQueue());

    const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferCPUSizeInBytes);
    getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer));

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.SizeInBytes    = (ui32)indexBufferCPUSizeInBytes;
    m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

    uploadBuffer.uploadBuffer(indexBufferCPU.data(), m_indexBuffer, indexBufferCPUSizeInBytes, getCommandQueue());
  }

public:
  VertexIndexBufferApp(const DX12AppConfig createInfo)
      : DX12App(createInfo)
      , m_counter(0)
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
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
    m_counter++;
  }
};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 08 Vertex & Index Buffer";
  config.useVSync = true;
  try
  {
    VertexIndexBufferApp app(config);
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
