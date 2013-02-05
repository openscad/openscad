#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include <QMainWindow>
#include <QSettings>
#include "ui_Preferences.h"
#include "rendersettings.h"
#include "linalg.h"
#include <map>

class Preferences : public QMainWindow, public Ui::Preferences
{
	Q_OBJECT;

public:
	~Preferences();
	static Preferences *inst() { if (!instance) instance = new Preferences(); return instance; }

	QVariant getValue(const QString &key) const;
	void apply() const;

public slots:
	void actionTriggered(class QAction *);
	void on_colorSchemeChooser_itemSelectionChanged();
	void on_fontChooser_activated(const QString &);
	void on_fontSize_editTextChanged(const QString &);
	void on_openCSGWarningBox_toggled(bool);
	void on_enableOpenCSGBox_toggled(bool);
	void on_cgalCacheSizeEdit_textChanged(const QString &);
	void on_polysetCacheSizeEdit_textChanged(const QString &);
	void on_opencsgLimitEdit_textChanged(const QString &);
	void on_forceGoldfeatherBox_toggled(bool);
	void on_updateCheckBox_toggled(bool);
	void on_snapshotCheckBox_toggled(bool);
	void on_checkNowButton_clicked();

signals:
	void requestRedraw() const;
	void fontChanged(const QString &family, uint size) const;
	void openCSGSettingsChanged() const;

private:
	Preferences(QWidget *parent = NULL);
	void keyPressEvent(QKeyEvent *e);
	void updateGUI();
	void removeDefaultSettings();

	QSettings::SettingsMap defaultmap;
	QHash<QString, std::map<RenderSettings::RenderColor, Color4f> > colorschemes;

	static Preferences *instance;
};

#endif
