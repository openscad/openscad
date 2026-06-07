#pragma once

// Thread-local storage for QOpenGL functions on macOS/Qt6
// This allows GLView.cc and other files to access OpenGL functions
// that are managed by QOpenGLFunctions in QGLView.cc

class QOpenGLFunctions;

// Set the current GL functions context for the thread
// Call this in QGLView::paintGL() and QGLView::initializeGL()
void setCurrentGLFunctions(QOpenGLFunctions *gl);

// Get the current GL functions context for the thread
QOpenGLFunctions *getCurrentGLFunctions();
