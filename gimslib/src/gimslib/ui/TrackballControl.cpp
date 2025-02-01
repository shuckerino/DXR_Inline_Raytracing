/// Cogra --- Coburg Graphics Framework 2017
/// (C) 2017 -- 2023 by Quirin Meyer
/// quirin.meyer@hs-coburg.de
#include <gimslib/ui/TrackballControl.hpp>
namespace gims
{
TrackballControl::TrackballControl(bool gazeIntoPositiveZDirection, f32 radius)
    : m_radius(radius)
    , m_normalizedMouseCoordinates(0, 0)
    , m_rotation(glm::angleAxis(0.0f, f32v3(0, 0, -1)))
    , m_hemisphere(gazeIntoPositiveZDirection == true ? -1.0f : 1.0f)
{
}

void TrackballControl::startRotation(const f32v2& normalizedMouseCoordinates)
{
  m_normalizedMouseCoordinates = normalizedMouseCoordinates;
}

void TrackballControl::updateRotation(const f32v2& normalizedMouseCoordinates)
{
  if (normalizedMouseCoordinates.x == m_normalizedMouseCoordinates.x &&
      normalizedMouseCoordinates.y == m_normalizedMouseCoordinates.y)
  {
    return;
  }
  f32v3 p1 = f32v3(m_normalizedMouseCoordinates.x, m_normalizedMouseCoordinates.y,
                   projectToSphere(m_normalizedMouseCoordinates));
  f32v3 p2 =
      f32v3(normalizedMouseCoordinates.x, normalizedMouseCoordinates.y, projectToSphere(normalizedMouseCoordinates));
  f32v3 a = glm::normalize(glm::cross(p1, p2));

  f32 phi                      = 2.0f * glm::asin(glm::clamp(glm::length(p1 - p2) / (2.0f * m_radius), -1.0f, 1.0f));
  m_rotation                   = glm::angleAxis(phi, a) * m_rotation;
  m_rotation                   = glm::normalize(m_rotation);
  m_normalizedMouseCoordinates = normalizedMouseCoordinates;
}

f32 TrackballControl::projectToSphere(const f32v2& normalizedMouseCoordinates)
{
  const auto d = normalizedMouseCoordinates.x * normalizedMouseCoordinates.x +
                 normalizedMouseCoordinates.y * normalizedMouseCoordinates.y;
  const auto t = m_radius * m_radius * 0.5f;
  if (d < t)
  {
    return m_hemisphere * sqrtf(m_radius * m_radius - d);
  }
  else
  {

    return m_hemisphere * t / sqrtf(d);
  }
}

void TrackballControl::reset()
{
  m_rotation = glm::angleAxis(0.0f, f32v3(0, 0, -1));
}

f32m4 TrackballControl::getRotationMatrix() const
{
  return glm::toMat4(m_rotation);
}

const f32q& TrackballControl::getRotationQuaterion() const
{
  return m_rotation;
}

void TrackballControl::setRotationQuaterion(const f32q& q)
{
  m_rotation = q;
}
} // namespace gims
