# Prefer QOpenGLWidget when not doing an mxe cross build
# To explicitly enable QOpenGLWidget: qmake CONFIG += qopenglwidget
# To explicitly enable QGLWidget: qmake CONFIG += qglwidget
!win*: CONFIG += qopenglwidget
qopenglwidget:!qglwidget:!lessThan(QT_VERSION, 5.4): CONFIG += using_qopenglwidget
contains(OSNAME,Msys) {
  CONFIG += using_qopenglwidget
}

using_qglwidget {
  message("Using QGLWidget")
  DEFINES += USE_QGLWIDGET
  QT += opengl
}

# see http://fedoraproject.org/wiki/UnderstandingDSOLinkChange
# and https://github.com/openscad/openscad/pull/119
# ( QT += opengl does not automatically link glu on some DSO systems. )
unix:!macx {
  QMAKE_LIBS_OPENGL *= -lGLU
  QMAKE_LIBS_OPENGL *= -lX11
}
