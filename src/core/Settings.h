#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <array>
#include <sstream>
#include <tuple>

#include "io/export_enums.h"
#include "core/SettingsGuiEnums.h"

namespace Settings {

// Note that those 2 values also relate to the currently
// static list of fields in the preferences GUI, so updating
// here needs a change in the UI definition!
constexpr inline size_t max_axis = 9;
constexpr inline size_t max_buttons = 24;

// Property name in GUI designer for matching enum values
constexpr inline auto PROPERTY_NAME = "_settings_value";
// Additional value for enums that can map to an additional value (e.g. GridSize in PDF Export)
constexpr inline auto PROPERTY_SELECTED_VALUE = "_selected_value";

constexpr inline auto SECTION_PYTHON = "python";
constexpr inline auto SECTION_EXPORT_PDF = "export-pdf";
constexpr inline auto SECTION_EXPORT_3MF = "export-3mf";
constexpr inline auto SECTION_EXPORT_SVG = "export-svg";
constexpr inline auto SECTION_COLOR_LIST = "color-list";

class SettingsEntryBase
{
public:
  const std::string& category() const { return _category; }
  const std::string& name() const { return _name; }
  const std::string key() const { return category() + "/" + name(); }

  virtual bool isDefault() const = 0;
  virtual std::string encode() const = 0;
  virtual void set(const std::string& encoded) = 0;
  virtual const std::tuple<std::string, std::string> help() const = 0;

protected:
  SettingsEntryBase(std::string category, std::string name);
  virtual ~SettingsEntryBase() = default;

private:
  std::string _category;
  std::string _name;
};

template <typename entry_type>
class SettingsEntry : public SettingsEntryBase
{
public:
  using entry_type_t = entry_type;

  virtual const entry_type decode(const std::string& encoded) const = 0;

protected:
  SettingsEntry(const std::string& category, const std::string& name) : SettingsEntryBase(category, name)
  {
  }
  virtual ~SettingsEntry() = default;
};

class SettingsEntryBool : public SettingsEntry<bool>
{
public:
  SettingsEntryBool(const std::string& category, const std::string& name, bool defaultValue)
    : SettingsEntry(category, name), _value(defaultValue), _defaultValue(defaultValue)
  {
  }

  bool value() const { return _value; }
  void setValue(bool value) { _value = value; }
  bool defaultValue() const { return _defaultValue; }
  bool isDefault() const override { return _value == _defaultValue; }
  std::string encode() const override;
  const bool decode(const std::string& encoded) const override;
  void set(const std::string& encoded) override { setValue(decode(encoded)); }
  const std::tuple<std::string, std::string> help() const override
  {
    return {"bool", defaultValue() ? "<true>/false" : "true/<false>"};
  }

private:
  bool _value;
  bool _defaultValue;
};

class SettingsEntryInt : public SettingsEntry<int>
{
public:
  SettingsEntryInt(const std::string& category, const std::string& name, int minimum, int maximum,
                   int defaultValue)
    : SettingsEntry(category, name),
      _value(defaultValue),
      _defaultValue(defaultValue),
      _minimum(minimum),
      _maximum(maximum)
  {
  }

  int value() const { return _value; }
  void setValue(int value) { _value = value; }
  int minimum() const { return _minimum; }
  int maximum() const { return _maximum; }
  int defaultValue() const { return _defaultValue; }
  bool isDefault() const override { return _value == _defaultValue; }
  std::string encode() const override;
  const int decode(const std::string& encoded) const override;
  void set(const std::string& encoded) override { setValue(decode(encoded)); }
  const std::tuple<std::string, std::string> help() const override
  {
    return {"int", std::to_string(_minimum) + " : <" + std::to_string(defaultValue()) +
                     "> : " + std::to_string(maximum())};
  }

private:
  int _value;
  int _defaultValue;
  int _minimum;
  int _maximum;
};

class SettingsEntryDouble : public SettingsEntry<double>
{
public:
  SettingsEntryDouble(const std::string& category, const std::string& name, double minimum, double step,
                      double maximum, double defaultValue)
    : SettingsEntry(category, name),
      _value(defaultValue),
      _defaultValue(defaultValue),
      _minimum(minimum),
      _step(step),
      _maximum(maximum)
  {
  }

  double value() const { return _value; }
  void setValue(double value) { _value = value; }
  double minimum() const { return _minimum; }
  double step() const { return _step; }
  double maximum() const { return _maximum; }
  double defaultValue() const { return _defaultValue; }
  bool isDefault() const override { return _value == _defaultValue; }
  std::string encode() const override;
  const double decode(const std::string& encoded) const override;
  void set(const std::string& encoded) override { setValue(decode(encoded)); }
  const std::tuple<std::string, std::string> help() const override
  {
    return {"double", std::to_string(_minimum) + " : <" + std::to_string(defaultValue()) +
                        "> : " + std::to_string(maximum())};
  }

private:
  double _value;
  double _defaultValue;
  double _minimum;
  double _step;
  double _maximum;
};

class SettingsEntryString : public SettingsEntry<std::string>
{
public:
  SettingsEntryString(const std::string& category, const std::string& name,
                      const std::string& defaultValue)
    : SettingsEntry(category, name), _value(defaultValue), _defaultValue(defaultValue)
  {
  }

  const std::string& value() const { return _value; }
  void setValue(const std::string& value) { _value = value; }
  const std::string& defaultValue() const { return _defaultValue; }
  bool isDefault() const override { return _value == _defaultValue; }
  std::string encode() const override { return value(); }
  const std::string decode(const std::string& encoded) const override { return encoded; }
  void set(const std::string& encoded) override { setValue(decode(encoded)); }
  const std::tuple<std::string, std::string> help() const override
  {
    return {"string", "\"" + encode() + "\""};
  }

private:
  std::string _value;
  std::string _defaultValue;
};

template <typename enum_type>
class SettingsEntryEnum : public SettingsEntry<enum_type>
{
public:
  struct Item {
    Item(enum_type value, std::string name, std::string description)
      : value(value), name(std::move(name)), description(std::move(description))
    {
    }
    enum_type value;
    std::string name;
    std::string description;
  };
  SettingsEntryEnum(const std::string& category, const std::string& name, std::vector<Item> items,
                    enum_type defaultValue)
    : SettingsEntry<enum_type>(category, name),
      _items(std::move(items)),
      _defaultValue(std::move(defaultValue))
  {
    setValue(_defaultValue);
  }

  const Item& item() const { return _items[_index]; }
  const enum_type& value() const { return item().value; }
  size_t index() const { return _index; }
  void setValue(const enum_type& value);
  void setIndex(size_t index)
  {
    if (index < _items.size()) _index = index;
  }
  const std::vector<Item>& items() const { return _items; }
  const enum_type& defaultValue() const { return _defaultValue; }
  bool isDefault() const override { return value() == _defaultValue; }
  std::string encode() const override;
  const enum_type decode(const std::string& encoded) const override;
  void set(const std::string& encoded) override { setValue(decode(encoded)); }
  const std::tuple<std::string, std::string> help() const override
  {
    std::string sep = "[";
    std::string list = "";
    for (const auto& item : items()) {
      const auto def = item.value == defaultValue();
      const auto p = def ? "<" : "";
      const auto s = def ? ">" : "";
      list += sep + p + item.name + s;
      sep = ",";
    }
    list += "]";
    return {"enum", list};
  }

private:
  std::vector<Item> _items;
  size_t _index{0};
  enum_type _defaultValue;
};

template <typename enum_type>
void SettingsEntryEnum<enum_type>::setValue(const enum_type& value)
{
  for (size_t i = 0; i < _items.size(); ++i) {
    if (_items[i].value == value) {
      _index = i;
      return;
    }
  }
}

template <typename enum_type>
std::string SettingsEntryEnum<enum_type>::encode() const
{
  return item().name;
}

template <typename enum_type>
const enum_type SettingsEntryEnum<enum_type>::decode(const std::string& encoded) const
{
  for (const Item& item : items()) {
    if (item.name == encoded) {
      return item.value;
    }
  }
  return defaultValue();
}

template <>
inline std::string SettingsEntryEnum<std::string>::encode() const
{
  return value();
}

template <>
inline const std::string SettingsEntryEnum<std::string>::decode(const std::string& encoded) const
{
  return encoded;
}

class LocalAppParameterType
{
public:
  enum Value : uint8_t { invalid, string, file, dir, extension, source, sourcedir };

  LocalAppParameterType() = default;
  constexpr LocalAppParameterType(Value v) : value(v) {}
  constexpr operator Value() const { return value; }
  explicit operator bool() const = delete;

  std::string icon() const
  {
    switch (value) {
    case string:    return "chokusen-parameter";
    case file:      return "chokusen-orthogonal";
    case dir:       return "chokusen-folder";
    case extension: return "chokusen-parameter";
    case source:    return "chokusen-file";
    case sourcedir: return "chokusen-folder";
    default:        return "*invalid*";
    }
  }

  std::string description() const
  {
    switch (value) {
    case string:    return "";
    case file:      return "<full path to the output file>";
    case dir:       return "<directory of the output file>";
    case extension: return "<extension of the output file without leading dot>";
    case source:    return "<full path to the main source file>";
    case sourcedir: return "<directory of the main source file>";
    default:        return "*invalid*";
    }
  }

private:
  Value value;
};

struct LocalAppParameter {
  LocalAppParameterType type;
  std::string value;

  LocalAppParameter() : type(LocalAppParameterType::string), value("") {}
  LocalAppParameter(const LocalAppParameterType t, std::string v) : type(t), value(std::move(v)) {}
  operator bool() const { return type != LocalAppParameterType::invalid; }
};

template <typename item_type>
class SettingsEntryList : public SettingsEntry<std::vector<item_type>>
{
public:
  using list_type_t = std::vector<item_type>;
  SettingsEntryList(const std::string& category, const std::string& name)
    : SettingsEntry<std::vector<item_type>>(category, name)
  {
  }
  const list_type_t& value() const { return _items; }
  void setValue(const list_type_t& items) { _items = items; }
  bool isDefault() const override { return _items.empty(); }
  std::string encode() const override
  {
    std::ostringstream oss;
    for (const auto& item : _items) {
      oss << item;
    }
    return oss.str();
  }
  const std::vector<item_type> decode(const std::string& encoded) const override
  {
    std::vector<item_type> items;
    std::stringstream ss;
    ss << encoded;
    while (ss.good()) {
      item_type item;
      ss >> item;
      if (item) {
        items.push_back(item);
      }
    }
    return items;
  }
  void set(const std::string& encoded) override { setValue(decode(encoded)); }
  const std::tuple<std::string, std::string> help() const override { return {"list", ""}; }

private:
  list_type_t _items;
};

class SettingsVisitor;

class Settings
{
public:
  static SettingsEntryBool showWarningsIn3dView;
  static SettingsEntryBool mouseCentricZoom;
  static SettingsEntryInt indentationWidth;
  static SettingsEntryInt tabWidth;
  static SettingsEntryEnum<std::string> lineWrap;
  static SettingsEntryEnum<std::string> lineWrapIndentationStyle;
  static SettingsEntryInt lineWrapIndentation;
  static SettingsEntryEnum<std::string> lineWrapVisualizationBegin;
  static SettingsEntryEnum<std::string> lineWrapVisualizationEnd;
  static SettingsEntryEnum<std::string> showWhitespace;
  static SettingsEntryInt showWhitespaceSize;
  static SettingsEntryBool autoIndent;
  static SettingsEntryBool backspaceUnindents;
  static SettingsEntryEnum<std::string> indentStyle;
  static SettingsEntryEnum<std::string> tabKeyFunction;
  static SettingsEntryBool highlightCurrentLine;
  static SettingsEntryBool enableBraceMatching;
  static SettingsEntryBool enableLineNumbers;
  static SettingsEntryBool enableNumberScrollWheel;
  static SettingsEntryEnum<std::string> modifierNumberScrollWheel;

  static SettingsEntryString defaultPrintService;
  static SettingsEntryBool enableRemotePrintServices;
  static SettingsEntryBool printServiceAlwaysShowDialog;
  static SettingsEntryString printServiceName;
  static SettingsEntryEnum<std::string> printServiceFileFormat;

  static SettingsEntryString octoPrintUrl;
  static SettingsEntryString octoPrintApiKey;
  static SettingsEntryEnum<std::string> octoPrintFileFormat;
  static SettingsEntryEnum<std::string> octoPrintAction;
  static SettingsEntryString octoPrintSlicerEngine;
  static SettingsEntryString octoPrintSlicerEngineDesc;
  static SettingsEntryString octoPrintSlicerProfile;
  static SettingsEntryString octoPrintSlicerProfileDesc;

  static SettingsEntryString localAppExecutable;
  static SettingsEntryString localAppTempDir;
  static SettingsEntryList<LocalAppParameter> localAppParameterList;
  static SettingsEntryEnum<std::string> localAppFileFormat;

  static SettingsEntryBool manifoldEnabled;
  static SettingsEntryEnum<std::string> renderBackend3D;
  static SettingsEntryEnum<std::string> toolbarExport3D;
  static SettingsEntryEnum<std::string> toolbarExport2D;

  static SettingsEntryBool summaryCamera;
  static SettingsEntryBool summaryArea;
  static SettingsEntryBool summaryBoundingBox;

  static SettingsEntryBool inputEnableDriverHIDAPI;
  static SettingsEntryBool inputEnableDriverHIDAPILog;
  static SettingsEntryBool inputEnableDriverSPNAV;
  static SettingsEntryBool inputEnableDriverJOYSTICK;
  static SettingsEntryBool inputEnableDriverQGAMEPAD;
  static SettingsEntryBool inputEnableDriverDBUS;

  static SettingsEntryEnum<std::string> inputTranslationX;
  static SettingsEntryEnum<std::string> inputTranslationY;
  static SettingsEntryEnum<std::string> inputTranslationZ;
  static SettingsEntryEnum<std::string> inputTranslationXVPRel;
  static SettingsEntryEnum<std::string> inputTranslationYVPRel;
  static SettingsEntryEnum<std::string> inputTranslationZVPRel;
  static SettingsEntryEnum<std::string> inputRotateX;
  static SettingsEntryEnum<std::string> inputRotateY;
  static SettingsEntryEnum<std::string> inputRotateZ;
  static SettingsEntryEnum<std::string> inputRotateXVPRel;
  static SettingsEntryEnum<std::string> inputRotateYVPRel;
  static SettingsEntryEnum<std::string> inputRotateZVPRel;
  static SettingsEntryEnum<std::string> inputZoom;
  static SettingsEntryEnum<std::string> inputZoom2;
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
  static SettingsEntryInt inputMousePreset;
  static SettingsEntryInt inputMouseLeftClick;
  static SettingsEntryInt inputMouseMiddleClick;
  static SettingsEntryInt inputMouseRightClick;
  static SettingsEntryInt inputMouseShiftLeftClick;
  static SettingsEntryInt inputMouseShiftMiddleClick;
  static SettingsEntryInt inputMouseShiftRightClick;
  static SettingsEntryInt inputMouseCtrlLeftClick;
  static SettingsEntryInt inputMouseCtrlMiddleClick;
  static SettingsEntryInt inputMouseCtrlRightClick;
  static SettingsEntryInt inputMouseCtrlShiftLeftClick;
  static SettingsEntryInt inputMouseCtrlShiftMiddleClick;
  static SettingsEntryInt inputMouseCtrlShiftRightClick;
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

  static void visit(const SettingsVisitor& visitor);
};

class SettingsPython
{
public:
  static SettingsEntryString pythonTrustedFiles;
  static SettingsEntryString pythonVirtualEnv;
};

class SettingsExportPdf
{
public:
  static SettingsEntryBool exportPdfAlwaysShowDialog;
  static SettingsEntryEnum<ExportPdfPaperSize> exportPdfPaperSize;
  static SettingsEntryEnum<ExportPdfPaperOrientation> exportPdfOrientation;
  static SettingsEntryBool exportPdfShowFilename;
  static SettingsEntryBool exportPdfShowScale;
  static SettingsEntryBool exportPdfShowScaleMessage;
  static SettingsEntryBool exportPdfShowGrid;
  static SettingsEntryDouble exportPdfGridSize;
  static SettingsEntryBool exportPdfAddMetaData;
  static SettingsEntryBool exportPdfAddMetaDataAuthor;
  static SettingsEntryBool exportPdfAddMetaDataSubject;
  static SettingsEntryBool exportPdfAddMetaDataKeywords;
  static SettingsEntryString exportPdfMetaDataTitle;
  static SettingsEntryString exportPdfMetaDataAuthor;
  static SettingsEntryString exportPdfMetaDataSubject;
  static SettingsEntryString exportPdfMetaDataKeywords;
  static SettingsEntryBool exportPdfFill;
  static SettingsEntryString exportPdfFillColor;
  static SettingsEntryBool exportPdfStroke;
  static SettingsEntryString exportPdfStrokeColor;
  static SettingsEntryDouble exportPdfStrokeWidth;

  static constexpr std::array<const SettingsEntryBase *, 17> cmdline{
    &exportPdfPaperSize,      &exportPdfOrientation,      &exportPdfShowFilename,
    &exportPdfShowScale,      &exportPdfShowScaleMessage, &exportPdfShowGrid,
    &exportPdfGridSize,       &exportPdfAddMetaData,      &exportPdfMetaDataTitle,
    &exportPdfMetaDataAuthor, &exportPdfMetaDataSubject,  &exportPdfMetaDataKeywords,
    &exportPdfFill,           &exportPdfFillColor,        &exportPdfStroke,
    &exportPdfStrokeColor,    &exportPdfStrokeWidth,
  };
};

class SettingsExport3mf
{
public:
  static SettingsEntryBool export3mfAlwaysShowDialog;
  static SettingsEntryEnum<Export3mfColorMode> export3mfColorMode;
  static SettingsEntryEnum<Export3mfUnit> export3mfUnit;
  static SettingsEntryString export3mfColor;
  static SettingsEntryEnum<Export3mfMaterialType> export3mfMaterialType;
  static SettingsEntryInt export3mfDecimalPrecision;
  static SettingsEntryBool export3mfAddMetaData;
  static SettingsEntryBool export3mfAddMetaDataDesigner;
  static SettingsEntryBool export3mfAddMetaDataDescription;
  static SettingsEntryBool export3mfAddMetaDataCopyright;
  static SettingsEntryBool export3mfAddMetaDataLicenseTerms;
  static SettingsEntryBool export3mfAddMetaDataRating;
  static SettingsEntryString export3mfMetaDataTitle;
  static SettingsEntryString export3mfMetaDataDesigner;
  static SettingsEntryString export3mfMetaDataDescription;
  static SettingsEntryString export3mfMetaDataCopyright;
  static SettingsEntryString export3mfMetaDataLicenseTerms;
  static SettingsEntryString export3mfMetaDataRating;

  static constexpr std::array<const SettingsEntryBase *, 12> cmdline{
    &export3mfColorMode,
    &export3mfUnit,
    &export3mfColor,
    &export3mfMaterialType,
    &export3mfDecimalPrecision,
    &export3mfAddMetaData,
    &export3mfMetaDataTitle,
    &export3mfMetaDataDesigner,
    &export3mfMetaDataDescription,
    &export3mfMetaDataCopyright,
    &export3mfMetaDataLicenseTerms,
    &export3mfMetaDataRating,
  };
};

class SettingsExportSvg
{
public:
  static SettingsEntryBool exportSvgAlwaysShowDialog;
  static SettingsEntryBool exportSvgFill;
  static SettingsEntryString exportSvgFillColor;
  static SettingsEntryBool exportSvgStroke;
  static SettingsEntryString exportSvgStrokeColor;
  static SettingsEntryDouble exportSvgStrokeWidth;

  static constexpr std::array<const SettingsEntryBase *, 5> cmdline{
    &exportSvgFill, &exportSvgFillColor, &exportSvgStroke, &exportSvgStrokeColor, &exportSvgStrokeWidth,
  };
};

class SettingsColorList
{
public:
  static SettingsEntryBool colorListWebColors;
  static SettingsEntryBool colorListXkcdColors;
  static SettingsEntryBool colorListSortAscending;
  static SettingsEntryEnum<ColorListFilterType> colorListFilterType;
  static SettingsEntryEnum<ColorListSortType> colorListSortType;
};

class SettingsVisitor
{
public:
  SettingsVisitor() = default;
  virtual ~SettingsVisitor() = default;

  virtual void handle(SettingsEntryBase& entry) const = 0;
};

}  // namespace Settings
