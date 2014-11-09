#include "editor.h"
#include "Preferences.h"

QMap<QString, QString> EditorInterface::knownFileExtensions;

EditorInterface::EditorInterface(QWidget* parent) : QWidget(parent)
{
}

QMap<QString, QString> & EditorInterface::getKnownFileExtensions()
{
    if (knownFileExtensions.empty()) {
	const QString importStatement = "import(\"%1\");\n";
	const QString surfaceStatement = "surface(\"%1\");\n";
	knownFileExtensions["stl"] = importStatement;
	knownFileExtensions["off"] = importStatement;
	knownFileExtensions["dxf"] = importStatement;
	knownFileExtensions["dat"] = surfaceStatement;
	knownFileExtensions["png"] = surfaceStatement;
	knownFileExtensions["scad"] = "";
	knownFileExtensions["csg"] = "";
    }
    return knownFileExtensions;
}

void EditorInterface::wheelEvent(QWheelEvent *event)
{
	QSettings settings;
	bool wheelzoom_enabled = Preferences::inst()->getValue("editor/ctrlmousewheelzoom").toBool();
	if ((event->modifiers() == Qt::ControlModifier) && wheelzoom_enabled) {
		if (event->delta() > 0)
			zoomIn();
		else if (event->delta() < 0)
			zoomOut();
	} else {
		QWidget::wheelEvent(event);
	}
}

void EditorInterface::onFileDropped(const QString &filename)
{
	emit fileDropped(filename);
}
