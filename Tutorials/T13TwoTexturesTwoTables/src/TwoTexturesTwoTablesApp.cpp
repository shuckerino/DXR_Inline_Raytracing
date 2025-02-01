#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <iostream>
using namespace gims;

class TwoTexturesAppTwoTables : public DX12App
{
private:
  ComPtr<ID3D12PipelineState> m_pipelineState;
  ComPtr<ID3D12RootSignature> m_rootSignature;

  void createRootSignature()
  {
    CD3DX12_ROOT_PARAMETER   parameters[2] = {};
    CD3DX12_DESCRIPTOR_RANGE range[2]      = {
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2},
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5},
    };
    parameters[0].InitAsDescriptorTable(1, &range[0]);
    parameters[1].InitAsDescriptorTable(1, &range[1]);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter                    = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias                = 0;
    sampler.MaxAnisotropy             = 0;
    sampler.ComparisonFunc            = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor               = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD                    = 0.0f;
    sampler.MaxLOD                    = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister            = 0;
    sampler.RegisterSpace             = 0;
    sampler.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init(2, parameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> rootBlob, errorBlob;
    D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_rootSignature));
  }

  void createPipeline()
  {
    const auto vertexShader =
        compileShader(L"../../../Tutorials/T13TwoTexturesTwoTables/Shaders/TwoTextures.hlsl", L"VS_main", L"vs_6_0");

    const auto pixelShader =
        compileShader(L"../../../Tutorials/T13TwoTexturesTwoTables/Shaders/TwoTextures.hlsl", L"PS_main", L"ps_6_0");

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

  ComPtr<ID3D12Resource>       m_texture0;
  ComPtr<ID3D12Resource>       m_texture1;
  ComPtr<ID3D12DescriptorHeap> m_srv;

  ComPtr<ID3D12Resource> createTexture(char const* const pathToFile)

  {
    ComPtr<ID3D12Resource> texture;
    i32                    textureWidth, textureHeight, textureComp;

    stbi_set_flip_vertically_on_load(1);
    std::unique_ptr<ui8, void (*)(ui8*)> image(stbi_load(pathToFile, &textureWidth, &textureHeight, &textureComp, 4),
                                               [](ui8* p) { stbi_image_free(p); });

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels           = 1;
    textureDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width               = textureWidth;
    textureDesc.Height              = textureHeight;
    textureDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize    = 1;
    textureDesc.SampleDesc.Count    = 1;
    textureDesc.SampleDesc.Quality  = 0;
    textureDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    throwIfFailed(getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc,
                                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&texture)));

    UploadHelper uploadHelper(getDevice(), GetRequiredIntermediateSize(texture.Get(), 0, 1));
    uploadHelper.uploadTexture(image.get(), texture, textureWidth, textureHeight, getCommandQueue());
    return texture;
  }

  ComPtr<ID3D12DescriptorHeap> createTextureSRV()
  {
    ComPtr<ID3D12DescriptorHeap> srv;
    D3D12_DESCRIPTOR_HEAP_DESC   desc = {};
    desc.Type                         = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors               = 2;
    desc.NodeMask                     = 0;
    desc.Flags                        = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    throwIfFailed(getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srv)));
    return srv;
  }

  void addTextureToSRV(ComPtr<ID3D12Resource>& texture, i32 offsetInDescriptors)
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    shaderResourceViewDesc.Texture2D.MipLevels             = 1;
    shaderResourceViewDesc.Texture2D.MostDetailedMip       = 0;
    shaderResourceViewDesc.Texture2D.ResourceMinLODClamp   = 0.0f;
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_srv->GetCPUDescriptorHandleForHeapStart());
    hDescriptor = hDescriptor.Offset(
        offsetInDescriptors, getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    getDevice()->CreateShaderResourceView(texture.Get(), &shaderResourceViewDesc, hDescriptor);
  }

public:
  TwoTexturesAppTwoTables(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {
    createRootSignature();
    createPipeline();
    m_srv      = createTextureSRV();
    m_texture0 = createTexture("../../../data/bunny.png");
    m_texture1 = createTexture("../../../data/checker-map_tho.png");
    addTextureToSRV(m_texture0, 0);
    addTextureToSRV(m_texture1, 1);
    // see https://stackoverflow.com/questions/55628161/how-to-bind-textures-to-different-register-in-dx12
  }

  virtual void onDraw()
  {
    const auto commandList = getCommandList();
    const auto rtvHandle   = getRTVHandle();
    const auto dsvHandle   = getDSVHandle();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->SetDescriptorHeaps(1, m_srv.GetAddressOf());

    CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_srv->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootDescriptorTable(0, tex);
    tex.Offset(1, getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    commandList->SetGraphicsRootDescriptorTable(1, tex);

    f32v4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    commandList->ClearRenderTargetView(rtvHandle, &clearColor.x, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    commandList->RSSetViewports(1, &getViewport());
    commandList->RSSetScissorRects(1, &getRectScissor());

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->DrawInstanced(6, 1, 0, 0);
  }
};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 13 Two Textures Two Tables";
  config.useVSync = true;
  try
  {
    TwoTexturesAppTwoTables app(config);
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
