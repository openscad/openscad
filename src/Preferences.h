#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include <QMainWindow>
#include <QSettings>
#include "ui_Preferences.h"
#include "rendersettings.h"

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
	void colorSchemeChanged();
	void fontFamilyChanged(const QString &);
	void fontSizeChanged(const QString &);
	void openCSGWarningChanged(bool);

signals:
	void requestRedraw() const;
	void fontChanged(const QString &family, uint size) const;

private:
	Preferences(QWidget *parent = NULL);
	void keyPressEvent(QKeyEvent *e);
	void updateGUI();
	void removeDefaultSettings();

	QSettings::SettingsMap defaultmap;
	QHash<QString, QMap<RenderSettings::RenderColor, QColor> > colorschemes;

	static Preferences *instance;
};

#endif
