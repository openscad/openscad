#pragma once

#include <QMainWindow>
#include <QSettings>

#include "qtgettext.h"
#include "ui_Preferences.h"
#include "settings.h"

class Preferences : public QMainWindow, public Ui::Preferences
{
	Q_OBJECT;

public:
	~Preferences();

	static void create(QStringList colorSchemes);
	static Preferences *inst();

	QVariant getValue(const QString &key) const;
	void init();
	void apply() const;
	void fireEditorConfigChanged() const;

public slots:
	void actionTriggered(class QAction *);
	void featuresCheckBoxToggled(bool);
	void on_colorSchemeChooser_itemSelectionChanged();
	void on_fontChooser_activated(const QString &);
	void on_fontSize_currentIndexChanged(const QString &);
	void on_syntaxHighlight_activated(const QString &);
	void on_openCSGWarningBox_toggled(bool);
	void on_enableOpenCSGBox_toggled(bool);
	void on_cgalCacheSizeEdit_textChanged(const QString &);
	void on_polysetCacheSizeEdit_textChanged(const QString &);
	void on_opencsgLimitEdit_textChanged(const QString &);
	void on_forceGoldfeatherBox_toggled(bool);
	void on_mouseWheelZoomBox_toggled(bool);
	void on_localizationCheckBox_toggled(bool);
	void on_updateCheckBox_toggled(bool);
	void on_snapshotCheckBox_toggled(bool);
	void on_mdiCheckBox_toggled(bool);
	void on_reorderCheckBox_toggled(bool);
	void on_undockCheckBox_toggled(bool);
	void on_checkNowButton_clicked();
	void on_launcherBox_toggled(bool);
	void on_editorType_currentIndexChanged(const QString &);

	void on_checkBoxShowWarningsIn3dView_toggled(bool);
	//
	// editor settings
	//

	// Indentation
	void on_checkBoxAutoIndent_toggled(bool);
	void on_checkBoxBackspaceUnindents_toggled(bool);
	void on_comboBoxIndentUsing_activated(int);
	void on_spinBoxIndentationWidth_valueChanged(int);
	void on_spinBoxTabWidth_valueChanged(int);
	void on_comboBoxTabKeyFunction_activated(int);
	void on_comboBoxShowWhitespace_activated(int);
	void on_spinBoxShowWhitespaceSize_valueChanged(int);

	// Line wrap
	void on_comboBoxLineWrap_activated(int);
	void on_comboBoxLineWrapIndentationStyle_activated(int);
	void on_spinBoxLineWrapIndentationIndent_valueChanged(int);
	void on_comboBoxLineWrapVisualizationStart_activated(int);
	void on_comboBoxLineWrapVisualizationEnd_activated(int);

	// Display
	void on_checkBoxHighlightCurrentLine_toggled(bool);
	void on_checkBoxEnableBraceMatching_toggled(bool);
	void on_checkBoxEnableLineNumbers_toggled(bool);

signals:
	void requestRedraw() const;
	void updateMdiMode(bool mdi) const;
	void updateUndockMode(bool undockMode) const;
	void updateReorderMode(bool undockMode) const;
	void fontChanged(const QString &family, uint size) const;
	void colorSchemeChanged(const QString &scheme) const;
	void openCSGSettingsChanged() const;
	void syntaxHighlightChanged(const QString &s) const;
	void editorTypeChanged(const QString &type);
	void editorConfigChanged() const;
	void ExperimentalChanged() const;

private:
	Preferences(QWidget *parent = nullptr);
	void keyPressEvent(QKeyEvent *e);
	void updateGUI();
	void removeDefaultSettings();
	void setupFeaturesPage();
	void writeSettings();
	void addPrefPage(QActionGroup *group, QAction *action, QWidget *widget);

	/** Initialize combobox list values from the settings range values */
	void initComboBox(QComboBox *comboBox, const Settings::SettingsEntry &entry);
	/** Initialize spinbox min/max values from the settings range values */
	void initSpinBox(QSpinBox *spinBox, const Settings::SettingsEntry &entry);
	/** Update combobox from current settings */
	void updateComboBox(QComboBox *comboBox, const Settings::SettingsEntry &entry);
	/** Set value from combobox to settings */
	void applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntry &entry);

	QSettings::SettingsMap defaultmap;
	QHash<const QAction *, QWidget *> prefPages;

	static Preferences *instance;
	static const char *featurePropertyName;
};
