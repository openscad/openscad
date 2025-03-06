#pragma once
#include <map>
#include <string>
#include <array>
// FIXME - copyright

namespace MouseConfig {

  enum Preset
  {
    OPEN_SCAD,
    BLENDER,
    CUSTOM,
    NUM_PRESETS
  };

  enum MouseAction
  {
    LEFT_CLICK,
    MIDDLE_CLICK,
    RIGHT_CLICK,
    SHIFT_LEFT_CLICK,
    SHIFT_MIDDLE_CLICK,
    SHIFT_RIGHT_CLICK,
    CTRL_LEFT_CLICK,
    CTRL_MIDDLE_CLICK,
    CTRL_RIGHT_CLICK,
    NUM_MOUSE_ACTIONS
  };

  enum ViewAction
  {
    NONE,
    PAN_LR_UD,
    PAN_FORE_BACK,
    ZOOM,
    ROTATE_ALT_AZ,
    ROTATE_PITCH_ROLL,
    NUM_VIEW_ACTIONS
  };

  inline static std::map<Preset, std::map<MouseAction, ViewAction>> presetSettings = {
    {OPEN_SCAD, {
      {LEFT_CLICK, ROTATE_ALT_AZ},
      {MIDDLE_CLICK, PAN_FORE_BACK},
      {RIGHT_CLICK, PAN_LR_UD},
      // FIXME - need to fill in the rest of the settings here.
    }},
    {BLENDER, {
      // FIXME - this is probably not the behaviour of Blender, I just need something to test
      {LEFT_CLICK, PAN_LR_UD},
      {RIGHT_CLICK, ROTATE_ALT_AZ},
    }},
  };

  static std::map<Preset, std::string> presetNames = {
    {OPEN_SCAD, "OpenSCAD"},
    {BLENDER, "Blender"},
    {CUSTOM, "Custom"},
  };

  static std::map<MouseAction, std::string> mouseActionNames = {
    {LEFT_CLICK, "Left-click"},
    {MIDDLE_CLICK, "Middle-click"},
    {RIGHT_CLICK, "Right-click"},
    {SHIFT_LEFT_CLICK, "Shift + Left-click"},
    {SHIFT_MIDDLE_CLICK, "Shift + Middle-click"},
    {SHIFT_RIGHT_CLICK, "Shift + Right-click"},
    {CTRL_LEFT_CLICK, "Ctrl + Left-click"},
    {CTRL_MIDDLE_CLICK, "Ctrl + Middle-click"},
    {CTRL_RIGHT_CLICK, "Ctrl + Right-click"},
  };

  static std::map<ViewAction, std::string> viewActionNames = {
    {NONE, "None"},
    {PAN_LR_UD, "Pan left/right & up/down"},
    {PAN_FORE_BACK, "Pan forward / backward"},
    {ZOOM, "Zoom"},
    {ROTATE_ALT_AZ, "Rotate in altitude/azimuth"},
    {ROTATE_PITCH_ROLL, "Rotate in pitch/roll"},
  };

  const static int ACTION_DIMENSION = 14;
  static std::map<ViewAction, std::array<float, ACTION_DIMENSION>> viewActionArrays = {
    {NONE, {
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Rotation
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
      0.0f, 0.0f,  // Zoom
    }},
    {PAN_LR_UD, {
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Rotation
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,  // Translation
      0.0f, 0.0f,  // Zoom
    }},
    {PAN_FORE_BACK, {
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Rotation
      0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // Translation
      0.0f, 0.0f,  // Zoom
    }},
    {ZOOM, {
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Rotation
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
      0.0f, -1.0f,  // Zoom
    }},
    {ROTATE_ALT_AZ, {
      0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // Rotation
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
      0.0f, 0.0f,  // Zoom
    }},
    {ROTATE_PITCH_ROLL, {
      0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // Rotation
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
      0.0f, 0.0f,  // Zoom
    }},
  };
};
