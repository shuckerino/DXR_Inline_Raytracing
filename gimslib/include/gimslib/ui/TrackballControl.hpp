/// Cogra --- Coburg Graphics Framework 2017
/// (C) 2017 - 2023 by Quirin Meyer
/// quirin.meyer@hs-coburg.de
#pragma once
#include <gimslib/types.hpp>

namespace gims

{
//! This class implements a trackball, also called arc-ball, providing intuitive rotation for objects.
//! A trackball is like a virtual sphere. Whenever the user starts dragging the mouse, the screen positions
//! are projected onto a virtual sphere and the corresponding z value is calculated. While dragging the mouse
//! two float3 vectors are retrieved, out of which an axis-angle rotation is derived. We use that to interpolate a
//! quaternion.
//! The quaternion can then be converted into a 4x4 matrix.
//! A possible use case is to call startRotation when starting to drag the mouse. While moving the mouse while holding
//! the mouse button, call updateRotatio.
//!
//! Note, that all variables concerning screen coordinates should be normalized an interval between
//! -1.0 and 1.0.
//!
//!
class TrackballControl
{
public:
  //! \brief Initializes trackball.
  //! \param gazeIntoPositiveZDirection true, if the observer gazes towards the positve z direction (Vuklan).
  //!                                   true, if the observer gazes towards the negative z direction (OpenGL).
  //!
  //! \param radius Radius of the virtual hemisphere sphere.
  explicit TrackballControl(bool gazeIntoPositiveZDirection, f32 radius);

  //! \brief Destroy the trackball
  ~TrackballControl() = default;

  //! \brief Call this, when the rotation starts, e.g., when the mouse button has been pressed.
  //!
  //! \param  pos Position on the screen space. Its components should be normalized to be between -1 and 1.
  void startRotation(const f32v2& normalizedMouseCoordinates);

  //! \brief Call this, when updating the rotation, e.g., while the mouse button has been pressed.
  //!
  //! \param  pos Position on the screen space. Its components should be normalized to be between -1 and 1.
  void updateRotation(const f32v2& normalizedMouseCoordinates);

  //! \brief Resets the trackball.
  void reset();

  //! Returns the current quaternion as a rotation matrix.
  f32m4 getRotationMatrix() const;

  //! \brief Returns the current quaternion.
  const f32q& getRotationQuaterion() const;

  //! \brief Sets the quaternion.
  void setRotationQuaterion(const f32q& q);

private:
  //! \brief Projects x,y into a sphere and returns its the corresponding
  //! z value. If x and y are not on the sphere project them on a hyperbola
  //! sheet.
  //! Note that x, y should be between -1.0, 1.0
  //! \param x x position.
  //! \param y y position.
  //! \return z value
  f32 projectToSphere(const f32v2& normalizedMouseCoordinates);

  //! Radius of the trackball sphere
  f32 m_radius;

  //! Position on the screen space. Its components should be normalized to be between -1 and 1.
  f32v2 m_normalizedMouseCoordinates;

  //! Current rotation expressed as a quaternion
  f32q m_rotation;

  //! -1.0 when the points are projected onto the negative hemisphere.
  //! 1.0 when the points are projected onto the positive hemisphere.
  f32 m_hemisphere;
};
} // namespace gims
