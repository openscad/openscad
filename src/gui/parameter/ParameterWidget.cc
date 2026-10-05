/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2014 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "gui/parameter/ParameterPyQtWidget.h"
#include "gui/parameter/ParameterWidget.h"

#include <QAction>
#include <QCoreApplication>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QString>
#include <QToolButton>
#include <QWidget>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/customizer/ParameterObject.h"
#include "gui/Preferences.h"
#include "gui/parameter/GroupWidget.h"
#include "gui/parameter/ParameterCheckBox.h"
#include "gui/parameter/ParameterComboBox.h"
#include "gui/parameter/ParameterSlider.h"
#include "gui/parameter/ParameterSpinBox.h"
#include "gui/parameter/ParameterText.h"
#include "gui/parameter/ParameterVector.h"
#include "gui/parameter/ParameterVirtualWidget.h"

namespace {

void appendOptional(std::ostringstream& out, const boost::optional<double>& value)
{
  if (value) {
    out << *value;
  }
  out << '\x1f';
}

// Length-prefix anything that originated from the user. Delimiters alone are not enough:
// a name, description, group, string default or enum key containing \x1f or \x1e could
// forge a field boundary, letting two genuinely different parameter sets produce the same
// signature. setParameters() would then take the fast path and skip a rebuild it actually
// needed, leaving the Customizer out of sync with the declarations.
void appendString(std::ostringstream& out, const std::string& value)
{
  out << value.size() << ':' << value;
}

// Identity of the Customizer's *shape*: the parameter list plus everything that decides
// which widget each parameter gets and how that widget is configured. Current values are
// excluded on purpose - they are owned by the parameter set and restored by loadSet(), so
// editing a value must not count as a shape change.
std::string parameterShapeSignature(const ParameterObjects& parameters)
{
  std::ostringstream out;
  out.precision(17);
  for (const auto& parameter : parameters) {
    appendString(out, parameter->name());
    appendString(out, parameter->description());
    appendString(out, parameter->group());
    out << static_cast<int>(parameter->type()) << '\x1f';
    switch (parameter->type()) {
    case ParameterObject::ParameterType::Bool:
      out << static_cast<const BoolParameter *>(parameter.get())->defaultValue;
      break;
    case ParameterObject::ParameterType::String: {
      const auto *p = static_cast<const StringParameter *>(parameter.get());
      appendString(out, p->defaultValue);
      out << '\x1f';
      if (p->maximumSize) {
        out << *p->maximumSize;
      }
      break;
    }
    case ParameterObject::ParameterType::Number: {
      const auto *p = static_cast<const NumberParameter *>(parameter.get());
      out << p->defaultValue << '\x1f';
      appendOptional(out, p->minimum);
      appendOptional(out, p->maximum);
      appendOptional(out, p->step);
      break;
    }
    case ParameterObject::ParameterType::Vector: {
      const auto *p = static_cast<const VectorParameter *>(parameter.get());
      for (const double element : p->defaultValue) {
        out << element << ',';
      }
      out << '\x1f';
      appendOptional(out, p->minimum);
      appendOptional(out, p->maximum);
      appendOptional(out, p->step);
      break;
    }
    case ParameterObject::ParameterType::Enum: {
      const auto *p = static_cast<const EnumParameter *>(parameter.get());
      out << p->defaultValueIndex << '\x1f';
      for (const auto& item : p->items) {
        appendString(out, item.key);
        if (std::holds_alternative<double>(item.value)) {
          out << 'd' << std::get<double>(item.value);
        } else {
          out << 's';
          appendString(out, std::get<std::string>(item.value));
        }
        out << ',';
      }
      break;
    }
    }
    out << '\x1e';
  }
  return out.str();
}

}  // namespace

ParameterWidget::ParameterWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  scrollAreaWidgetContents->layout()->setAlignment(Qt::AlignTop);

  autoPreviewTimer.setInterval(1000);
  autoPreviewTimer.setSingleShot(true);

  connect(&autoPreviewTimer, &QTimer::timeout, this, &ParameterWidget::emitParametersChanged);
  // connect(comboBoxPreset, &QComboBox::editTextChanged, this, &ParameterWidget::onSetNameChanged);

  auto *customizer_menu = new QMenu(this);
  parameterMenuButton->setMenu(customizer_menu);

  QAction *collapseAction = customizer_menu->addAction(_("Collapse All"));
  connect(collapseAction, &QAction::triggered, this, &ParameterWidget::onCollapseAll);

  QAction *expandAction = customizer_menu->addAction(_("Expand All"));
  connect(expandAction, &QAction::triggered, this, &ParameterWidget::onExpandAll);

  QString fontfamily = GlobalPreferences::inst()->getValue("advanced/customizerFontFamily").toString();
  uint fontsize = GlobalPreferences::inst()->getValue("advanced/customizerFontSize").toUInt();
  setFontFamilySize(fontfamily, fontsize);

  connect(GlobalPreferences::inst(), &Preferences::customizerFontChanged, this,
          &ParameterWidget::setFontFamilySize);
}

void ParameterWidget::resetForNewDocument()
{
  pendingSessionState.clear();
  invalidJsonFile.clear();
  source.clear();
  parameterShape.clear();
  savedGroupStates.clear();
  savedScrollValue = 0;
  setModified(false);

  if (comboBoxPreset->isEditable() && comboBoxPreset->lineEdit()) {
    QObject::disconnect(comboBoxPreset->lineEdit(), &QLineEdit::textEdited, this,
                        &ParameterWidget::onSetNameChanged);
  }

  ParameterObjects oldParameters = std::move(this->parameters);
  widgets.clear();
  QLayout *layout = this->scrollAreaWidgetContents->layout();
  while (layout->count() > 0) {
    QLayoutItem *child = layout->takeAt(0);
    if (child->widget()) {
      child->widget()->deleteLater();
    }
    delete child;
  }
  QCoreApplication::processEvents();

  sets.clear();

  comboBoxPreset->clear();
  comboBoxPreset->addItem(_("<design default>"));
  comboBoxPreset->setCurrentIndex(0);
  updateSetEditability();
}

// Call after resetForNewDocument() if setParameters() may have run already; otherwise only before the
// first setParameters() (e.g. new tab from createTab).
void ParameterWidget::readFile(const QString& scadFile)
{
  assert(sets.empty());
  assert(parameters.empty());
  assert(widgets.empty());

  QString jsonFile = getJsonFile(scadFile);
  if (!std::filesystem::exists(jsonFile.toStdString()) || this->sets.readFile(jsonFile.toStdString())) {
    this->invalidJsonFile = QString();
  } else {
    this->invalidJsonFile = jsonFile;
  }

  for (const auto& set : this->sets) {
    comboBoxPreset->addItem(QString::fromStdString(set.name()));
  }
}

// Write the json file if the parameter sets are not empty.
// This prevents creating unnecessary json files.
void ParameterWidget::saveFile(const QString& scadFile)
{
  if (sets.empty()) {
    return;
  }

  QString jsonFile = getJsonFile(scadFile);
  if (jsonFile == this->invalidJsonFile) {
    QMessageBox msgBox;
    msgBox.setWindowTitle(_("Saving presets"));
    msgBox.setText(QString(_("%1 was found, but was unreadable. Do you want to overwrite %1?"))
                     .arg(this->invalidJsonFile));
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    if (msgBox.exec() == QMessageBox::Cancel) {
      return;
    }
  }

  cleanSets();
  sets.writeFile(jsonFile.toStdString());
}

void ParameterWidget::saveBackupFile(const QString& scadFile)
{
  if (sets.empty()) {
    return;
  }

  sets.writeFile(getJsonFile(scadFile).toStdString());
}

void ParameterWidget::setParameters(const SourceFile *sourceFile, const std::string& source)
{
  this->source = source;

  // Build the incoming set first so its shape can be compared with what is on screen. This
  // is a pure AST walk; no widget is touched and nothing existing is modified.
  ParameterObjects newParameters = ParameterObjects::fromSourceFile(sourceFile);
  const std::string newShape = parameterShapeSignature(newParameters);

  // Fast path: same shape as the widgets already built. This is the common case, because a
  // re-render triggered from the Customizer re-declares exactly the same parameters. Keep
  // the widgets so scroll offset, keyboard focus and group expansion survive, and let
  // loadSet() below push the current set's values into them. The existing ParameterObjects
  // have to be kept too, since every widget holds a raw pointer to its own; the incoming
  // ones carry no information the kept ones lack, the shape being identical.
  //
  // Programmatic setValue() is safe here: it is the same call loadSet() already makes when
  // switching presets. The spin box widgets guard re-entry with lastSent/lastApplied, and
  // the check box and combo box listen on clicked/activated, which only user input emits.
  if (!widgets.empty() && pendingSessionState.isEmpty() && newShape == this->parameterShape) {
    loadSet(comboBoxPreset->currentIndex());
    return;
  }

  // Shape really changed, so a rebuild is unavoidable. Snapshot the UI state now:
  // rebuildWidgets() cannot read it itself, the group widgets being gone by the time it runs.
  captureUiState();

  // Store old parameters temporarily - they must stay alive until widgets using them
  // are fully deleted. Using deleteLater() defers widget deletion until the event
  // loop, but if the parameters are destroyed first, the widgets will access freed
  // memory when Qt delivers pending events (like mouse release on a slider).
  ParameterObjects oldParameters = std::move(this->parameters);

  // Schedule widgets for deletion. Use deleteLater() because Qt may have captured
  // pointers to widgets during mouse press events. Direct deletion would leave
  // Qt with dangling pointers when it tries to deliver the mouse release event.
  widgets.clear();
  QLayout *layout = this->scrollAreaWidgetContents->layout();
  while (layout->count() > 0) {
    QLayoutItem *child = layout->takeAt(0);
    if (child->widget()) {
      child->widget()->deleteLater();
    }
    delete child;
  }

  // Process pending events to ensure widgets are deleted while old parameters
  // are still valid. This handles the case where a slider is being dragged
  // when F5 is pressed - the release event will be processed here.
  QCoreApplication::processEvents();

  // Now it's safe to load new parameters - old widgets have been deleted
  // and oldParameters will be destroyed when this function returns.
  this->parameters = std::move(newParameters);
  this->parameterShape = newShape;

  if (!pendingSessionState.isEmpty()) {
    const QJsonDocument doc = QJsonDocument::fromJson(pendingSessionState);
    pendingSessionState.clear();
    if (doc.isObject()) {
      const QJsonObject root = doc.object();
      const int currentIndex = root.value(QStringLiteral("currentIndex")).toInt(0);
      const QString setsJson = root.value(QStringLiteral("setsJson")).toString();
      const QByteArray setsUtf8 = setsJson.toUtf8();
      std::string setsStr(reinterpret_cast<const char *>(setsUtf8.constData()),
                          static_cast<size_t>(setsUtf8.size()));
      if (!setsStr.empty() && this->sets.readFromString(setsStr)) {
        comboBoxPreset->clear();
        comboBoxPreset->addItem(_("<design default>"));
        for (const auto& set : this->sets) {
          comboBoxPreset->addItem(QString::fromStdString(set.name()));
        }
        const int idx = std::max(0, std::min(currentIndex, comboBoxPreset->count() - 1));
        comboBoxPreset->setCurrentIndex(idx);
      }
    }
  }

  rebuildWidgets();
  loadSet(comboBoxPreset->currentIndex());
}

QByteArray ParameterWidget::getSessionState()
{
  const int idx = comboBoxPreset->currentIndex();
  if (idx > 0 && static_cast<size_t>(idx) <= sets.size()) {
    for (const auto& param : parameters) {
      sets[idx - 1][param->name()] = param->exportValue();
    }
  }
  std::string setsStr;
  cleanSets();
  sets.writeToString(setsStr);
  QJsonObject root;
  root.insert(QStringLiteral("currentIndex"), idx);
  root.insert(QStringLiteral("setsJson"),
              QString::fromUtf8(setsStr.data(), static_cast<int>(setsStr.size())));
  return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void ParameterWidget::setSessionState(const QByteArray& state)
{
  pendingSessionState = state;
}

void ParameterWidget::applyParameters(SourceFile *sourceFile)
{
  this->parameters.apply(sourceFile);
}

bool ParameterWidget::childHasFocus()
{
  if (this->hasFocus()) {
    return true;
  }
  auto children = this->findChildren<QWidget *>();
  for (auto child : children) {
    if (child->hasFocus()) {
      return true;
    }
  }
  return false;
}

void ParameterWidget::setModified(bool modified)
{
  if (this->modified != modified) {
    this->modified = modified;
    emit modificationChanged();
  }
}

void ParameterWidget::emitParametersChanged()
{
  for (const auto& kvp : widgets) {
    for (ParameterVirtualWidget *widget : kvp.second) {
      widget->valueApplied();
    }
  }
  emit parametersChanged();
}

void ParameterWidget::autoPreview(bool immediate)
{
  autoPreviewTimer.stop();
  if (checkBoxAutoPreview->isChecked()) {
    if (immediate) {
      emitParametersChanged();
    } else {
      autoPreviewTimer.start();
    }
  }
}

void ParameterWidget::on_checkBoxAutoPreview_toggled(bool)
{
  autoPreview(true);
}

void ParameterWidget::on_comboBoxDetails_currentIndexChanged(int)
{
  rebuildWidgets();
}

void ParameterWidget::on_comboBoxPreset_activated(int index)
{
  loadSet(index);
  autoPreview(true);
}

void ParameterWidget::onSetNameChanged()
{
  assert(static_cast<size_t>(comboBoxPreset->count()) == sets.size() + 1);
  comboBoxPreset->setItemText(comboBoxPreset->currentIndex(), comboBoxPreset->lineEdit()->text());
  sets[comboBoxPreset->currentIndex() - 1].setName(comboBoxPreset->currentText().toStdString());
  setModified();
}

void ParameterWidget::on_addButton_clicked()
{
  bool ok = true;
  QString result =
    QInputDialog::getText(this, _("Create new set of parameter"), _("Enter name of the parameter set"),
                          QLineEdit::Normal, "", &ok);

  if (ok) {
    createSet(result.trimmed());
  }
  setModified();
}

void ParameterWidget::on_deleteButton_clicked()
{
  int index = comboBoxPreset->currentIndex();
  assert(index > 0);
  int newIndex;
  if (index + 1 == comboBoxPreset->count()) {
    newIndex = index - 1;
  } else {
    newIndex = index + 1;
  }
  comboBoxPreset->setCurrentIndex(newIndex);
  loadSet(newIndex);

  comboBoxPreset->removeItem(index);
  sets.erase(sets.begin() + (index - 1));
  setModified();
  autoPreview(true);
}

void ParameterWidget::onCollapseAll()
{
  for (GroupWidget *groupWidget : this->findChildren<GroupWidget *>()) {
    groupWidget->setExpanded(false);
  }
}

void ParameterWidget::onExpandAll()
{
  for (GroupWidget *groupWidget : this->findChildren<GroupWidget *>()) {
    groupWidget->setExpanded(true);
  }
}

void ParameterWidget::parameterModified(bool immediate)
{
  auto *widget = (ParameterVirtualWidget *)sender();
  ParameterObject *parameter = widget->getParameter();

  // When attempting to modify the design default, create a new set to edit.
  if (comboBoxPreset->currentIndex() == 0) {
    std::set<std::string> setNames;
    for (const auto& set : this->sets) {
      setNames.insert(set.name());
    }

    QString name;
    for (int i = 1;; i++) {
      name = _("New set ") + QString::number(i);
      if (setNames.count(name.toStdString()) == 0) {
        break;
      }
    }
    createSet(name);
  }

  size_t setIndex = comboBoxPreset->currentIndex() - 1;
  assert(setIndex < sets.size());
  sets[setIndex][parameter->name()] = parameter->exportValue();

  assert(widgets.count(parameter) == 1);
  for (ParameterVirtualWidget *otherWidget : widgets[parameter]) {
    if (otherWidget != widget) {
      otherWidget->setValue();
    }
  }

  setModified();
  autoPreview(immediate);
}

void ParameterWidget::loadSet(size_t index)
{
  assert(index <= sets.size());
  if (index == 0) {
    parameters.reset();
  } else {
    parameters.importValues(sets[index - 1]);
  }

  updateSetEditability();

  for (const auto& pair : widgets) {
    for (ParameterVirtualWidget *widget : pair.second) {
      widget->setValue();
    }
  }
}

void ParameterWidget::createSet(const QString& name)
{
  sets.push_back(parameters.exportValues(name.toStdString()));
  comboBoxPreset->addItem(name);
  comboBoxPreset->setCurrentIndex(comboBoxPreset->count() - 1);
  updateSetEditability();
}

void ParameterWidget::updateSetEditability()
{
  if (comboBoxPreset->currentIndex() == 0) {
    comboBoxPreset->setEditable(false);
    deleteButton->setEnabled(false);
  } else {
    if (!comboBoxPreset->isEditable()) {
      comboBoxPreset->setEditable(true);
      connect(comboBoxPreset->lineEdit(), &QLineEdit::textEdited, this,
              &ParameterWidget::onSetNameChanged);
    }
    deleteButton->setEnabled(true);
  }
}

// Snapshot the UI state a rebuild would otherwise discard. Does nothing when no group
// widget is alive to read, so a snapshot taken before a teardown is not clobbered by the
// captureUiState() call at the top of rebuildWidgets().
void ParameterWidget::captureUiState()
{
  const auto groupWidgets = this->findChildren<GroupWidget *>();
  if (groupWidgets.isEmpty()) {
    return;
  }
  savedGroupStates.clear();
  for (GroupWidget *groupWidget : groupWidgets) {
    savedGroupStates[groupWidget->title()] = groupWidget->isExpanded();
  }
  savedScrollValue = scrollArea->verticalScrollBar()->value();
}

void ParameterWidget::rebuildWidgets()
{
  // Reads current state when the widgets are still up (the detail-level combo box path),
  // and otherwise falls through to what setParameters() captured before its teardown.
  captureUiState();

  widgets.clear();
  QLayout *layout = this->scrollAreaWidgetContents->layout();
  while (layout->count() > 0) {
    QLayoutItem *child = layout->takeAt(0);
    delete child->widget();
    delete child;
  }

  auto descriptionStyle = static_cast<DescriptionStyle>(comboBoxDetails->currentIndex());
  std::vector<ParameterGroup> parameterGroups = getParameterGroups();
  for (const auto& group : parameterGroups) {
    auto *groupWidget = new GroupWidget(group.name);
    for (ParameterObject *parameter : group.parameters) {
      ParameterVirtualWidget *parameterWidget = createParameterWidget(parameter, descriptionStyle);
      connect(parameterWidget, &ParameterVirtualWidget::changed, this,
              &ParameterWidget::parameterModified);
      if (!widgets.count(parameter)) {
        widgets[parameter] = {};
      }
      widgets[parameter].push_back(parameterWidget);
      groupWidget->addWidget(parameterWidget);
    }
    auto it = savedGroupStates.find(group.name);
    groupWidget->setExpanded(it == savedGroupStates.end() || it->second);
    layout->addWidget(groupWidget);
  }

  // The new contents have not been laid out yet, so the scroll bar's range is still stale
  // and setValue() would clamp to 0. Defer until the layout has settled.
  if (savedScrollValue > 0) {
    const int scrollValue = savedScrollValue;
    QTimer::singleShot(
      0, this, [this, scrollValue]() { scrollArea->verticalScrollBar()->setValue(scrollValue); });
  }
  savedScrollValue = 0;
}

std::vector<ParameterWidget::ParameterGroup> ParameterWidget::getParameterGroups()
{
  std::vector<ParameterWidget::ParameterGroup> output;
  std::map<std::string, size_t> groupIndices;
  std::vector<ParameterObject *> globalParameters;

  for (const std::unique_ptr<ParameterObject>& parameter : parameters) {
    std::string group = parameter->group();
    if (group == "Global") {
      globalParameters.push_back(parameter.get());
    } else if (group == "Hidden") {
      continue;
    } else {
      if (!groupIndices.count(group)) {
        groupIndices[group] = output.size();
        output.push_back({QString::fromStdString(group)});
      }
      output[groupIndices[group]].parameters.push_back(parameter.get());
    }
  }

  if (output.size() == 0 && globalParameters.size() > 0) {
    ParameterGroup global;
    global.name = "Global";
    global.parameters = std::move(globalParameters);
    output.push_back(std::move(global));
  } else {
    for (auto& group : output) {
      group.parameters.insert(group.parameters.end(), globalParameters.begin(), globalParameters.end());
    }
  }

  return output;
}

ParameterVirtualWidget *ParameterWidget::createParameterWidget(ParameterObject *parameter,
                                                               DescriptionStyle descriptionStyle)
{
  if (parameter->type() == ParameterObject::ParameterType::Bool) {
    return new ParameterCheckBox(this, static_cast<BoolParameter *>(parameter), descriptionStyle);
  } else if (parameter->type() == ParameterObject::ParameterType::String) {
    return new ParameterText(this, static_cast<StringParameter *>(parameter), descriptionStyle);
  } else if (parameter->type() == ParameterObject::ParameterType::Number) {
    auto *numberParameter = static_cast<NumberParameter *>(parameter);
    if (numberParameter->minimum && numberParameter->maximum) {
      return new ParameterSlider(this, numberParameter, descriptionStyle);
    } else {
      return new ParameterSpinBox(this, numberParameter, descriptionStyle);
    }
  } else if (parameter->type() == ParameterObject::ParameterType::Vector) {
    return new ParameterVector(this, static_cast<VectorParameter *>(parameter), descriptionStyle);
  } else if (parameter->type() == ParameterObject::ParameterType::Custom) {
    return new ParameterPyQtWidget(this, static_cast<CustomParameter *>(parameter), descriptionStyle);
  } else if (parameter->type() == ParameterObject::ParameterType::Enum) {
    return new ParameterComboBox(this, static_cast<EnumParameter *>(parameter), descriptionStyle);
  } else {
    assert(false);
    throw std::runtime_error("Unsupported parameter widget type");
  }
}

QString ParameterWidget::getJsonFile(const QString& scadFile)
{
  std::filesystem::path p = scadFile.toStdString();
  return QString::fromStdString(p.replace_extension(".json").string());
}

// Remove set values that do not correspond to a parameter,
// or that cannot be parsed as such.
void ParameterWidget::cleanSets()
{
  std::map<std::string, ParameterObject *> namedParameters;
  for (const auto& parameter : parameters) {
    namedParameters[parameter->name()] = parameter.get();
  }

  for (ParameterSet& set : sets) {
    for (auto it = set.begin(); it != set.end();) {
      if (!namedParameters.count(it->first)) {
        it = set.erase(it);
      } else {
        if (namedParameters[it->first]->importValue(it->second, false)) {
          ++it;
        } else {
          it = set.erase(it);
        }
      }
    }
  }
}

void ParameterWidget::setFontFamilySize(const QString& fontFamily, uint fontSize)
{
  scrollArea->setFont(QFont(fontFamily, fontSize));
}
