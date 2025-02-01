/// Cogra --- Coburg Graphics Framework
/// (C) 2017 -- 2023 by Quirin Meyer
/// quirin.meyer@hs-coburg.de
#include <gimslib/ui/PitchShiftControl.hpp>
namespace gims
{
PitchShiftControl::PitchShiftControl(f32v3 defaultTranslation)
    : m_shift(0.0f, 0.0f)
    , m_pitch(0.0f, 0.0f)
    , m_translation(defaultTranslation)
    , m_defaultTranslation(defaultTranslation)
{
}

PitchShiftControl::~PitchShiftControl() = default;

void PitchShiftControl::startPitch(const f32v2& normalizedMousePosition)
{
  m_pitch = normalizedMousePosition;
}

void PitchShiftControl::updatePitch(const f32v2& normalizedMousePosition)
{
  f32v2 delta = normalizedMousePosition - m_pitch;
  m_translation.x += delta.x;
  m_translation.y += delta.y;
  m_pitch = normalizedMousePosition;
}

void PitchShiftControl::startShift(const f32v2& normalizedMousePosition)
{
  m_shift = normalizedMousePosition;
}

void PitchShiftControl::updateShift(const f32v2& normalizedMousePosition)
{
  f32v2 delta = normalizedMousePosition - m_shift;
  m_translation.z -= delta.y;
  m_shift = normalizedMousePosition;
}

void PitchShiftControl::reset()
{
  m_translation = m_defaultTranslation;
  m_pitch       = f32v2(0, 0);
  m_shift       = f32v2(0, 0);
}

f32v3 PitchShiftControl::getTranslation() const
{
  return m_translation;
}

void PitchShiftControl::setTranslation(const f32v3 val)
{
  m_translation = val;
}

f32m4 PitchShiftControl::getTranslationMatrix() const
{
  return glm::translate(m_translation);
}
} // namespace gims
