#pragma once
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


namespace gims
{
class Event
{
public:
  Event();

  ~Event();

  HANDLE getHandle() const;

  void wait();


private:
  HANDLE m_eventHandle;
};
}