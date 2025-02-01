#include "AABB.hpp"
namespace gims
{
AABB::AABB()
    : m_lowerLeftBottom(std::numeric_limits<f32>::max())
    , m_upperRightTop(-std::numeric_limits<f32>::max())
{
}
AABB::AABB(f32v3 const* const positions, ui32 nPositions)
    : m_lowerLeftBottom(std::numeric_limits<f32>::max())
    , m_upperRightTop(-std::numeric_limits<f32>::max())

{
  for (ui32 i = 0; i < nPositions; i++)
  {
    const auto& p     = positions[i];
    m_lowerLeftBottom = glm::min(m_lowerLeftBottom, p);
    m_upperRightTop   = glm::max(m_upperRightTop, p);
  }
}

f32v3 AABB::calculateCentroid() const
{
  return ((m_lowerLeftBottom + m_upperRightTop) / f32(2));
}

f32v3 AABB::calculateAxesLengths() const
{
  return (m_upperRightTop - m_lowerLeftBottom);
}

f32 AABB::findLongestAxisLegth() const
{
  f32v3 axesLengths = calculateAxesLengths();
  return std::max(axesLengths.x, std::max(axesLengths.y, axesLengths.z));
}

f32m4 AABB::getNormalizationTransformation() const
{
  // Assignment 2
  f32v3 centroid = calculateCentroid();
  glm::highp_mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -centroid);

  f32v3             axesLengths        = calculateAxesLengths();
  f32 longestAxisLength = findLongestAxisLegth();
  f32v3             scalingFactors     = (glm::vec3(1.0f) / longestAxisLength);
  glm::highp_mat4 scalingMatrix     = glm::scale(glm::mat4(1.0f), scalingFactors);

  return scalingMatrix * translationMatrix;
}
AABB AABB::getUnion(const AABB& other) const
{
  // Assignment 2
  AABB unifiedAABB;
  unifiedAABB.m_lowerLeftBottom = f32v3(
      std::min(m_lowerLeftBottom.x, other.m_lowerLeftBottom.x),
      std::min(m_lowerLeftBottom.y, other.m_lowerLeftBottom.y),
      std::min(m_lowerLeftBottom.z, other.m_lowerLeftBottom.z)
  );
  unifiedAABB.m_upperRightTop   = f32v3(
      std::max(m_upperRightTop.x, other.m_upperRightTop.x),
      std::max(m_upperRightTop.y, other.m_upperRightTop.y),
      std::max(m_upperRightTop.z, other.m_upperRightTop.z));

  return unifiedAABB;
}
const f32v3& AABB::getLowerLeftBottom()
{
  return m_lowerLeftBottom;
}
const f32v3& AABB::getUpperRightTop() const
{
  return m_upperRightTop;
}
AABB AABB::getTransformed(f32m4& transformation) const
{
  // Assignment 2
  AABB transformedAABB;
  transformedAABB.m_lowerLeftBottom = transformation * f32v4(m_lowerLeftBottom, 1.0f);
  transformedAABB.m_upperRightTop   = transformation * f32v4(m_upperRightTop, 1.0f);

  return transformedAABB;
}
} // namespace gims
