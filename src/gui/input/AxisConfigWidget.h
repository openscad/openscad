#pragma once

#include "gui/qtgettext.h"
#include <QComboBox>
#include <QWidget>
#include "gui/Settings.h"
#include "ui_AxisConfigWidget.h"
#include "gui/InitConfigurator.h"

class AxisConfigWidget : public QWidget, public Ui::Axis, public InitConfigurator
{
  Q_OBJECT

public:
  AxisConfigWidget(QWidget *parent = nullptr);
  void updateButtonState(int, bool) const;
  void AxesChanged(int nr, double val) const;
  void init();

public slots:
  // Input Driver
  void on_AxisTrim();
  void on_AxisTrimReset();
  void on_comboBoxTranslationX_activated(int val);
  void on_comboBoxTranslationY_activated(int val);
  void on_comboBoxTranslationZ_activated(int val);
  void on_comboBoxTranslationXVPRel_activated(int val);
  void on_comboBoxTranslationYVPRel_activated(int val);
  void on_comboBoxTranslationZVPRel_activated(int val);
  void on_comboBoxRotationX_activated(int val);
  void on_comboBoxRotationY_activated(int val);
  void on_comboBoxRotationZ_activated(int val);
  void on_comboBoxRotationXVPRel_activated(int val);
  void on_comboBoxRotationYVPRel_activated(int val);
  void on_comboBoxRotationZVPRel_activated(int val);
  void on_comboBoxZoom_activated(int val);
  void on_comboBoxZoom2_activated(int val);

  void on_doubleSpinBoxRotateGain_valueChanged(double val);
  void on_doubleSpinBoxRotateVPRelGain_valueChanged(double val);
  void on_doubleSpinBoxTranslationGain_valueChanged(double val);
  void on_doubleSpinBoxTranslationVPRelGain_valueChanged(double val);
  void on_doubleSpinBoxZoomGain_valueChanged(double val);

  void on_doubleSpinBoxDeadzone0_valueChanged(double);
  void on_doubleSpinBoxDeadzone1_valueChanged(double);
  void on_doubleSpinBoxDeadzone2_valueChanged(double);
  void on_doubleSpinBoxDeadzone3_valueChanged(double);
  void on_doubleSpinBoxDeadzone4_valueChanged(double);
  void on_doubleSpinBoxDeadzone5_valueChanged(double);
  void on_doubleSpinBoxDeadzone6_valueChanged(double);
  void on_doubleSpinBoxDeadzone7_valueChanged(double);
  void on_doubleSpinBoxDeadzone8_valueChanged(double);

  void on_doubleSpinBoxTrim0_valueChanged(double);
  void on_doubleSpinBoxTrim1_valueChanged(double);
  void on_doubleSpinBoxTrim2_valueChanged(double);
  void on_doubleSpinBoxTrim3_valueChanged(double);
  void on_doubleSpinBoxTrim4_valueChanged(double);
  void on_doubleSpinBoxTrim5_valueChanged(double);
  void on_doubleSpinBoxTrim6_valueChanged(double);
  void on_doubleSpinBoxTrim7_valueChanged(double);
  void on_doubleSpinBoxTrim8_valueChanged(double);

  void on_checkBoxHIDAPI_toggled(bool);
  void on_checkBoxSpaceNav_toggled(bool);
  void on_checkBoxJoystick_toggled(bool);
  void on_checkBoxQGamepad_toggled(bool);
  void on_checkBoxDBus_toggled(bool);

  void updateStates();

signals:
  void inputMappingChanged() const;
  void inputCalibrationChanged() const;
  void inputGainChanged() const;

private:
  /** Set value from combobox to settings */
  void applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntryEnum& entry);
  void writeSettings();

  bool initialized = false;

  QString NotEnabledDuringBuild = _("This driver was not enabled during build time and is thus not available.");

  QString DBusInputDriverDescription = _("The DBUS driver is not for actual devices but for remote control, Linux only.");
  QString HidApiInputDriverDescription = _("The HIDAPI driver communicates directly with the 3D mice, Windows and macOS.");
  QString SpaceNavInputDriverDescription = _("The SpaceNav driver enables 3D-input-devices using the spacenavd daemon, Linux only.");
  QString JoystickInputDriverDescription = _("The Joystick driver uses the Linux joystick device (fixed to /dev/input/js0), Linux only.");
  QString QGamepadInputDriverDescription = _("The QGAMEPAD driver is for multiplattform Gamepad Support.");

  bool darkModeDetected = false;

  QString ProgressbarStyleLight =
    "QProgressBar::chunk {"
    "background: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #66d9ff,stop: 1 #ccf2ff );"
    "border-radius: 5px;"
    "border: 1px solid #007399;"
    "}";

  QString ProgressbarStyleLightActive =
    "QProgressBar::chunk {"
    "background: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #66ff66,stop: 1 #ccffcc );"
    "border-radius: 5px;"
    "border: 1px solid #007399;"
    "}";

  QString ProgressbarStyleDark =
    "QProgressBar::chunk {"
    "background: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #001a33,stop: 1 #0069cc );"
    "border-radius: 5px;"
    "border: 1px solid #000d1a;"
    "}";

  QString ProgressbarStyleDarkActive =
    "QProgressBar::chunk {"
    "background: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #00331a,stop: 1 #00cc69 );"
    "border-radius: 5px;"
    "border: 1px solid #000d1a;"
    "}";
};
