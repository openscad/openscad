!lessThan(QT_MAJOR_VERSION, 5) {
  CONFIG += has_qt5
}

has_qt5:!lessThan(QT_MINOR_VERSION, 4) {
  CONFIG += has_qopenglwidget
}

has_qt5:!lessThan(QT_MINOR_VERSION, 9) {
  CONFIG += has_ccache
}
