#include "printutils.h"
#include "MainWindow.h"

void PRINT(const QString &msg)
{
  do {
    if (MainWindow::current_win.isNull()) {
      fprintf(stderr, "%s\n", msg.toAscii().data()); 
    }
    else {
      MainWindow::current_win->console->append(msg);
    }
  } while (0);
}
