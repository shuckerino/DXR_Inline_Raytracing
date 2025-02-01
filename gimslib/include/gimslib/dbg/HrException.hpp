#pragma once
#include <stdexcept>
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
namespace gims
{

class HrException : public std::runtime_error
{
public:
  HrException(HRESULT hr);
  HRESULT error() const
  {
    return m_hr;
  }

private:
  const HRESULT m_hr;
};

void throwIfFailed(HRESULT hr);

template<class T> inline void throwIfNullptr(T p)
{
  if (p == nullptr)
  {
    throw std::runtime_error("Nullptr");
  }
}

template<class T> inline void throwIfZero(T p)
{
  if (p == 0)
  {
    throw std::runtime_error("Null");
  }
}

} // namespace gims