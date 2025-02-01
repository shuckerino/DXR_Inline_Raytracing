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
  ComPtr<ID3D12PipelineState>   m_pipelineState;  
  ComPtr<ID3D12RootSignature>   m_rootSignature;
  
  ComPtr<ID3D12Resource>        m_offscreenTarget;
  
  CD3DX12_CPU_DESCRIPTOR_HANDLE m_offscreenTarget_CPU_UAV;
  CD3DX12_GPU_DESCRIPTOR_HANDLE m_offscreenTarget_GPU_UAV;

  ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;

  void createRootSignature()
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
                                     IID_PPV_ARGS(&m_rootSignature));
  }

  void createPipeline()
  {
    const auto computeShader = compileShader(L"../../../Tutorials/T15ComputeShaderWriteToRenderTarget/Shaders/WriteToRenderTarget.hlsl", L"main", L"cs_6_0");
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS             = HLSLCompiler::convert(computeShader);
    psoDesc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;
    throwIfFailed(getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
  }

public:
  ComputeShaderWriteToRenderTarget(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {
    createRootSignature();
    createPipeline();

    D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
    uavHeapDesc.NumDescriptors             = 2;
    uavHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uavHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    throwIfFailed(getDevice()->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&m_descriptorHeap)));

    const auto descSize = getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_offscreenTarget_CPU_UAV =
        CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, descSize);
    m_offscreenTarget_GPU_UAV =
        CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, descSize);
    
    createRenderTargetTexture();
  }

  virtual void onDraw()
  {
    const auto& cmds = getCommandList();

    {
      const auto toUAV = CD3DX12_RESOURCE_BARRIER::Transition(m_offscreenTarget.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                              D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      cmds->ResourceBarrier(1, &toUAV);
    }

    cmds->SetPipelineState(m_pipelineState.Get());
    cmds->SetComputeRootSignature(m_rootSignature.Get());
    cmds->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
    cmds->SetComputeRootDescriptorTable(0, m_offscreenTarget_GPU_UAV);
    cmds->SetComputeRoot32BitConstant(1, getWidth(), 0);
    cmds->SetComputeRoot32BitConstant(1, getHeight(), 1);

    const ui32v3 threadGroupSize(16, 16, 1);
    cmds->Dispatch(ui32(ceilf(getWidth() / f32(threadGroupSize.x))), 
                   ui32(ceilf(getHeight() / f32(threadGroupSize.y))), 1);

    {
      CD3DX12_RESOURCE_BARRIER barriers[] = {
          CD3DX12_RESOURCE_BARRIER::Transition(m_offscreenTarget.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                               D3D12_RESOURCE_STATE_COMMON),
          CD3DX12_RESOURCE_BARRIER::Transition(getRenderTarget().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                               D3D12_RESOURCE_STATE_COPY_DEST)};

      cmds->ResourceBarrier(_countof(barriers), barriers);
    }

    cmds->CopyResource(getRenderTarget().Get(), m_offscreenTarget.Get());

    {
      CD3DX12_RESOURCE_BARRIER toRenderTarget[] = {CD3DX12_RESOURCE_BARRIER::Transition(
          getRenderTarget().Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET)};
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
    tex.Flags                 = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    throwIfFailed(getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &tex,
                                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                       IID_PPV_ARGS(&m_offscreenTarget)));

    D3D12_UNORDERED_ACCESS_VIEW_DESC unorderAccessViewDesc = {};
    unorderAccessViewDesc.Format                           = getDX12AppConfig().renderTargetFormat;
    unorderAccessViewDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
    unorderAccessViewDesc.Texture2D.MipSlice               = 0;
    getDevice()->CreateUnorderedAccessView(m_offscreenTarget.Get(), nullptr, &unorderAccessViewDesc, 
        m_offscreenTarget_CPU_UAV);
  }

  virtual void onResize()
  {
    createRenderTargetTexture();
  }
};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 15 Compute Shader Write To Render Target";
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
