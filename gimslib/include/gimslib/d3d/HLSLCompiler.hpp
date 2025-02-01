#pragma once
#include <d3dx12/d3dx12.h>
#include <dxcapi.h>
#include <filesystem>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

namespace gims
{
class HLSLCompiler
{
public:
  HLSLCompiler();

  ComPtr<IDxcBlob> compileShader(const std::filesystem::path& shaderFile, const wchar_t* targetProfile,
                                 const wchar_t* entryPoint);

  static D3D12_SHADER_BYTECODE convert(ComPtr<IDxcBlob> in);


private:
  ComPtr<IDxcUtils>          m_utils;
  ComPtr<IDxcCompiler3>      m_compiler;  
  ComPtr<IDxcIncludeHandler> m_includeHandler;
};
} // namespace gims