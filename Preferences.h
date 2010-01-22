#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include <QMainWindow>
#include "ui_Preferences.h"

class Preferences : public QMainWindow, public Ui::Preferences
{
	Q_OBJECT;

public:
	~Preferences();
	static Preferences *inst() { if (!instance) instance = new Preferences(); return instance; }

	enum RenderColor {
		BACKGROUND_COLOR,
		OPENCSG_FACE_FRONT_COLOR,
		OPENCSG_FACE_BACK_COLOR,
		CGAL_FACE_FRONT_COLOR,
		CGAL_FACE_2D_COLOR,
		CGAL_FACE_BACK_COLOR,
		CGAL_EDGE_FRONT_COLOR,
		CGAL_EDGE_BACK_COLOR,
		CGAL_EDGE_2D_COLOR,
		CROSSHAIR_COLOR
	};
	const QColor &color(RenderColor idx);

public slots:
	void actionTriggered(class QAction *);
	void colorSchemeChanged();
	void fontFamilyChanged(const QString &);
	void fontSizeChanged(const QString &);

signals:
	void requestRedraw();
	void fontChanged(const QString &family, uint size);

private:
	Preferences(QWidget *parent = NULL);
	void keyPressEvent(QKeyEvent *e);

	QHash<QString, QMap<RenderColor, QColor> > colorschemes;
	QString colorscheme;
	QString fontfamily;
	uint fontsize;

	static Preferences *instance;
};

#endif
