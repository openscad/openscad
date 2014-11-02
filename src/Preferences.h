#pragma once

#include <QMainWindow>
#include <QSettings>
#include "ui_Preferences.h"

class Preferences : public QMainWindow, public Ui::Preferences
{
	Q_OBJECT;

public:
	~Preferences();
        
        static void create(QWidget *parent, QStringList colorSchemes);
	static Preferences *inst();

	QVariant getValue(const QString &key) const;
        void init();
	void apply() const;

public slots:
	void actionTriggered(class QAction *);
	void featuresCheckBoxToggled(bool);
	void on_colorSchemeChooser_itemSelectionChanged();
	void on_fontChooser_activated(const QString &);
	void on_fontSize_editTextChanged(const QString &);
	void on_syntaxHighlight_activated(const QString &);
	void on_openCSGWarningBox_toggled(bool);
	void on_enableOpenCSGBox_toggled(bool);
	void on_cgalCacheSizeEdit_textChanged(const QString &);
	void on_polysetCacheSizeEdit_textChanged(const QString &);
	void on_opencsgLimitEdit_textChanged(const QString &);
	void on_forceGoldfeatherBox_toggled(bool);
	void on_mouseWheelZoomBox_toggled(bool);
	void on_updateCheckBox_toggled(bool);
	void on_snapshotCheckBox_toggled(bool);
	void on_mdiCheckBox_toggled(bool);
	void on_undockCheckBox_toggled(bool);
	void on_checkNowButton_clicked();
	void on_launcherBox_toggled(bool);
	void on_editorType_editTextChanged(const QString &);

signals:
	void requestRedraw() const;
	void updateMdiMode(bool mdi) const;
	void updateUndockMode(bool mdi) const;
	void fontChanged(const QString &family, uint size) const;
	void colorSchemeChanged(const QString &scheme) const;
	void openCSGSettingsChanged() const;
	void syntaxHighlightChanged(const QString &s) const;
	void editorTypeChanged(const QString &type);

private:
	Preferences(QWidget *parent = NULL);
	void keyPressEvent(QKeyEvent *e);
	void updateGUI();
	void removeDefaultSettings();
	void setupFeaturesPage();
	void addPrefPage(QActionGroup *group, QAction *action, QWidget *widget);

	QSettings::SettingsMap defaultmap;
	QHash<const QAction *, QWidget *> prefPages;

	static Preferences *instance;
	static const char *featurePropertyName;
};
