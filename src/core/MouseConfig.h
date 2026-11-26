#pragma once
#include <array>
#include <map>
#include <string>

namespace MouseConfig {

enum Preset { OPEN_SCAD, OPEN_SCAD_MACOS, BLENDER, FUSION, CUSTOM, NUM_PRESETS };

const static size_t NUM_BUTTONS = 3;
const static size_t NUM_MODIFIER_STATES = 4;

enum MouseAction {
  LEFT_CLICK,
  SHIFT_LEFT_CLICK,
  CTRL_LEFT_CLICK,
  CTRL_SHIFT_LEFT_CLICK,
  MIDDLE_CLICK,
  SHIFT_MIDDLE_CLICK,
  CTRL_MIDDLE_CLICK,
  CTRL_SHIFT_MIDDLE_CLICK,
  RIGHT_CLICK,
  SHIFT_RIGHT_CLICK,
  CTRL_RIGHT_CLICK,
  CTRL_SHIFT_RIGHT_CLICK,
  SCROLL,
  SHIFT_SCROLL,
  CTRL_SCROLL,
  CTRL_SHIFT_SCROLL,
  NUM_MOUSE_ACTIONS
};

enum ViewAction {
  NONE,
  PAN_LR_UD,
  PAN_FORE_BACK,
  ZOOM,
  ROTATE_ALT_AZ,
  ROTATE_PITCH_ROLL,
  ZOOM_FOV,
  NUM_VIEW_ACTIONS
};

inline static std::map<Preset, std::map<MouseAction, ViewAction>> presetSettings = {
  {OPEN_SCAD,
   {
     {LEFT_CLICK, ROTATE_ALT_AZ},
     {MIDDLE_CLICK, PAN_FORE_BACK},
     {RIGHT_CLICK, PAN_LR_UD},
     {SHIFT_LEFT_CLICK, ROTATE_PITCH_ROLL},
     {SHIFT_MIDDLE_CLICK, ZOOM},
     {SHIFT_RIGHT_CLICK, ZOOM},
     {CTRL_LEFT_CLICK, ROTATE_ALT_AZ},
     {CTRL_MIDDLE_CLICK, PAN_FORE_BACK},
     {CTRL_RIGHT_CLICK, PAN_LR_UD},
     {CTRL_SHIFT_LEFT_CLICK, ROTATE_PITCH_ROLL},
     {CTRL_SHIFT_MIDDLE_CLICK, ZOOM},
     {CTRL_SHIFT_RIGHT_CLICK, ZOOM},
     {SCROLL, ZOOM},
     {SHIFT_SCROLL, ZOOM_FOV},
     {CTRL_SCROLL, ZOOM},
     {CTRL_SHIFT_SCROLL, ZOOM_FOV},
   }},
  {OPEN_SCAD_MACOS,
   {
     {LEFT_CLICK, ROTATE_ALT_AZ},
     {MIDDLE_CLICK, PAN_FORE_BACK},
     {RIGHT_CLICK, PAN_LR_UD},
     {SHIFT_LEFT_CLICK, ROTATE_PITCH_ROLL},
     {SHIFT_MIDDLE_CLICK, ZOOM},
     {SHIFT_RIGHT_CLICK, ZOOM},
     {CTRL_LEFT_CLICK, PAN_LR_UD},
     {CTRL_MIDDLE_CLICK, PAN_FORE_BACK},
     {CTRL_RIGHT_CLICK, PAN_LR_UD},
     {CTRL_SHIFT_LEFT_CLICK, ZOOM},
     {CTRL_SHIFT_MIDDLE_CLICK, ZOOM},
     {CTRL_SHIFT_RIGHT_CLICK, ZOOM},
     {SCROLL, ZOOM},
     {SHIFT_SCROLL, ZOOM_FOV},
     {CTRL_SCROLL, ZOOM},
     {CTRL_SHIFT_SCROLL, ZOOM_FOV},
   }},
  {BLENDER,
   {
     {MIDDLE_CLICK, ROTATE_ALT_AZ},  // Technically slightly different - Blender always
                                     // seems to rotate around the z-axis. But very close.
     {SHIFT_MIDDLE_CLICK, PAN_LR_UD},
     {CTRL_MIDDLE_CLICK, ZOOM},
     {CTRL_SHIFT_MIDDLE_CLICK, ZOOM},
     {SCROLL, ZOOM},
     {SHIFT_SCROLL, ZOOM_FOV},
     {CTRL_SCROLL, ZOOM},
     {CTRL_SHIFT_SCROLL, ZOOM_FOV},
   }},
  {FUSION,
   {
     {MIDDLE_CLICK, PAN_LR_UD},
     {SHIFT_MIDDLE_CLICK, ROTATE_ALT_AZ},
     {CTRL_MIDDLE_CLICK, PAN_LR_UD},
     {CTRL_SHIFT_MIDDLE_CLICK, ZOOM},
     {SCROLL, ZOOM},
     {SHIFT_SCROLL, ZOOM_FOV},
     {CTRL_SCROLL, ZOOM},
     {CTRL_SHIFT_SCROLL, ZOOM_FOV},
   }},
};

static std::map<Preset, std::string> presetNames = {
  {OPEN_SCAD, "OpenSCAD"}, {OPEN_SCAD_MACOS, "OpenSCAD (Mac OS)"},
  {BLENDER, "Blender"},    {FUSION, "Fusion"},
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
  {CTRL_SHIFT_LEFT_CLICK, "Ctrl + Shift + Left-click"},
  {CTRL_SHIFT_MIDDLE_CLICK, "Ctrl + Shift + Middle-click"},
  {CTRL_SHIFT_RIGHT_CLICK, "Ctrl + Shift + Right-click"},
  {SCROLL, "Scroll"},
  {SHIFT_SCROLL, "Shift + Scroll"},
  {CTRL_SCROLL, "Ctrl + Scroll"},
  {CTRL_SHIFT_SCROLL, "Ctrl + Shift + Scroll"},
};

static std::map<ViewAction, std::string> viewActionNames = {
  {NONE, "None"},
  {PAN_LR_UD, "Pan left/right & up/down"},
  {PAN_FORE_BACK, "Pan forward / backward"},
  {ZOOM, "Zoom"},
  {ROTATE_ALT_AZ, "Rotate in altitude/azimuth"},
  {ROTATE_PITCH_ROLL, "Rotate in pitch/roll"},
  {ZOOM_FOV, "FOV Zoom"},
};

const static int ACTION_DIMENSION = 6 + 6 + 2 + 2;
static std::map<ViewAction, std::array<float, ACTION_DIMENSION>> viewActionArrays = {
  // clang-format off
  {NONE,
   {
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Rotation
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
     0.0f, 0.0f,                          // Zoom
     0.0f, 0.0f,                          // FOV Zoom
   }},
  {PAN_LR_UD,
   {
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // Rotation
     1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,  // Translation
     0.0f, 0.0f,                           // Zoom
     0.0f, 0.0f,                           // FOV Zoom
   }},
  {PAN_FORE_BACK,
   {
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // Rotation
     0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // Translation
     0.0f, 0.0f,                           // Zoom
     0.0f, 0.0f,                           // FOV Zoom
   }},
  {ZOOM,
   {
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Rotation
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
     0.0f, -1.0f,                         // Zoom
     0.0f, 0.0f,                          // FOV Zoom
   }},
  {ROTATE_ALT_AZ,
   {
     0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // Rotation
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
     0.0f, 0.0f,                          // Zoom
     0.0f, 0.0f,                          // FOV Zoom
   }},
  {ROTATE_PITCH_ROLL,
   {
     0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // Rotation
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
     0.0f, 0.0f,                          // Zoom
     0.0f, 0.0f,                          // FOV Zoom
   }},
  {ZOOM_FOV,
   {
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Rotation
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Translation
     0.0f, 0.0f,                          // Zoom
     0.0f, -1.0f,                         // FOV Zoom
   }},
  // clang-format on
};
};  // namespace MouseConfig
