#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <iostream>
using namespace gims;

class ComputeShaderWriteToRenderTarget : public DX12App
{
private:
  ComPtr<ID3D12PipelineState> m_computePipelineState;  

  ComPtr<ID3D12PipelineState> m_graphicsPipelineSate;  
  
  ComPtr<ID3D12RootSignature> m_computeRootSignature;
  ComPtr<ID3D12RootSignature> m_graphicsRootSignature;

  ComPtr<ID3D12Resource> m_offscreenTarget;

  CD3DX12_CPU_DESCRIPTOR_HANDLE m_offscreenTarget_CPU_UAV;
  CD3DX12_GPU_DESCRIPTOR_HANDLE m_offscreenTarget_GPU_UAV;

  CD3DX12_CPU_DESCRIPTOR_HANDLE m_offscreenTarget_CPU_RTV;  

  ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
  ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;

  void createGraphicsRootSignature()
  {
    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> rootBlob, errorBlob;
    D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_graphicsRootSignature));
  }

  void createComputeRootSignature()
  {
    CD3DX12_ROOT_PARAMETER   parameters[2] = {};
    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    parameters[0].InitAsDescriptorTable(1, &uavTable);
    parameters[1].InitAsConstants(2, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init(_countof(parameters), parameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> rootBlob, errorBlob;
    D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_computeRootSignature));
  }

  void createGraphicsPipeline()
  {



    const auto vertexShader =
            compileShader(L"../../../Tutorials/T16ComputeShaderPostProcess/Shaders/Triangle.hlsl", L"VS_main", L"vs_6_0");

        const auto pixelShader =
            compileShader(L"../../../Tutorials/T16ComputeShaderPostProcess/Shaders/Triangle.hlsl", L"PS_main", L"ps_6_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                        = {};
    psoDesc.pRootSignature                     = m_graphicsRootSignature.Get();
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
    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_graphicsPipelineSate)));
  }

  void createComputePipeline()
  {
    const auto computeShader = compileShader(L"../../../Tutorials/T16ComputeShaderPostProcess/Shaders/ComputeShaderPostProcess.hlsl", L"main", L"cs_6_0");
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.pRootSignature = m_computeRootSignature.Get();
    psoDesc.CS             = HLSLCompiler::convert(computeShader);
    psoDesc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;
    throwIfFailed(getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_computePipelineState)));
  }

public:
  ComputeShaderWriteToRenderTarget(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {

    createGraphicsRootSignature();
    createComputeRootSignature();

    createGraphicsPipeline();
    createComputePipeline();

    {
      D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
      srvHeapDesc.NumDescriptors             = 2;
      srvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      srvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      throwIfFailed(getDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_descriptorHeap)));

      const auto descSize = getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
      m_offscreenTarget_CPU_UAV =
          CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, descSize);
      m_offscreenTarget_GPU_UAV =
          CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, descSize);
    }

    {
      D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
      rtvHeapDesc.NumDescriptors             = 1;
      rtvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
      rtvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
      throwIfFailed(getDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));

      const auto descSize = getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
      m_offscreenTarget_CPU_RTV =
          CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, descSize);
    }

    createRenderTargetTexture();
  }

  virtual void onDraw()
  {
    const auto& cmds = getCommandList();   
    const auto dsvHandle = getDSVHandle();
    cmds->OMSetRenderTargets(1, &m_offscreenTarget_CPU_RTV, FALSE, &dsvHandle);
    

    f32v4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    cmds->ClearRenderTargetView(m_offscreenTarget_CPU_RTV, &clearColor.x, 0, nullptr);    
    cmds->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    cmds->RSSetViewports(1, &getViewport());
    cmds->RSSetScissorRects(1, &getRectScissor());

    cmds->SetPipelineState(m_graphicsPipelineSate.Get());
    cmds->SetGraphicsRootSignature(m_graphicsRootSignature.Get());
    cmds->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmds->DrawInstanced(3, 1, 0, 0);

    {
      CD3DX12_RESOURCE_BARRIER toUAV = CD3DX12_RESOURCE_BARRIER::Transition(
          m_offscreenTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      cmds->ResourceBarrier(1, &toUAV);
    }

    cmds->SetPipelineState(m_computePipelineState.Get());
    cmds->SetComputeRootSignature(m_computeRootSignature.Get());
    cmds->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
    cmds->SetComputeRootDescriptorTable(0, m_offscreenTarget_GPU_UAV);

    cmds->SetComputeRoot32BitConstant(1, getWidth(), 0);
    cmds->SetComputeRoot32BitConstant(1, getHeight(), 1);

    const ui32v3 threadGroupSize(16, 16, 1);
    cmds->Dispatch(ui32(ceilf(getWidth() / f32(threadGroupSize.x))), ui32(ceilf(getHeight() / f32(threadGroupSize.y))),
                   1);

    {
      CD3DX12_RESOURCE_BARRIER barriers[] = {
          CD3DX12_RESOURCE_BARRIER::Transition(m_offscreenTarget.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                               D3D12_RESOURCE_STATE_COPY_SOURCE),
          CD3DX12_RESOURCE_BARRIER::Transition(getRenderTarget().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                               D3D12_RESOURCE_STATE_COPY_DEST)};

      cmds->ResourceBarrier(_countof(barriers), barriers);
    }

    cmds->CopyResource(getRenderTarget().Get(), m_offscreenTarget.Get());

    {
      CD3DX12_RESOURCE_BARRIER toRenderTarget[] = {
          CD3DX12_RESOURCE_BARRIER::Transition(m_offscreenTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
                                               D3D12_RESOURCE_STATE_RENDER_TARGET),
          CD3DX12_RESOURCE_BARRIER::Transition(getRenderTarget().Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                               D3D12_RESOURCE_STATE_RENDER_TARGET),
      };

      cmds->ResourceBarrier(_countof(toRenderTarget), toRenderTarget);
    }
  }

  void createRenderTargetTexture()
  {
    D3D12_RESOURCE_DESC tex = {};
    m_offscreenTarget.Reset();
    tex.Dimension             = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    tex.Alignment             = 0;
    tex.Width                 = getWidth();
    tex.Height                = getHeight();
    tex.DepthOrArraySize      = 1;
    tex.MipLevels             = 1;
    tex.Format                = getDX12AppConfig().renderTargetFormat;
    tex.SampleDesc.Count      = 1;
    tex.SampleDesc.Quality    = 0;
    tex.Layout                = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    tex.Flags                 = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_CLEAR_VALUE clearValue = {};
    f32v4             clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValue.Format            = getDX12AppConfig().renderTargetFormat;
    clearValue.Color[0]          = clearColor.x;
    clearValue.Color[1]          = clearColor.y;
    clearValue.Color[2]          = clearColor.z;
    clearValue.Color[3]          = clearColor.w;
    throwIfFailed(getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &tex,
                                                       D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
                                                       IID_PPV_ARGS(&m_offscreenTarget)));

    D3D12_UNORDERED_ACCESS_VIEW_DESC unorderAccessViewDesc = {};
    unorderAccessViewDesc.Format                           = getDX12AppConfig().renderTargetFormat;
    unorderAccessViewDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
    unorderAccessViewDesc.Texture2D.MipSlice               = 0;
    getDevice()->CreateUnorderedAccessView(m_offscreenTarget.Get(), nullptr, &unorderAccessViewDesc,
                                           m_offscreenTarget_CPU_UAV);

    D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};

    renderTargetViewDesc.Format             = getDX12AppConfig().renderTargetFormat;
    renderTargetViewDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    getDevice()->CreateRenderTargetView(m_offscreenTarget.Get(), &renderTargetViewDesc, m_offscreenTarget_CPU_RTV);
  }

  virtual void onResize()
  {
    createRenderTargetTexture();
  }
};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 16 Compute Shader Post-Process";
  config.useVSync = true;
  try
  {
    ComputeShaderWriteToRenderTarget app(config);
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
