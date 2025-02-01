#pragma once
#include <d3d12.h>
#include <gimslib/types.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <wrl.h>
using namespace gims;
namespace gims
{
namespace DX12Util
{

void reportLiveObjects();
void waitForFence(Microsoft::WRL::ComPtr<ID3D12Fence>& fence, ui64 completionValue);
void waitForFence(Microsoft::WRL::ComPtr<ID3D12Fence>& fence, ui64 completionValue, HANDLE waitEvent);
void throwOnDeviceLost(Microsoft::WRL::ComPtr<ID3D12Device> device, HRESULT hr, const std::string debugString);

template<class T> Microsoft::WRL::ComPtr<ID3D12Device> getDevice(const Microsoft::WRL::ComPtr<T>& id3dInterface)
{
  Microsoft::WRL::ComPtr<ID3D12Device> result;
  throwIfFailed(id3dInterface->GetDevice(IID_PPV_ARGS(&result)));
  return result;
}

} // namespace DX12Util

} // namespace gims