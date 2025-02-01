#include <format>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <stacktrace>
using namespace gims;
namespace
{
std::string HrToString(HRESULT hr)
{
  return std::format("HR-Error: {:#010x}\n", (ui32)hr);
}
} // namespace

namespace gims
{
HrException::HrException(HRESULT hr)
    : std::runtime_error(HrToString(hr) + "\n" + std::to_string(std::stacktrace::current()))
    , m_hr(hr)
{
}
void throwIfFailed(HRESULT hr)
{
  if (FAILED(hr))
  {
    throw HrException(hr);
  }
}

} // namespace gims