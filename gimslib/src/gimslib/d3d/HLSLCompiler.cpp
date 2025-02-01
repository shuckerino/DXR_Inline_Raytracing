#include <d3dx12/d3dx12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <gimslib/d3d/HLSLCompiler.hpp>
#include <gimslib/dbg/HrException.hpp>

namespace
{
std::string wstring_to_string(const std::wstring& wstr)
{
  std::string str(wstr.size(), '\0');
  std::transform(wstr.begin(), wstr.end(), str.begin(),
                 [](wchar_t wc)
                 {
                   return static_cast<char>(wc); // This assumes ASCII-compatible data.
                 });
  return str;
}
} // namespace

namespace gims
{
HLSLCompiler::HLSLCompiler()
{
  HMODULE dxcompilerDLL = LoadLibraryW(L"dxcompiler.dll");

  if (!dxcompilerDLL)
  {
    throw std::runtime_error("Error while loading dxcompiler.dll.");
  }

  DxcCreateInstanceProc pfxDxcCreateInstance =
      DxcCreateInstanceProc(GetProcAddress(dxcompilerDLL, "DxcCreateInstance"));

  if (pfxDxcCreateInstance == nullptr)
  {
    throw std::runtime_error("Error while finding DxcCreateInstance from dxcompiler.dll.");
  }

  throwIfFailed(pfxDxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)));
  throwIfFailed(pfxDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
  throwIfFailed(m_utils->CreateDefaultIncludeHandler(&m_includeHandler));
}

ComPtr<IDxcBlob> HLSLCompiler::compileShader(const std::filesystem::path& shaderFile, const wchar_t* targetProfile,
                                             const wchar_t* entryPoint)
{
  ComPtr<IDxcBlobEncoding> sourceCode;
  if (FAILED(m_utils->LoadFile(shaderFile.wstring().c_str(), nullptr, &sourceCode)))
  {
    throw std::runtime_error("Unable to load " + shaderFile.string());
  }

  std::vector<const wchar_t*> userArguments = {};
  std::vector<DxcDefine>      userDefines   = {};

  ComPtr<IDxcCompilerArgs> arguments;
  m_utils->BuildArguments(shaderFile.wstring().c_str(), entryPoint, targetProfile, userArguments.data(),
                          (UINT32)userArguments.size(), userDefines.data(), (UINT32)userDefines.size(), &arguments);

  DxcBuffer sourceCodeAsBuffer;
  sourceCodeAsBuffer.Ptr      = sourceCode->GetBufferPointer();
  sourceCodeAsBuffer.Size     = sourceCode->GetBufferSize();
  sourceCodeAsBuffer.Encoding = DXC_CP_UTF8;

  ComPtr<IDxcResult> result = nullptr;
  throwIfFailed(m_compiler->Compile(&sourceCodeAsBuffer, arguments->GetArguments(), arguments->GetCount(), nullptr,
                                    IID_PPV_ARGS(&result)));

  HRESULT compileStatus;
  result->GetStatus(&compileStatus);

  if (FAILED(compileStatus))
  {
    ComPtr<IDxcBlobUtf8> errors = nullptr;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

    if (errors != nullptr && errors->GetStringPointer() != nullptr && errors->GetStringLength() != 0)
    {
      const auto errorString =
          std::format("Failed to compile {} (Profile: {}, Entroy: {}):\n{}", shaderFile.string(),
                      wstring_to_string(targetProfile), wstring_to_string(entryPoint), errors->GetStringPointer());
      OutputDebugStringA(errorString.c_str());
      throw std::exception((char*)errorString.c_str());
    }
  }

  ComPtr<IDxcBlob> shaderBlob;
  result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
  return shaderBlob;
}

D3D12_SHADER_BYTECODE HLSLCompiler::convert(ComPtr<IDxcBlob> in)
{
  D3D12_SHADER_BYTECODE result = {};
  if (in)
  {
    result.BytecodeLength  = in->GetBufferSize();
    result.pShaderBytecode = in->GetBufferPointer();
  }
  return result;
}
} // namespace gims