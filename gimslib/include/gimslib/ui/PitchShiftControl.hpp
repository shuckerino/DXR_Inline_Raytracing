/// Cogra --- Coburg Graphics Framework
/// (C) 2017 -- 2023 by Quirin Meyer
/// quirin.meyer@hs-coburg.de
#pragma once

//! \brief Provides a Pitch and Shift Control using a mouse interface.
//!
//! This class provides a translation vector that is computed typically from mouse motion.
//! Shifting alters the translation vector in x and y direction.
//! Pitching alters the translation vector in z direction.
//! Whenever the users starts dragging the mouse call startPitch or startShift. During the mouse motion call updateShift
//! and updatePitch.
//!
//! For example, startShift is called when the middle mouse button is clicked and startPitch is called when the right
//! mouse button is clicked.
//! During the mouse motion function, call the respective update functions.
//!
//! \date June 2010
#include <gimslib/types.hpp>
namespace gims
{
class PitchShiftControl
{
public:
  //! \brief Initializes the PitchShiftControl.
  //!
  //! \param  defaultTranslation Optionally provide a defaultTranslation, to which the translation is set when reset is
  //! called.
  explicit PitchShiftControl(f32v3 defaultTranslation = f32v3(0, 0, 0));

  //! \brief Destructor.
  virtual ~PitchShiftControl();

  //! \brief Tell the control to start pitching.  This happens for example, when the user started clicking the mouse and
  //! starts dragging it.
  //!
  //! Pitching alters the translation vector in z direction.
  //!
  //! \param  pos Position on the screen space. Its components should be normalized to be between -1 and 1.
  void startPitch(const f32v2& normalizedMousePosition);

  //! \brief Tell the control to update the Pitching.  This happens for example, when the user is holding the a mouse
  //! button down.
  //!
  //! Pitching alters the translation vector in z direction.
  //!
  //! \param  normalizedMousePosition Position on the screen space. Its components should be normalized to be between -1
  //! and 1.
  void updatePitch(const f32v2& normalizedMousePosition);

  //! \brief Tell the control to start shifting.  This happens for example, when the user started clicking the mouse and
  //! starts dragging it.
  //!
  //! Shifting alters the translation vector in x and y direction.
  //!
  //! \param  pos Position on the screen space. Its components should be normalized to be between -1 and 1.
  void startShift(const f32v2& normalizedMousePosition);

  //! \brief Tell the control to update the shifting.  This happens for example, when the user is holding the a mouse
  //! button down.
  //!
  //! Shifting alters the translation vector in x and y direction.
  //!
  //! \param  pos normalizedMousePosition on the screen space. Its components should be normalized to be between -1
  //! and 1.
  void updateShift(const f32v2& normalizedMousePosition);

  //! \brief Resets the control to its default parameters
  void reset();

  //! Gets the current translation.
  f32v3 getTranslation() const;

  //! Gets the current translation matrix.
  f32m4 getTranslationMatrix() const;

  //! \brief Sets the current translation
  //!
  //! \param  val The new translation.
  void setTranslation(f32v3 val);

private:
  //! Current position of the mouse, while being shifted.
  f32v2 m_shift;

  //! Current position of the mouse, while being pitching.
  f32v2 m_pitch;

  //! Shift and pitch are converted into a translation vector.
  f32v3 m_translation;

  //! When reseting the control, set translation to the default translation.
  f32v3 m_defaultTranslation;
};
} // namespace gims
