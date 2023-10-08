/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2022 Jordan Brown <openscad@jordan.maileater.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * This file defines two helper functions for saving and restoring OpenGL contexts.
 * They are in a separate file (rather than in the more obvious QGLView.cc) because
 * <QOpenGLContext> is incompatible with GLEW and produces compilation warning messages.
 * Putting them in a separate file in helper functions allows the main file to need
 * only an incomplete declaration of QOpenGLContext, which it already has.
 *
 * See also the discussion at QGLView::mouseDoubleClickEvent().
 */
#include <QOpenGLContext>

QOpenGLContext *
getGLContext()
{
  return (QOpenGLContext::currentContext());
}

void
setGLContext(QOpenGLContext *ctx)
{
  /*
   * This seems like the simplest way to select QOpenGLContext.
   *
   * Why isn't there a QOpenGLContext::makeCurrent() that uses the context's assigned
   * surface?
   */
  ctx->makeCurrent(ctx->surface());
}
