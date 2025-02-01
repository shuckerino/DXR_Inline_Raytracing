#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <intsafe.h>
#include <sstream>
#include <wrl.h>
#include <gimslib/sys/Event.hpp>
namespace gims
{
namespace DX12Util
{

void reportLiveObjects()
{
  Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
  {
    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_FLAGS(
                                                     DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL)));
  }
}

void waitForFence(Microsoft::WRL::ComPtr<ID3D12Fence>& fence, ui64 completionValue, HANDLE waitEvent)
{
  if (fence->GetCompletedValue() < completionValue)
  {
    fence->SetEventOnCompletion(completionValue, waitEvent);
    WaitForSingleObjectEx(waitEvent, INFINITE, FALSE);
  }
}

void waitForFence(Microsoft::WRL::ComPtr<ID3D12Fence>& fence, ui64 completionValue)
{
  if (fence->GetCompletedValue() < completionValue)
  {
    gims::Event waitEvent;
    fence->SetEventOnCompletion(completionValue, waitEvent.getHandle());
    WaitForSingleObjectEx(waitEvent.getHandle(), INFINITE, FALSE);
  }
}


bool isDeviceLost(HRESULT hr)
{
  return hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET;
}

void throwOnDeviceLost(Microsoft::WRL::ComPtr<ID3D12Device> device, HRESULT hr, const std::string debugString)
{
  if (isDeviceLost(hr))
  {
    std::ostringstream buff;
    buff << "Lost device during " << debugString << ". Reason : ";
    if (hr == DXGI_ERROR_DEVICE_REMOVED)
    {
      buff << "DXGI_ERROR_DEVICE_REMOVED" << std::hex << device->GetDeviceRemovedReason();
    }
    else
    {
      buff << "DXGI_ERROR_DEVICE_RESET";
    }
    buff << ".";
    OutputDebugStringA(buff.str().c_str());
    throw HrException(hr);
  }
  else
  {
    throwIfFailed(hr);
  }
}

} // namespace DX12Util
} // namespace gims