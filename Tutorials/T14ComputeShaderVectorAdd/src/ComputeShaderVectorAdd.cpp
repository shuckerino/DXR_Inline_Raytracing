#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <iostream>
using namespace gims;

class ComputeShaderVectorAdd : public DX12App
{
private:
  ComPtr<ID3D12PipelineState> m_pipelineState;  
  ComPtr<ID3D12RootSignature> m_rootSignature;

  void createRootSignature()
  {
    CD3DX12_ROOT_PARAMETER parameters[3] = {};
    parameters[0].InitAsShaderResourceView(0);
    parameters[1].InitAsShaderResourceView(1);
    parameters[2].InitAsUnorderedAccessView(0);

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init(_countof(parameters), parameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> rootBlob, errorBlob;
    D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_rootSignature));
  }

  void createPipeline()
  {
    const auto computeShader =
        compileShader(L"../../../Tutorials/T14ComputeShaderVectorAdd/Shaders/VectorAdd.hlsl", L"main", L"cs_6_0");
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS             = HLSLCompiler::convert(computeShader);
    psoDesc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;
    throwIfFailed(getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
  }

  static constexpr ui32 nElements = 2048;

  ComPtr<ID3D12Resource> m_inA_GPU;
  ComPtr<ID3D12Resource> m_inB_GPU;
  ComPtr<ID3D12Resource> m_outC_GPU;
  ComPtr<ID3D12Resource> m_outC_readBack;

  struct Data
  {
    f32v3 v0;
    f32v2 v1;
  };

  std::vector<Data> m_referenceResult_CPU;

  void createBuffers()
  {
    std::vector<Data> inA_CPU, inB_CPU;
    inA_CPU.reserve(nElements);
    inB_CPU.reserve(nElements);
    m_referenceResult_CPU.reserve(nElements);
    for (size_t i = 0; i < nElements; i++)
    {
      f32 f = static_cast<f32>(i);
      inA_CPU.emplace_back(f32v3(f, 2.0f * f, 0.5f * f), f32v2(f, f * f));
      inB_CPU.emplace_back(f32v3(0.25f * f, 0.125f * f, 0.0625f * f), f32v2(0.0f, 1.0f));
      m_referenceResult_CPU.emplace_back(inA_CPU.back().v0 + inB_CPU.back().v0, inA_CPU.back().v1 + inB_CPU.back().v1);
    }

    const auto sizeInBytes = nElements * sizeof(Data);

    const auto bufferDesc            = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
    const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_inA_GPU.GetAddressOf()));
    getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_inB_GPU.GetAddressOf()));

    UploadHelper uploader(getDevice(), sizeInBytes);
    uploader.uploadBuffer(inA_CPU.data(), m_inA_GPU, sizeInBytes, getCommandQueue());
    uploader.uploadBuffer(inB_CPU.data(), m_inB_GPU, sizeInBytes, getCommandQueue());

    const auto bufferDescUAV = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, 
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDescUAV,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(m_outC_GPU.GetAddressOf()));

    const auto readBackHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    getDevice()->CreateCommittedResource(&readBackHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                         D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                         IID_PPV_ARGS(m_outC_readBack.GetAddressOf()));
  }

  void compute()
  {
    const auto& cmds             = getCommandList();
    const auto& commandAllocator = getCommandAllocator();

    throwIfFailed(commandAllocator->Reset());
    throwIfFailed(cmds->Reset(commandAllocator.Get(), nullptr));

    cmds->SetPipelineState(m_pipelineState.Get());

    cmds->SetComputeRootSignature(m_rootSignature.Get());
    cmds->SetComputeRootShaderResourceView(0, m_inA_GPU->GetGPUVirtualAddress());
    cmds->SetComputeRootShaderResourceView(1, m_inB_GPU->GetGPUVirtualAddress());
    cmds->SetComputeRootUnorderedAccessView(2, m_outC_GPU->GetGPUVirtualAddress());
    cmds->Dispatch( (nElements + 127) / 128, 1, 1);

    const auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(m_outC_GPU.Get(), 
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    cmds->ResourceBarrier(1, &barrier0);
    cmds->CopyResource(m_outC_readBack.Get(), m_outC_GPU.Get());
    const auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(m_outC_GPU.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmds->ResourceBarrier(1, &barrier1);

    cmds->Close();

    ID3D12CommandList* ppCommandLists[] = {cmds.Get()};
    getCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);    
  }

  void compare()
  {
    waitForGPU();
    Data* mapped;
    bool  resultCorrect = true;
    m_outC_readBack->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
    const auto tollerance = 1.0f / (float)(1 << 14);
    for (size_t i = 0; i < nElements; i++)
    {
      const auto v0Okay = glm::compMax(glm::abs(mapped[i].v0 - m_referenceResult_CPU[i].v0)) <= tollerance;
      const auto v1Okay = glm::compMax(glm::abs(mapped[i].v1 - m_referenceResult_CPU[i].v1)) <= tollerance;

      if (!v0Okay || !v1Okay)
      {
        resultCorrect = false;
        break;
      }
    }
    std::cerr << "Compute Shader Result is " << (resultCorrect ? "correct" : "incorrect") << ".\n";
    m_outC_readBack->Unmap(0, nullptr);
  }

public:
  ComputeShaderVectorAdd(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {
    createRootSignature();
    createPipeline();
    createBuffers();
    compute();
    compare();
  }

  virtual void onDraw()
  {
  }
};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 14 Vector Add";
  config.useVSync = true;
  try
  {
    ComputeShaderVectorAdd app(config);
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
