#include "AutoUpdater.h"
#include <QAction>
#include <QMenuBar>

AutoUpdater *AutoUpdater::updater_instance = nullptr;

void AutoUpdater::init()
{
#ifdef OPENSCAD_UPDATER
	if (!this->updateAction) {
		auto mb = new QMenuBar();
		this->updateMenu = mb->addMenu("special");
		this->updateAction = new QAction("Check for Update..", this);
		// Add to application menu
		this->updateAction->setMenuRole(QAction::ApplicationSpecificRole);
		this->updateAction->setEnabled(true);
		this->connect(this->updateAction, SIGNAL(triggered()), this, SLOT(checkForUpdates()));

		this->updateMenu->addAction(this->updateAction);

	}
#endif // ifdef OPENSCAD_UPDATER
}
