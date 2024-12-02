#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Settings {

class SettingsEntry
{
public:
  const std::string& category() const { return _category; }
  const std::string& name() const { return _name; }
  const std::string key() const { return category() + "/" + name(); }

  virtual bool isDefault() const = 0;
  virtual std::string encode() const = 0;
  virtual void decode(const std::string& encoded) = 0;

protected:
  SettingsEntry(std::string category, std::string name);
  virtual ~SettingsEntry() = default;

private:
  std::string _category;
  std::string _name;
};

class SettingsEntryBool : public SettingsEntry
{
public:
  SettingsEntryBool(const std::string& category, const std::string& name, bool defaultValue) :
    SettingsEntry(category, name),
    _value(defaultValue),
    _defaultValue(defaultValue)
  {}

  bool value() const { return _value; }
  void setValue(bool value) { _value = value; }
  bool isDefault() const override { return _value == _defaultValue; }
  std::string encode() const override;
  void decode(const std::string& encoded) override;

private:
  bool _value;
  bool _defaultValue;
};

class SettingsEntryInt : public SettingsEntry
{
public:
  SettingsEntryInt(const std::string& category, const std::string& name, int minimum, int maximum, int defaultValue) :
    SettingsEntry(category, name),
    _value(defaultValue),
    _defaultValue(defaultValue),
    _minimum(minimum),
    _maximum(maximum)
  {}

  int value() const { return _value; }
  void setValue(int value) { _value = value; }
  int minimum() const { return _minimum; }
  int maximum() const { return _maximum; }
  bool isDefault() const override { return _value == _defaultValue; }
  std::string encode() const override;
  void decode(const std::string& encoded) override;

private:
  int _value;
  int _defaultValue;
  int _minimum;
  int _maximum;
};

class SettingsEntryDouble : public SettingsEntry
{
public:
  SettingsEntryDouble(const std::string& category, const std::string& name, double minimum, double step, double maximum, double defaultValue) :
    SettingsEntry(category, name),
    _value(defaultValue),
    _defaultValue(defaultValue),
    _minimum(minimum),
    _step(step),
    _maximum(maximum)
  {}

  double value() const { return _value; }
  void setValue(double value) { _value = value; }
  double minimum() const { return _minimum; }
  double step() const { return _step; }
  double maximum() const { return _maximum; }
  bool isDefault() const override { return _value == _defaultValue; }
  std::string encode() const override;
  void decode(const std::string& encoded) override;

private:
  double _value;
  double _defaultValue;
  double _minimum;
  double _step;
  double _maximum;
};

class SettingsEntryString : public SettingsEntry
{
public:
  SettingsEntryString(const std::string& category, const std::string& name, const std::string& defaultValue) :
    SettingsEntry(category, name),
    _value(defaultValue),
    _defaultValue(defaultValue)
  {}

  const std::string& value() const { return _value; }
  void setValue(const std::string& value) { _value = value; }
  bool isDefault() const override { return _value == _defaultValue; }
  std::string encode() const override { return value(); }
  void decode(const std::string& encoded) override { setValue(encoded); }

private:
  std::string _value;
  std::string _defaultValue;
};

class SettingsEntryEnum : public SettingsEntry
{
public:
  struct Item {
    std::string value;
    std::string description;
  };
  SettingsEntryEnum(const std::string& category, const std::string& name, std::vector<Item> items, std::string defaultValue) :
    SettingsEntry(category, name),
    _items(std::move(items)),
    _defaultValue(std::move(defaultValue))
  {
    setValue(_defaultValue);
  }

  const std::string& value() const { return _items[_index].value; }
  size_t index() const { return _index; }
  void setValue(const std::string& value);
  void setIndex(size_t index) { if (index < _items.size()) _index = index; }
  const std::vector<Item>& items() const { return _items; }
  bool isDefault() const override { return value() == _defaultValue; }
  std::string encode() const override { return value(); }
  void decode(const std::string& encoded) override { setValue(encoded); }

private:
  std::vector<Item> _items;
  size_t _index{0};
  std::string _defaultValue;
};

class SettingsVisitor;

class Settings
{
public:
  static SettingsEntryBool showWarningsIn3dView;
  static SettingsEntryBool mouseCentricZoom;
  static SettingsEntryBool mouseSwapButtons;
  static SettingsEntryInt indentationWidth;
  static SettingsEntryInt tabWidth;
  static SettingsEntryEnum lineWrap;
  static SettingsEntryEnum lineWrapIndentationStyle;
  static SettingsEntryInt lineWrapIndentation;
  static SettingsEntryEnum lineWrapVisualizationBegin;
  static SettingsEntryEnum lineWrapVisualizationEnd;
  static SettingsEntryEnum showWhitespace;
  static SettingsEntryInt showWhitespaceSize;
  static SettingsEntryBool autoIndent;
  static SettingsEntryBool backspaceUnindents;
  static SettingsEntryEnum indentStyle;
  static SettingsEntryEnum tabKeyFunction;
  static SettingsEntryBool highlightCurrentLine;
  static SettingsEntryBool enableBraceMatching;
  static SettingsEntryBool enableLineNumbers;
  static SettingsEntryBool enableNumberScrollWheel;
  static SettingsEntryEnum modifierNumberScrollWheel;
  static SettingsEntryString defaultPrintService;
  static SettingsEntryString printServiceName;
  static SettingsEntryString printServiceFileFormat;
  static SettingsEntryString octoPrintUrl;
  static SettingsEntryString octoPrintApiKey;
  static SettingsEntryEnum octoPrintFileFormat;
  static SettingsEntryEnum localSlicerFileFormat;
  static SettingsEntryEnum octoPrintAction;
  static SettingsEntryString octoPrintSlicerEngine;
  static SettingsEntryString octoPrintSlicerEngineDesc;
  static SettingsEntryString octoPrintSlicerProfile;
  static SettingsEntryString octoPrintSlicerProfileDesc;
  static SettingsEntryString localSlicerExecutable;

  static SettingsEntryBool manifoldEnabled;
  static SettingsEntryEnum renderBackend3D;
  static SettingsEntryEnum toolbarExport3D;
  static SettingsEntryEnum toolbarExport2D;

  static SettingsEntryBool summaryCamera;
  static SettingsEntryBool summaryArea;
  static SettingsEntryBool summaryBoundingBox;

  static SettingsEntryBool inputEnableDriverHIDAPI;
  static SettingsEntryBool inputEnableDriverHIDAPILog;
  static SettingsEntryBool inputEnableDriverSPNAV;
  static SettingsEntryBool inputEnableDriverJOYSTICK;
  static SettingsEntryBool inputEnableDriverQGAMEPAD;
  static SettingsEntryBool inputEnableDriverDBUS;

  static SettingsEntryEnum inputTranslationX;
  static SettingsEntryEnum inputTranslationY;
  static SettingsEntryEnum inputTranslationZ;
  static SettingsEntryEnum inputTranslationXVPRel;
  static SettingsEntryEnum inputTranslationYVPRel;
  static SettingsEntryEnum inputTranslationZVPRel;
  static SettingsEntryEnum inputRotateX;
  static SettingsEntryEnum inputRotateY;
  static SettingsEntryEnum inputRotateZ;
  static SettingsEntryEnum inputRotateXVPRel;
  static SettingsEntryEnum inputRotateYVPRel;
  static SettingsEntryEnum inputRotateZVPRel;
  static SettingsEntryEnum inputZoom;
  static SettingsEntryEnum inputZoom2;
  static SettingsEntryDouble inputTranslationGain;
  static SettingsEntryDouble inputTranslationVPRelGain;
  static SettingsEntryDouble inputRotateGain;
  static SettingsEntryDouble inputRotateVPRelGain;
  static SettingsEntryDouble inputZoomGain;
  static SettingsEntryString inputButton0;
  static SettingsEntryString inputButton1;
  static SettingsEntryString inputButton2;
  static SettingsEntryString inputButton3;
  static SettingsEntryString inputButton4;
  static SettingsEntryString inputButton5;
  static SettingsEntryString inputButton6;
  static SettingsEntryString inputButton7;
  static SettingsEntryString inputButton8;
  static SettingsEntryString inputButton9;
  static SettingsEntryString inputButton10;
  static SettingsEntryString inputButton11;
  static SettingsEntryString inputButton12;
  static SettingsEntryString inputButton13;
  static SettingsEntryString inputButton14;
  static SettingsEntryString inputButton15;
  static SettingsEntryString inputButton16;
  static SettingsEntryString inputButton17;
  static SettingsEntryString inputButton18;
  static SettingsEntryString inputButton19;
  static SettingsEntryString inputButton20;
  static SettingsEntryString inputButton21;
  static SettingsEntryString inputButton22;
  static SettingsEntryString inputButton23;
  static SettingsEntryDouble axisTrim0;
  static SettingsEntryDouble axisTrim1;
  static SettingsEntryDouble axisTrim2;
  static SettingsEntryDouble axisTrim3;
  static SettingsEntryDouble axisTrim4;
  static SettingsEntryDouble axisTrim5;
  static SettingsEntryDouble axisTrim6;
  static SettingsEntryDouble axisTrim7;
  static SettingsEntryDouble axisTrim8;
  static SettingsEntryDouble axisDeadzone0;
  static SettingsEntryDouble axisDeadzone1;
  static SettingsEntryDouble axisDeadzone2;
  static SettingsEntryDouble axisDeadzone3;
  static SettingsEntryDouble axisDeadzone4;
  static SettingsEntryDouble axisDeadzone5;
  static SettingsEntryDouble axisDeadzone6;
  static SettingsEntryDouble axisDeadzone7;
  static SettingsEntryDouble axisDeadzone8;
  static SettingsEntryInt joystickNr;

  static SettingsEntryString& inputButton(size_t id);
  static SettingsEntryDouble& axisTrim(size_t id);
  static SettingsEntryDouble& axisDeadzone(size_t id);

  static void visit(const SettingsVisitor& visitor);
};

class SettingsVisitor
{
public:
  SettingsVisitor() = default;
  virtual ~SettingsVisitor() = default;

  virtual void handle(SettingsEntry& entry) const = 0;
};

} // namespace Settings
