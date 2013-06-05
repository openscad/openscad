#include <Carbon/Carbon.h>
#include <QApplication>
#include "MainWindow.h"

extern "C" {
	OSErr eventHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
}

OSErr eventHandler(const AppleEvent *, AppleEvent *, SRefCon )
{
// FIXME: Ugly hack; just using the first MainWindow we can find
	MainWindow *mainwin = NULL;
	foreach (QWidget *w, QApplication::topLevelWidgets()) {
		mainwin = qobject_cast<MainWindow*>(w);
		if (mainwin) break;
	}
	if (mainwin) {
		mainwin->actionReloadRenderCSG();
	}
	return noErr;
}

void installAppleEventHandlers()
{
	// Reload handler
  OSErr err = AEInstallEventHandler('SCAD', 'relo', NewAEEventHandlerUPP(eventHandler), 0, true);
  require_noerr(err, CantInstallAppleEventHandler);
	return;

CantInstallAppleEventHandler:
	fprintf(stderr, "AEInstallEventHandler() failed: %d\n", err);	;
}
