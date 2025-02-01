#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <iostream>
using namespace gims;

class ConstantBufferApp : public DX12App
{
private:
  ComPtr<ID3D12PipelineState> m_pipelineState;
  
  ComPtr<ID3D12RootSignature> m_rootSignature;
  ui32                        m_counter;

  void createRootSignature()
  {
    CD3DX12_ROOT_PARAMETER parameter = {};

    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    parameter.InitAsDescriptorTable(1, &cbvTable);

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init(1, &parameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> rootBlob, errorBlob;
    D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_rootSignature));
  }

  void createPipeline()
  {

    const auto vertexShader =
        compileShader(L"../../../Tutorials/T10ConstantBuffer/Shaders/ConstantBuffer.hlsl", L"VS_main", L"vs_6_0");

    const auto pixelShader =
        compileShader(L"../../../Tutorials/T10ConstantBuffer/Shaders/ConstantBuffer.hlsl", L"PS_main", L"ps_6_0");


    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                        = {};
    psoDesc.pRootSignature                     = m_rootSignature.Get();
    psoDesc.VS                                 = HLSLCompiler::convert(vertexShader);
    psoDesc.PS                                 = HLSLCompiler::convert(pixelShader);
    psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask                         = UINT_MAX;
    psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                   = 1;
    psoDesc.SampleDesc.Count                   = 1;
    psoDesc.RTVFormats[0]                      = getRenderTarget()->GetDesc().Format;
    psoDesc.DSVFormat                          = getDepthStencil()->GetDesc().Format;
    psoDesc.DepthStencilState.DepthEnable      = FALSE;
    psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
    psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.StencilEnable    = FALSE;

    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
  }

  struct ConstantBuffer
  {
    f32m4 rotation;
  };

  std::vector<ComPtr<ID3D12Resource>> m_constantBuffers;
  ComPtr<ID3D12DescriptorHeap>        m_cbv;

  void createConstantBuffer()
  {
    const ConstantBuffer cb         = {};
    const auto           frameCount = getDX12AppConfig().frameCount;
    m_constantBuffers.resize(frameCount);
    for (ui32 i = 0; i < frameCount; i++)
    {
      static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
      static const auto constantBufferDesc   = CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstantBuffer) + 0xff) & ~0xff);
      getDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
                                           D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                           IID_PPV_ARGS(&m_constantBuffers[i]));
    }

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors             = frameCount;
    desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    throwIfFailed(getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_cbv)));

    for (ui32 i = 0; i < frameCount; i++)
    {
      D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
      cbvDesc.BufferLocation                  = m_constantBuffers[i]->GetGPUVirtualAddress();
      cbvDesc.SizeInBytes                     = (sizeof(ConstantBuffer) + 0xff) & ~0xff;
      CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(
          m_cbv->GetCPUDescriptorHandleForHeapStart(), i,
          getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
      getDevice()->CreateConstantBufferView(&cbvDesc, cbvHandle);
    }
  }

  void updateConstantBuffer()
  {
    ConstantBuffer cb;
    cb.rotation = glm::eulerAngleZ(glm::radians(m_counter / 60.0f) * 360.0f);

    const auto& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];
    void*       p;
    currentConstantBuffer->Map(0, nullptr, &p);
    ::memcpy(p, &cb, sizeof(cb));
    currentConstantBuffer->Unmap(0, nullptr);
  }

public:
  ConstantBufferApp(const DX12AppConfig createInfo)
      : DX12App(createInfo)
      , m_counter(0)
  {
    createRootSignature();
    createPipeline();
    createConstantBuffer();
  }

  virtual void onDraw()
  {
    updateConstantBuffer();
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

    ID3D12DescriptorHeap* heaps[] = {m_cbv.Get()};
  
    commandList->SetDescriptorHeaps(1, heaps);
    
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbv->GetGPUDescriptorHandleForHeapStart());
    cbvHandle.Offset(getFrameIndex(),
                     getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    
    commandList->DrawInstanced(3, 1, 0, 0);
    m_counter++;
  }
};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 10 Constant Buffer";
  config.useVSync = true;
  try
  {
    ConstantBufferApp app(config);
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
