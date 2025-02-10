#include "core/Settings.h"

#include <ostream>
#include <cassert>
#include <cstddef>
#include <istream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptors.hpp>

#include "io/export_enums.h"
#include "io/export.h"
#include "utils/printutils.h"

#include "json/json.hpp"

using json = nlohmann::json;

namespace Settings {

namespace {

std::vector<SettingsEntryBase *> entries;

std::vector<SettingsEntryEnum<std::string>::Item> createFileFormatItems(std::vector<FileFormat> formats) {
  std::vector<SettingsEntryEnum<std::string>::Item> items;
  std::transform(formats.begin(), formats.end(), std::back_inserter(items),
                 [](const FileFormat& format){
    const FileFormatInfo &info = fileformat::info(format);
    return SettingsEntryEnum<std::string>::Item{info.identifier, info.identifier, info.description};
    });
  return items;
}

std::vector<SettingsEntryEnum<std::string>::Item> axisValues() {
  std::vector<SettingsEntryEnum<std::string>::Item> output;
  output.push_back({"None", "none", _("None")});
  for (size_t i = 0; i < max_axis; ++i) {
    const auto userData = (boost::format("+%d") % (i + 1)).str();
    const auto name = (boost::format(_("axis-%d")) % i).str();
    const auto text = (boost::format(_("Axis %d")) % i).str();
    output.push_back({userData, name, text});
    const auto userDataInv = (boost::format("-%d") % (i + 1)).str();
    const auto nameInv = (boost::format(_("axis-inverted-%d")) % i).str();
    const auto textInv = (boost::format(_("Axis %d (inverted)")) % i).str();
    output.push_back({userDataInv, nameInv, textInv});
  }
  return output;
}

} // namespace

void Settings::visit(const SettingsVisitor& visitor)
{
  for (SettingsEntryBase *entry : entries) {
    visitor.handle(*entry);
  }
}

SettingsEntryBase::SettingsEntryBase(std::string category, std::string name) :
  _category(std::move(category)), _name(std::move(name))
{
  entries.push_back(this);
}

std::string SettingsEntryBool::encode() const
{
  return _value ? "true" : "false";
}

const bool SettingsEntryBool::decode(const std::string& encoded) const
{
  std::string trimmed = boost::algorithm::trim_copy(encoded);
  if (trimmed == "true") {
    return true;
  } else if (trimmed == "false") {
    return false;
  } else {
    try {
      return boost::lexical_cast<bool>(trimmed);
    } catch (const boost::bad_lexical_cast&) {
      return defaultValue();
    }
  }
}

std::string SettingsEntryInt::encode() const
{
  return STR(_value);
}

const int SettingsEntryInt::decode(const std::string& encoded) const
{
  try {
    return boost::lexical_cast<int>(boost::algorithm::trim_copy(encoded));
  } catch (const boost::bad_lexical_cast&) {
    return defaultValue();
  }
}

std::string SettingsEntryDouble::encode() const
{
  return STR(_value);
}

const double SettingsEntryDouble::decode(const std::string& encoded) const
{
  try {
    return boost::lexical_cast<double>(boost::algorithm::trim_copy(encoded));
  } catch (const boost::bad_lexical_cast&) {
    return defaultValue();
  }
}

std::ostream& operator<<(std::ostream& stream, const LocalAppParameter& param)
{
  json data;
  data["type"] = static_cast<int>(param.type);
  if (!param.value.empty()) {
    data["value"] = param.value;
  }
  stream << data.dump();
  return stream;
}

std::istream& operator>>(std::istream& stream, LocalAppParameter& param)
{
  try {
    json data;
    stream >> data;
    param.type = static_cast<LocalAppParameterType>(data["type"]);
    if (data.contains("value")) {
      param.value = data["value"];
    }
  } catch (const json::exception& e) {
    param.type = LocalAppParameterType::invalid;
    param.value = "";
  }
  return stream;
}

SettingsEntryBool Settings::showWarningsIn3dView("3dview", "showWarningsIn3dView", true);
SettingsEntryBool Settings::mouseCentricZoom("3dview", "mouseCentricZoom", true);
SettingsEntryBool Settings::mouseSwapButtons("3dview", "mouseSwapButtons", false);
SettingsEntryInt Settings::indentationWidth("editor", "indentationWidth", 1, 16, 4);
SettingsEntryInt Settings::tabWidth("editor", "tabWidth", 1, 16, 4);
SettingsEntryEnum<std::string> Settings::lineWrap("editor", "lineWrap", {
  {"None", "none", _("None")},
  {"Char", "char", _("Wrap at character boundaries")},
  {"Word", "word", _("Wrap at word boundaries")}
}, "Word");
SettingsEntryEnum<std::string> Settings::lineWrapIndentationStyle("editor", "lineWrapIndentationStyle", {
  {"Fixed",    "fixed",    _("Fixed")},
  {"Same",     "same",     _("Same")},
  {"Indented", "indented", _("Indented")}
}, "Fixed");
SettingsEntryInt Settings::lineWrapIndentation("editor", "lineWrapIndentation", 0, 999, 4);
SettingsEntryEnum<std::string> Settings::lineWrapVisualizationBegin("editor", "lineWrapVisualizationBegin", {
  {"None",   "none",   _("None")},
  {"Text",   "text"  , _("Text")},
  {"Border", "border", _("Border")},
  {"Margin", "margin", _("Margin")}
}, "None");
SettingsEntryEnum<std::string> Settings::lineWrapVisualizationEnd("editor", "lineWrapVisualizationEnd", {
  {"None",   "none",   _("None")},
  {"Text",   "text",   _("Text")},
  {"Border", "border", _("Border")},
  {"Margin", "margin", _("Margin")}
}, "Border");
SettingsEntryEnum<std::string> Settings::showWhitespace("editor", "showWhitespaces", {
  {"Never",            "never",        _("Never")},
  {"Always",           "always",       _("Always")},
  {"AfterIndentation", "after-indent", _("After indentation")}
}, "Never");
SettingsEntryInt Settings::showWhitespaceSize("editor", "showWhitespacesSize", 1, 16, 2);
SettingsEntryBool Settings::autoIndent("editor", "autoIndent", true);
SettingsEntryBool Settings::backspaceUnindents("editor", "backspaceUnindents", false);
SettingsEntryEnum<std::string> Settings::indentStyle("editor", "indentStyle", {
  {"Spaces", "spaces", _("Spaces")},
  {"Tabs",   "tabs",   _("Tabs")}
}, "spaces");
SettingsEntryEnum<std::string> Settings::tabKeyFunction("editor", "tabKeyFunction", {
  {"Indent",    "indent", _("Indent")},
  {"InsertTab", "tab",    _("Insert Tab")}
}, "Indent");
SettingsEntryBool Settings::highlightCurrentLine("editor", "highlightCurrentLine", true);
SettingsEntryBool Settings::enableBraceMatching("editor", "enableBraceMatching", true);
SettingsEntryBool Settings::enableLineNumbers("editor", "enableLineNumbers", true);
SettingsEntryBool Settings::enableNumberScrollWheel("editor", "enableNumberScrollWheel", true);
SettingsEntryEnum<std::string> Settings::modifierNumberScrollWheel("editor", "modifierNumberScrollWheel", {
  {"Alt",               "alt",               _("Alt")},
  {"Left Mouse Button", "left-mouse-button", _("Left Mouse Button")},
  {"Either",            "either",            _("Either")}
}, "Alt");

SettingsEntryString Settings::defaultPrintService("printing", "printService", "NONE");

SettingsEntryString Settings::printServiceName("printing", "printServiceName", "");
SettingsEntryString Settings::printServiceFileFormat("printing", "printServiceFileFormat", "stl");

SettingsEntryString Settings::octoPrintUrl("printing", "octoPrintUrl", "");
SettingsEntryString Settings::octoPrintApiKey("printing", "octoPrintApiKey", "");
SettingsEntryEnum<std::string> Settings::octoPrintAction("printing", "octoPrintAction", {
  {"upload", "upload", _("Upload only")},
  {"slice",  "slice",  _("Upload & Slice")},
  {"select", "select", _("Upload, Slice & Select for printing")},
  {"print",  "print",  _("Upload, Slice & Start printing")}
}, "upload");
SettingsEntryString Settings::octoPrintSlicerEngine("printing", "octoPrintSlicerEngine", "");
SettingsEntryString Settings::octoPrintSlicerEngineDesc("printing", "octoPrintSlicerEngineDesc", "");
SettingsEntryString Settings::octoPrintSlicerProfile("printing", "octoPrintSlicerProfile", "");
SettingsEntryString Settings::octoPrintSlicerProfileDesc("printing", "octoPrintSlicerProfileDesc", "");
SettingsEntryEnum<std::string> Settings::octoPrintFileFormat(
    "printing", "octoPrintFileFormat",
    createFileFormatItems({FileFormat::ASCII_STL, FileFormat::BINARY_STL, FileFormat::_3MF, FileFormat::OFF}),
    fileformat::info(FileFormat::ASCII_STL).description);

SettingsEntryString Settings::localAppExecutable("printing", "localAppExecutable", "");
SettingsEntryString Settings::localAppTempDir("printing", "localAppTempDir", "");
SettingsEntryEnum<std::string> Settings::localAppFileFormat(
    "printing", "localAppFileFormat", createFileFormatItems(fileformat::all3D()),
    fileformat::info(FileFormat::ASCII_STL).description);
SettingsEntryList<LocalAppParameter> Settings::localAppParameterList("printing", "localAppParameterList");

SettingsEntryEnum<std::string> Settings::renderBackend3D("advanced", "renderBackend3D", {
  {"CGAL",     "cgal",     "CGAL (old/slow)"},
  {"Manifold", "manifold", "Manifold (new/fast)"}
}, "CGAL");
SettingsEntryEnum<std::string> Settings::toolbarExport3D("advanced", "toolbarExport3D", createFileFormatItems(fileformat::all3D()), fileformat::info(FileFormat::ASCII_STL).description);
SettingsEntryEnum<std::string> Settings::toolbarExport2D("advanced", "toolbarExport2D", createFileFormatItems(fileformat::all2D()), fileformat::info(FileFormat::DXF).description);

SettingsEntryBool Settings::summaryCamera("summary", "camera", false);
SettingsEntryBool Settings::summaryArea("summary", "measurementArea", false);
SettingsEntryBool Settings::summaryBoundingBox("summary", "boundingBox", false);

SettingsEntryBool Settings::inputEnableDriverHIDAPI("input", "enableDriverHIDAPI", false);
SettingsEntryBool Settings::inputEnableDriverHIDAPILog("input", "enableDriverHIDAPILog", false);
SettingsEntryBool Settings::inputEnableDriverSPNAV("input", "enableDriverSPNAV", false);
SettingsEntryBool Settings::inputEnableDriverJOYSTICK("input", "enableDriverJOYSTICK", false);
SettingsEntryBool Settings::inputEnableDriverQGAMEPAD("input", "enableDriverQGAMEPAD", false);
SettingsEntryBool Settings::inputEnableDriverDBUS("input", "enableDriverDBUS", false);

SettingsEntryEnum<std::string> Settings::inputTranslationX("input", "translationX", axisValues(), "+1");
SettingsEntryEnum<std::string> Settings::inputTranslationY("input", "translationY", axisValues(), "-2");
SettingsEntryEnum<std::string> Settings::inputTranslationZ("input", "translationZ", axisValues(), "-3");
SettingsEntryEnum<std::string> Settings::inputTranslationXVPRel("input", "translationXVPRel", axisValues(), "None");
SettingsEntryEnum<std::string> Settings::inputTranslationYVPRel("input", "translationYVPRel", axisValues(), "None");
SettingsEntryEnum<std::string> Settings::inputTranslationZVPRel("input", "translationZVPRel", axisValues(), "None");
SettingsEntryEnum<std::string> Settings::inputRotateX("input", "rotateX", axisValues(), "+4");
SettingsEntryEnum<std::string> Settings::inputRotateY("input", "rotateY", axisValues(), "-5");
SettingsEntryEnum<std::string> Settings::inputRotateZ("input", "rotateZ", axisValues(), "-6");
SettingsEntryEnum<std::string> Settings::inputRotateXVPRel("input", "rotateXVPRel", axisValues(), "None");
SettingsEntryEnum<std::string> Settings::inputRotateYVPRel("input", "rotateYVPRel", axisValues(), "None");
SettingsEntryEnum<std::string> Settings::inputRotateZVPRel("input", "rotateZVPRel", axisValues(), "None");
SettingsEntryEnum<std::string> Settings::inputZoom("input", "zoom", axisValues(), "None");
SettingsEntryEnum<std::string> Settings::inputZoom2("input", "zoom2", axisValues(), "None");

SettingsEntryDouble Settings::inputTranslationGain("input", "translationGain", 0.01, 0.01, 9.99, 1.00);
SettingsEntryDouble Settings::inputTranslationVPRelGain("input", "translationVPRelGain", 0.01, 0.01, 9.99, 1.00);
SettingsEntryDouble Settings::inputRotateGain("input", "rotateGain", 0.01, 0.01, 9.99, 1.00);
SettingsEntryDouble Settings::inputRotateVPRelGain("input", "rotateVPRelGain", 0.01, 0.01, 9.99, 1.00);
SettingsEntryDouble Settings::inputZoomGain("input", "zoomGain", 0.1, 0.1, 99.9, 1.0);

SettingsEntryString Settings::inputButton0("input", "button0", "");
SettingsEntryString Settings::inputButton1("input", "button1", "");
SettingsEntryString Settings::inputButton2("input", "button2", "");
SettingsEntryString Settings::inputButton3("input", "button3", "");
SettingsEntryString Settings::inputButton4("input", "button4", "");
SettingsEntryString Settings::inputButton5("input", "button5", "");
SettingsEntryString Settings::inputButton6("input", "button6", "");
SettingsEntryString Settings::inputButton7("input", "button7", "");
SettingsEntryString Settings::inputButton8("input", "button8", "");
SettingsEntryString Settings::inputButton9("input", "button9", "");
SettingsEntryString Settings::inputButton10("input", "button10", "");
SettingsEntryString Settings::inputButton11("input", "button11", "");
SettingsEntryString Settings::inputButton12("input", "button12", "");
SettingsEntryString Settings::inputButton13("input", "button13", "");
SettingsEntryString Settings::inputButton14("input", "button14", "");
SettingsEntryString Settings::inputButton15("input", "button15", "");
SettingsEntryString Settings::inputButton16("input", "button16", "");
SettingsEntryString Settings::inputButton17("input", "button17", "");
SettingsEntryString Settings::inputButton18("input", "button18", "");
SettingsEntryString Settings::inputButton19("input", "button19", "");
SettingsEntryString Settings::inputButton20("input", "button20", "");
SettingsEntryString Settings::inputButton21("input", "button21", "");
SettingsEntryString Settings::inputButton22("input", "button22", "");
SettingsEntryString Settings::inputButton23("input", "button23", "");
SettingsEntryDouble Settings::axisTrim0("input", "axisTrim0", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim1("input", "axisTrim1", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim2("input", "axisTrim2", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim3("input", "axisTrim3", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim4("input", "axisTrim4", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim5("input", "axisTrim5", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim6("input", "axisTrim6", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim7("input", "axisTrim7", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisTrim8("input", "axisTrim8", -1.0, 0.01, 1.0, 0.0);
SettingsEntryDouble Settings::axisDeadzone0("input", "axisDeadzone0", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone1("input", "axisDeadzone1", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone2("input", "axisDeadzone2", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone3("input", "axisDeadzone3", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone4("input", "axisDeadzone4", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone5("input", "axisDeadzone5", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone6("input", "axisDeadzone6", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone7("input", "axisDeadzone7", 0.0, 0.01, 1.0, 0.10);
SettingsEntryDouble Settings::axisDeadzone8("input", "axisDeadzone8", 0.0, 0.01, 1.0, 0.10);

SettingsEntryInt Settings::joystickNr("input", "joystickNr", 0, 9, 0);

SettingsEntryBool SettingsExportPdf::exportPdfAlwaysShowDialog(SECTION_EXPORT_PDF, "always-show-sialog", true);
SettingsEntryEnum<ExportPdfPaperSize> SettingsExportPdf::exportPdfPaperSize(SECTION_EXPORT_PDF, "paper-size", {
  {ExportPdfPaperSize::A6,      "a6",      _("A6 (105 x 148 mm)")},
  {ExportPdfPaperSize::A5,      "a5",      _("A5 (148 x 210 mm)")},
  {ExportPdfPaperSize::A4,      "a4",      _("A4 (210x297 mm)")},
  {ExportPdfPaperSize::A3,      "a3",      _("A3 (297x420 mm)")},
  {ExportPdfPaperSize::LETTER,  "letter",  _("Letter (8.5x11 in)")},
  {ExportPdfPaperSize::LEGAL,   "legal",   _("Legal (8.5x14 in)")},
  {ExportPdfPaperSize::TABLOID, "tabloid", _("Tabloid (11x17 in)")}
}, ExportPdfPaperSize::A4);
SettingsEntryEnum<ExportPdfPaperOrientation> SettingsExportPdf::exportPdfOrientation(SECTION_EXPORT_PDF, "orientation", {
  {ExportPdfPaperOrientation::PORTRAIT,  "portrait",  _("Portrait (Vertical)")},
  {ExportPdfPaperOrientation::LANDSCAPE, "landscape", _("Landscape (Horizontal)")},
  {ExportPdfPaperOrientation::AUTO,      "auto",      _("Auto")}
}, ExportPdfPaperOrientation::PORTRAIT);
SettingsEntryBool SettingsExportPdf::exportPdfShowFilename(SECTION_EXPORT_PDF, "show-filename", true);
SettingsEntryBool SettingsExportPdf::exportPdfShowScale(SECTION_EXPORT_PDF, "show-scale", true);
SettingsEntryBool SettingsExportPdf::exportPdfShowScaleMessage(SECTION_EXPORT_PDF, "show-scale-message", true);
SettingsEntryBool SettingsExportPdf::exportPdfShowGrid(SECTION_EXPORT_PDF, "show-grid", true);
SettingsEntryDouble SettingsExportPdf::exportPdfGridSize(SECTION_EXPORT_PDF, "grid-size", 1.0, 1.0, 100.0, 10.0);
SettingsEntryBool SettingsExportPdf::exportPdfAddMetaData(SECTION_EXPORT_PDF, "add-meta-data", true);
SettingsEntryBool SettingsExportPdf::exportPdfAddMetaDataAuthor(SECTION_EXPORT_PDF, "add-meta-data-author", false);
SettingsEntryBool SettingsExportPdf::exportPdfAddMetaDataSubject(SECTION_EXPORT_PDF, "add-meta-data-subject", false);
SettingsEntryBool SettingsExportPdf::exportPdfAddMetaDataKeywords(SECTION_EXPORT_PDF, "add-meta-data-keywords", false);
SettingsEntryString SettingsExportPdf::exportPdfMetaDataTitle(SECTION_EXPORT_PDF, "meta-data-title", "");
SettingsEntryString SettingsExportPdf::exportPdfMetaDataAuthor(SECTION_EXPORT_PDF, "meta-data-author", "");
SettingsEntryString SettingsExportPdf::exportPdfMetaDataSubject(SECTION_EXPORT_PDF, "meta-data-subject", "");
SettingsEntryString SettingsExportPdf::exportPdfMetaDataKeywords(SECTION_EXPORT_PDF, "meta-data-keywords", "");

SettingsEntryBool SettingsExport3mf::export3mfAlwaysShowDialog(SECTION_EXPORT_3MF, "always-show-dialog", true);
SettingsEntryEnum<Export3mfColorMode> SettingsExport3mf::export3mfColorMode(SECTION_EXPORT_3MF, "color-mode", {
  {Export3mfColorMode::model,               "model",               _("Use colors from model")},
  {Export3mfColorMode::none,                "none",                _("No colors")},
  {Export3mfColorMode::selected_only,       "selected-only",       _("Use selected color only")},
}, Export3mfColorMode::model);
SettingsEntryEnum<Export3mfUnit> SettingsExport3mf::export3mfUnit(SECTION_EXPORT_3MF, "unit", {
  {Export3mfUnit::micron,     "micron",     _("Micron")},
  {Export3mfUnit::millimeter, "millimeter", _("Millimeter")},
  {Export3mfUnit::centimeter, "centimeter", _("Centimeter")},
  {Export3mfUnit::meter,      "meter",      _("Meter")},
  {Export3mfUnit::inch,       "inch",       _("Inch")},
  {Export3mfUnit::foot,       "foot",       _("Feet")},
}, Export3mfUnit::millimeter);
SettingsEntryString SettingsExport3mf::export3mfColor(SECTION_EXPORT_3MF, "color", "#f9d72c"); // Cornfield: CGAL_FACE_FRONT_COLOR
SettingsEntryEnum<Export3mfMaterialType> SettingsExport3mf::export3mfMaterialType(SECTION_EXPORT_3MF, "material-type", {
  {Export3mfMaterialType::color,        "color",        _("Color")},
  {Export3mfMaterialType::basematerial, "basematerial", _("Base Material")},
}, Export3mfMaterialType::basematerial);
SettingsEntryInt SettingsExport3mf::export3mfDecimalPrecision(SECTION_EXPORT_3MF, "decimal-precision", 1, 16, 6);
SettingsEntryBool SettingsExport3mf::export3mfAddMetaData(SECTION_EXPORT_3MF, "add-meta-data", true);
SettingsEntryBool SettingsExport3mf::export3mfAddMetaDataDesigner(SECTION_EXPORT_3MF, "add-meta-data-designer", false);
SettingsEntryBool SettingsExport3mf::export3mfAddMetaDataDescription(SECTION_EXPORT_3MF, "add-meta-data-description", false);
SettingsEntryBool SettingsExport3mf::export3mfAddMetaDataCopyright(SECTION_EXPORT_3MF, "add-meta-data-copyright", false);
SettingsEntryBool SettingsExport3mf::export3mfAddMetaDataLicenseTerms(SECTION_EXPORT_3MF, "add-meta-data-license-terms", false);
SettingsEntryBool SettingsExport3mf::export3mfAddMetaDataRating(SECTION_EXPORT_3MF, "add-meta-data-rating", false);
SettingsEntryString SettingsExport3mf::export3mfMetaDataTitle(SECTION_EXPORT_3MF, "meta-data-title", "");
SettingsEntryString SettingsExport3mf::export3mfMetaDataDesigner(SECTION_EXPORT_3MF, "meta-data-designer", "");
SettingsEntryString SettingsExport3mf::export3mfMetaDataDescription(SECTION_EXPORT_3MF, "meta-data-description", "");
SettingsEntryString SettingsExport3mf::export3mfMetaDataCopyright(SECTION_EXPORT_3MF, "meta-data-copyright", "");
SettingsEntryString SettingsExport3mf::export3mfMetaDataLicenseTerms(SECTION_EXPORT_3MF, "meta-data-license-terms", "");
SettingsEntryString SettingsExport3mf::export3mfMetaDataRating(SECTION_EXPORT_3MF, "meta-data-rating", "");

} // namespace Settings
