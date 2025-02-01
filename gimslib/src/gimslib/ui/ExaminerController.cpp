/// Cogra --- Coburg Graphics Framework 2017 -- 2023
/// (C) by Quirin Meyer
/// quirin.meyer@hs-coburg.de
#include <gimslib/ui/ExaminerController.hpp>

namespace gims
{
ExaminerController::ExaminerController(bool gazeIntoPositiveZDirection)
    : m_bRotate(false)
    , m_trackball(gazeIntoPositiveZDirection, 0.8f)
    , m_bShift(false)
    , m_bPitch(false)

{
  reset();
}

void ExaminerController::reset()
{
  m_bRotate = false;
  m_bShift  = false;
  m_bPitch  = false;
  m_pitchShiftControl.reset();
  m_trackball.reset();
}

f32m4 ExaminerController::getRotationMatrix() const
{
  return m_trackball.getRotationMatrix();
}

f32m4 ExaminerController::getTranslationMatrix() const
{
  return m_pitchShiftControl.getTranslationMatrix();
}

f32m4 ExaminerController::getTransformationMatrix() const
{
  return getTranslationMatrix() * getRotationMatrix();
}

void ExaminerController::click(bool pressed, int button, bool keyboardModifier, const f32v2& normalizedMouseCoordinates)
{
  if (button == 1)
  {
    if (pressed)
    {
      if (keyboardModifier)
      {
        m_bShift  = true;
        m_bRotate = false;
        m_pitchShiftControl.startShift(normalizedMouseCoordinates);
      }
      else
      {
        m_bRotate = true;
        m_bShift  = false;
        m_trackball.startRotation(normalizedMouseCoordinates);
      }
    }
    else
    {
      m_bRotate = false;
      m_bShift  = false;
    }
  }

  if (button == 2)
  {
    if (pressed)
    {
      m_bPitch = true;
      m_pitchShiftControl.startPitch(normalizedMouseCoordinates);
    }
    else
    {
      m_bPitch = false;
    }
  }
}

void ExaminerController::move(const f32v2& normalizedMouseCoordinates)
{
  if (m_bRotate)
  {
    m_trackball.updateRotation(normalizedMouseCoordinates);
  }
  if (m_bPitch)
  {
    m_pitchShiftControl.updatePitch(normalizedMouseCoordinates);
  }
  if (m_bShift)
  {
    m_pitchShiftControl.updateShift(normalizedMouseCoordinates);
  }
}

void ExaminerController::abort()
{
  m_bRotate = false;
  m_bShift  = false;
  m_bPitch  = false;
}

bool ExaminerController::active() const
{
  return m_bRotate || m_bShift || m_bPitch;
}

f32q ExaminerController::getRotationQuaterion() const
{
  return m_trackball.getRotationQuaterion();
}

void ExaminerController::setRotationQuaterion(const f32q& r)
{
  m_trackball.setRotationQuaterion(r);
}

f32v3 ExaminerController::getTranslationVector() const
{
  return m_pitchShiftControl.getTranslation();
}

void ExaminerController::setTranslationVector(const f32v3& t)
{
  m_pitchShiftControl.setTranslation(t);
}
} // namespace gims
