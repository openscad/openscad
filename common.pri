OBJECTS_DIR = objects
MOC_DIR = objects
UI_DIR = objects
RCC_DIR = objects

include(win32.pri)
include(flex.pri)
include(bison.pri)
include(cgal.pri)
include(opencsg.pri)
include(glew.pri)
include(boost.pri)

CONFIG(eigen2) { include(eigen2.pri) }
CONFIG(eigen3) { include(eigen3.pri) }

