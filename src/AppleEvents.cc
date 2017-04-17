#include "AppleEvents.h"
#include <MacTypes.h>
#include <AssertMacros.h>
#include <CoreServices/CoreServices.h>
#include <QApplication>
#include "MainWindow.h"

extern "C" {
	OSErr eventHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
}

OSErr eventHandler(const AppleEvent *, AppleEvent *, SRefCon )
{
// FIXME: Ugly hack; just using the first MainWindow we can find
	MainWindow *mainwin = nullptr;
	for (auto &w : QApplication::topLevelWidgets()) {
		mainwin = qobject_cast<MainWindow*>(w);
		if (mainwin) break;
	}
	if (mainwin) {
		mainwin->actionReloadRenderPreview();
	}
	return noErr;
}

void installAppleEventHandlers()
{
	// Reload handler
  auto err = AEInstallEventHandler('SCAD', 'relo', NewAEEventHandlerUPP(eventHandler), 0, true);
  __Require_noErr(err, CantInstallAppleEventHandler);
	return;

CantInstallAppleEventHandler:
	fprintf(stderr, "AEInstallEventHandler() failed: %d\n", err);	;
}
