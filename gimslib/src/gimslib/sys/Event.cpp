#include "gimslib/sys/Event.hpp"
#include <gimslib/dbg/HrException.hpp>
namespace
{
HANDLE createEvent()
{
  auto result = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (result == 0)
  {
    gims::throwIfFailed(HRESULT_FROM_WIN32(GetLastError()));
  }
  return result;
}
}

namespace gims
{
Event::Event()
    : m_eventHandle(createEvent())
{

}

Event::~Event()
{
  if (m_eventHandle != 0)
  {
    CloseHandle(m_eventHandle);
  }
}

HANDLE Event::getHandle() const
{
  return m_eventHandle;
}

void Event::wait()
{
  WaitForSingleObjectEx(m_eventHandle, INFINITE, FALSE);
}



} // namespace gims