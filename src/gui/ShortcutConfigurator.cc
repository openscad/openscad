#include "ShortcutConfigurator.h"
#include <QFile>
#include <QVariant>
#include <QAbstractItemModel>
#include <QHeaderView>
#include <QJsonObject>
#include <QMessageBox>
#include <QKeyEvent>
#include <QEvent>
#include "platform/PlatformUtils.h"

ShortcutConfigurator::ShortcutConfigurator(QWidget *parent) : QWidget(parent)
{
  const int numRows = 0;
  const int numColumns = 3;
  setupUi(this);
  configFileLoc = PlatformUtils::userConfigPath() + "/shortcuts.json";
  model = new QStandardItemModel(numRows, numColumns, this);
  proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setSourceModel(model);
  proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
  shortcutCatcher = nullptr;

  connect(shortcutsTable, &QTableView::clicked, this, &ShortcutConfigurator::onTableCellClicked);
}

ShortcutConfigurator::~ShortcutConfigurator()
{
  if (shortcutCatcher) {
    delete shortcutCatcher;
  }
}

bool ShortcutConfigurator::handleKeyPressEvent(const QKeyEvent *keyEvent)
{
  const auto key = static_cast<Qt::Key>(keyEvent->key());
  if (key == Qt::Key_unknown) {
    return false;
  }

  // The user has clicked just and only one of the special keys Ctrl, Shift, Alt, Meta.
  if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_AltGr ||
      key == Qt::Key_Meta) {
    return false;
  }

  // check for a combination of user clicks
  const auto modifiers = QApplication::keyboardModifiers();
  const auto keyText = keyEvent->text();

  int keyWithModifiers = key;
  if (modifiers & Qt::ShiftModifier) keyWithModifiers += Qt::SHIFT;
  if (modifiers & Qt::ControlModifier) keyWithModifiers += Qt::CTRL;
  if (modifiers & Qt::AltModifier) keyWithModifiers += Qt::ALT;
  if (modifiers & Qt::MetaModifier) keyWithModifiers += Qt::META;

  pressedKeySequence = QKeySequence(keyWithModifiers);
  if (pressedKeySequence.isEmpty()) {
    return false;
  }

  const auto info =
    QString(_("You Pressed: %1")).arg(pressedKeySequence.toString(QKeySequence::NativeText));
  shortcutCatcher->setInformativeText(info);
  return true;
}

bool ShortcutConfigurator::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    const auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
    if (keyEvent && handleKeyPressEvent(keyEvent)) return true;
  }

  return QObject::eventFilter(obj, event);
}

void ShortcutConfigurator::collectDefaults(const QList<QAction *>& allActions)
{
  for (auto& action : allActions) {
    defaultShortcuts.insert(action, action->shortcuts());
    QString actionName = action->objectName();
    shortcutsMap.insert(actionName, action);
    actionsList.push_back(action);
    if (!actionsName.contains(actionName)) {
      actionsName.push_back(actionName);
    }
    const QList<QKeySequence> shortcutsList = action->shortcuts();
    for (auto& shortcutSeq : shortcutsList) {
      QString shortcut = shortcutSeq.toString(QKeySequence::NativeText);
      if (!shortcutOccupied.contains(shortcut)) shortcutOccupied.insert(shortcut, actionName);
    }
  }
}

void ShortcutConfigurator::createModel(const QList<QAction *>& actions)
{
  QList<QString> alreadyInGUI;
  int row = 0;
  model->removeRows(0, model->rowCount());
  model->removeColumns(0, 3);
  QList<QString> labels = QList<QString>() << QString(_("Action")) << QString(_("Shortcut"))
                                           << QString(_("Alternative-1"));
  model->setHorizontalHeaderLabels(labels);
  for (auto& action : actions) {
    QString actionName = action->objectName();
    if (!actionName.isEmpty()) {
      if (alreadyInGUI.contains(actionName)) continue;
      else alreadyInGUI.push_back(actionName);
      auto *actionNameItem = new QStandardItem(actionName);
      model->setRowCount(row + 1);
      model->setItem(row, 0, actionNameItem);
      actionNameItem->setFlags(actionNameItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);

      const QList<QKeySequence> shortcutsList = action->shortcuts();
      int index = 1;
      for (auto& shortcutSeq : shortcutsList) {
        const QString shortcut = shortcutSeq.toString(QKeySequence::NativeText);
        auto *shortcutItem = new QStandardItem(shortcut);
        if (index > 2) {
          model->setColumnCount(index + 1);
          QString label = QString(_("Alternative-%1")).arg(index - 1);
          labels.push_back(label);
          model->setHorizontalHeaderLabels(labels);
        }
        model->setItem(row, index, shortcutItem);
        index++;
      }
      row++;
    }
  }
}

void ShortcutConfigurator::initGUI(const QList<QAction *>& allActions)
{
  shortcutsTable->verticalHeader()->hide();
  shortcutsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  shortcutsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
  shortcutsTable->setSelectionMode(QAbstractItemView::SingleSelection);
  shortcutsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  createModel(allActions);
  shortcutsTable->setModel(proxyModel);
}

QJsonObject ShortcutConfigurator::readConfigFile()
{
  QFile jsonFile(QString::fromStdString(configFileLoc.c_str()));

  // check if a User-Defined Shortcuts file exists or not
  if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  QByteArray jsonData = jsonFile.readAll();
  QJsonDocument doc = QJsonDocument::fromJson(jsonData);
  QJsonObject object = doc.object();
  return object;
}

bool ShortcutConfigurator::writeToConfigFile(const QJsonObject& object)
{
  QFile jsonFile(QString::fromStdString(configFileLoc.c_str()));

  if (!jsonFile.open(QIODevice::WriteOnly)) {
    return false;
  }

  QJsonDocument doc(object);
  jsonFile.write(doc.toJson());
  jsonFile.close();
  return true;
}

void ShortcutConfigurator::applyConfigFile(const QList<QAction *>& actions)
{
  QJsonObject object = readConfigFile();
  for (auto& action : actions) {
    QString actionName = action->objectName();
    if (!actionName.isEmpty()) {
      QList<QKeySequence> shortcutsListFinal;

      // check if the actions new shortcut exists in the file or not
      // This would allow users to remove any default shortcut, by just leaving the key's value for an
      // action name as empty.
      QJsonObject::const_iterator i = object.find(actionName);
      if (i != object.end() && i.key() == actionName) {
        QJsonValue val = object.value(actionName);
        // check if it is an array or not
        if (!val.isArray()) {
          QString singleShortcut = val.toString().trimmed();
          if (shortcutOccupied.contains(singleShortcut) &&
              shortcutOccupied[singleShortcut] != actionName) {
            raiseError(QString::fromStdString(configFileLoc.c_str()) +
                       QString(":\n" + actionName + " shortcut \"" + singleShortcut +
                               "\" conflicts with " + shortcutOccupied[singleShortcut]));
            continue;
          } else if (!singleShortcut.isEmpty()) {
            shortcutOccupied.insert(singleShortcut, actionName);
          }
          action->setShortcut(QKeySequence(singleShortcut));
        } else {
          // create a key sequence
          QJsonArray array = val.toArray();
          for (const auto& v : array) {
            QString shortcut = v.toString();
            if (shortcut.isEmpty()) continue;
            if (defaultShortcuts[action].size() != 0 &&
                !QString::compare(shortcut, "DEFAULT", Qt::CaseInsensitive)) {
              const QString defaultShortcut =
                defaultShortcuts[action][0].toString(QKeySequence::NativeText);
              if (defaultShortcut.isEmpty()) continue;
              shortcutOccupied.insert(defaultShortcut, actionName);
              shortcutsListFinal.append(defaultShortcut);
            } else {
              if (shortcutOccupied.contains(shortcut) && shortcutOccupied[shortcut] != actionName) {
                raiseError(QString::fromStdString(configFileLoc.c_str()) +
                           QString(":\n" + actionName + " shortcut \"" + shortcut +
                                   "\" conflicts with " + shortcutOccupied[shortcut]));
                continue;
              } else if (!shortcut.isEmpty()) {
                shortcutOccupied.insert(shortcut, actionName);
              }
              shortcutsListFinal.append(shortcut);
            }
          }

          action->setShortcuts(shortcutsListFinal);
        }
      }
    }
  }
}

QString ShortcutConfigurator::getData(int row, int col)
{
  return shortcutsTable->model()->index(row, col).data().toString();
}

void ShortcutConfigurator::putData(QModelIndex indexA, const QString& data)
{
  shortcutsTable->model()->setData(indexA, data);
}

void ShortcutConfigurator::raiseError(const QString& errorMsg)
{
  QMessageBox messageBox;
  messageBox.critical(0, "Error", errorMsg);
  messageBox.setFixedSize(500, 200);
}

void ShortcutConfigurator::updateShortcut(QAction *changedAction, const QString& updatedShortcut,
                                          const QModelIndex& index)
{
  QList<QKeySequence> shortcutsListFinal;
  bool singleShortcutUpdate = false;
  if (index.column() == 1) {
    // get the primary (updatedShortcut)
    shortcutsListFinal.append(updatedShortcut);

    QList<QKeySequence> assignedShortcuts = changedAction->shortcuts();

    if (assignedShortcuts.size() != 0) {
      // un-assign the previous primary
      shortcutOccupied.remove(assignedShortcuts[0].toString(QKeySequence::NativeText));

      for (int i = 1; i < assignedShortcuts.size(); i++) {
        shortcutsListFinal.append(assignedShortcuts[i]);
      }
      changedAction->setShortcuts(shortcutsListFinal);
    } else {
      changedAction->setShortcut(QKeySequence(updatedShortcut));
      singleShortcutUpdate = true;
    }
  } else if (index.column() >= 1) {
    QString primaryShortcut = getData(index.row(), 1);
    QList<QKeySequence> assignedShortcuts = changedAction->shortcuts();
    if (assignedShortcuts.size() >= index.column()) {
      // sufficent number of columns, replacement
      for (int i = 0; i < assignedShortcuts.size(); i++) {
        if (i == index.column() - 1) {
          shortcutOccupied.remove(assignedShortcuts[i].toString(QKeySequence::NativeText));
          shortcutsListFinal.append(updatedShortcut);
        } else shortcutsListFinal.append(assignedShortcuts[i]);
      }

    } else {
      // append the new shortcut
      for (const auto& assignedShortcut : assignedShortcuts) {
        shortcutsListFinal.append(assignedShortcut);
      }
      shortcutsListFinal.append(updatedShortcut);
    }
    changedAction->setShortcuts(shortcutsListFinal);
  }

  if (updatedShortcut != QString::fromUtf8(""))
    shortcutOccupied.insert(updatedShortcut, changedAction->objectName());
  putData(index, updatedShortcut);
  // write into the file
  QJsonObject object = readConfigFile();

  if (singleShortcutUpdate) {
    object.insert(changedAction->objectName(), updatedShortcut);
  } else {
    QJsonArray array;
    for (auto& shortcut : shortcutsListFinal) array.append(shortcut.toString());

    QJsonValue newValue = QJsonValue(array);
    object.insert(changedAction->objectName(), newValue);
  }
  writeToConfigFile(object);
}

void ShortcutConfigurator::onTableCellClicked(const QModelIndex& index)
{
  if (index.isValid() && index.column() != 0) {
    // just to avoid opening of multiple dialogs on multiple clicks on cell
    if (shortcutCatcher) delete shortcutCatcher;

    shortcutCatcher = new QMessageBox;
    QString updatedAction = getData(index.row(), 0);

    shortcutCatcher->raise();
    shortcutCatcher->setStyleSheet("QLabel{min-width: 400px;}");
    shortcutCatcher->setInformativeText(QString());
    shortcutCatcher->setWindowTitle(QString(_("Set Shortcut for %1")).arg(updatedAction));
    shortcutCatcher->installEventFilter(this);
    shortcutCatcher->setText(_("Press the Key Sequence"));
    shortcutCatcher->setStandardButtons(QMessageBox::Apply | QMessageBox::Reset | QMessageBox::Cancel);
    shortcutCatcher->setDefaultButton(QMessageBox::Apply);
    shortcutCatcher->setWindowModality(Qt::WindowModal);
    shortcutCatcher->grabKeyboard();
    int ret = shortcutCatcher->exec();
    shortcutCatcher->releaseKeyboard();
    QString updatedShortcut;

    switch (ret) {
    case QMessageBox::Apply:
      updatedShortcut = pressedKeySequence.toString(QKeySequence::NativeText);
      pressedKeySequence = QKeySequence();
      shortcutCatcher->close();
      if (updatedShortcut.isEmpty()) return;
      break;
    case QMessageBox::Reset:
      updatedShortcut = QString();
      shortcutCatcher->close();
      break;
    default: shortcutCatcher->close(); return;
    }

    if (shortcutOccupied.contains(updatedShortcut)) {
      if (shortcutOccupied[updatedShortcut] == updatedAction)
        return;  // ignore if assigned shortcut is assiged to same action again.
      raiseError(QString("Shortcut Already Occupied By: " + shortcutOccupied[updatedShortcut]));
      return;
    }

    QMultiHash<QString, QAction *>::iterator i = shortcutsMap.find(updatedAction);
    while (i != shortcutsMap.end() && i.key() == updatedAction) {
      updateShortcut(i.value(), updatedShortcut, index);
      ++i;
    }
  }
}

void ShortcutConfigurator::on_searchBox_textChanged(const QString& text)
{
  proxyModel->setFilterFixedString(text.trimmed());
}

void ShortcutConfigurator::on_reset_clicked()
{
  QMap<QAction *, QList<QKeySequence>>::iterator i;
  QJsonObject object;

  QMessageBox msgBox;
  msgBox.setText(_("Choosing 'Yes' will restore all the shortcuts!"));
  msgBox.setInformativeText(_("Do you want to continue?"));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::Yes);

  if (msgBox.exec() == QMessageBox::Yes) {
    for (i = defaultShortcuts.begin(); i != defaultShortcuts.end(); ++i) {
      QAction *actionKey = i.key();
      actionKey->setShortcuts(i.value());
    }
    initGUI(actionsList);
    shortcutOccupied.clear();
    writeToConfigFile(object);
  }
}

void ShortcutConfigurator::resetClass()
{
  shortcutsMap.clear();
  shortcutOccupied.clear();
  actionsName.clear();
  defaultShortcuts.clear();
  actionsList.clear();
}
