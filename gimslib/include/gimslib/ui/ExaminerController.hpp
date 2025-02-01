/// Cogra --- Coburg Graphics Framework 2017 -- 2023
/// (C) by Quirin Meyer
/// quirin.meyer@hs-coburg.de
#pragma once
#include <gimslib/types.hpp>
#include <gimslib/ui/PitchShiftControl.hpp>
#include <gimslib/ui/TrackballControl.hpp>
namespace gims
{
class ExaminerController
{
public:
  //! \brief Constructor.
  //! \param gazeIntoPositiveZDirection true, if the observer gazes towards the positve z direction (Vuklan, D3D12).
  //!                                   false, if the observer gazes towards the negative z direction (OpenGL).
  //!
  ExaminerController(bool gazeIntoPositiveZDirection);

  //! \brief Resets the controller to its default parameters.
  void reset();

  //! \brief Returns the rotation matrix.
  f32m4 getRotationMatrix() const;

  //! \brief Returns the rotation quaternion.
  f32q getRotationQuaterion() const;

  //! \brief Sets the rotation quaternion.
  void setRotationQuaterion(const f32q& r);

  //! \brief Gets the translation vector.
  f32v3 getTranslationVector() const;

  //! \brief Sets the translation vector.
  void setTranslationVector(const f32v3& t);

  //! \brief Returns the translation matrix,
  f32m4 getTranslationMatrix() const;

  //! \brief Returns the product of translation and rotation matrix.
  f32m4 getTransformationMatrix() const;

  //! \brief Controller Processing when the mouse button is pressed or released.
  //! \param  pressed	True, if a mouse button has just been clicked, false if it has just been released.
  //! \param  button	Which button is affected by the event. 1 for left, 2 for middle, 4 for right.
  //! \param  keyboardModifier True of a keyoard modifier key was pressed.
  //! \param  normalizedMouseCoordinates Mouse coordinates in the range [-1;1]^2
  void click(bool pressed, int button, bool keyboardModifier, const f32v2& normalizedMouseCoordinates);

  //! \brief Controller processing when the mouse button is moved.
  //!
  //! \param  normalizedMouseCoordinates Mouse coordinates in the range [-1;1]^2
  void move(const f32v2& normalizedMouseCoordinates);

  //! Aborts the mouse movement.
  void abort();

  //! True, if the examiner controller is currently in action.
  bool active() const;

private:
  //! True of the controller is currently in rotation mode, e.g. when the corresponding mouse button has been pressed.
  bool m_bRotate;

  //! Trackball controller. Takes care of rotations.
  TrackballControl m_trackball;

  //! True of the controller is currently in shift mode, e.g. when the corresponding mouse button has been pressed.
  bool m_bShift;

  //! True of the controller is currently in pitch mode, e.g. when the corresponding mouse button has been pressed.
  bool m_bPitch;

  //! Shift pitch controller. Takes care of translations.
  PitchShiftControl m_pitchShiftControl;
};
} // namespace gims
