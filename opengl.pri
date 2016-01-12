# Prefer QOpenGLWidget for non-Windows platforms
# To explicitly enable QOpenGLWidget: qmake CONFIG += qopenglwidget
# To explicitly enable QGLWidget: qmake CONFIG += qglwidget
!win*: CONFIG += qopenglwidget
qopenglwidget:!qglwidget:!lessThan(QT_VERSION, 5.4): CONFIG += using_qopenglwidget

using_qopenglwidget {
  message("Using QOpenGLWidget")
  DEFINES += USE_QOPENGLWIDGET
}
else {
  message("Using QGLWidget")
  QT += opengl
}

# see http://fedoraproject.org/wiki/UnderstandingDSOLinkChange
# and https://github.com/openscad/openscad/pull/119
# ( QT += opengl does not automatically link glu on some DSO systems. )
unix:!macx {
  QMAKE_LIBS_OPENGL *= -lGLU
  QMAKE_LIBS_OPENGL *= -lX11
}
