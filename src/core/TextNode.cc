/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
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

#include "Children.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "Parameters.h"
#include "printutils.h"
#include "Builtins.h"

#include "TextNode.h"
#include "FreetypeRenderer.h"

#include <Python.h>
#include <pyopenscad.h>

#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

static std::shared_ptr<AbstractNode> builtin_text(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", inst->name());
  }

  auto node = std::make_shared<TextNode>(inst);

  auto *session = arguments.session();
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
                                            {"text", "size", "font"},
                                            {"direction", "language", "script", "halign", "valign", "spacing"}
                                            );
  parameters.set_caller("text");

  node->params.set_loc(inst->location());
  node->params.set_documentPath(session->documentRoot());
  node->params.set(parameters);
  node->params.detect_properties();

  return node;
}

PyObject* python_text(PyObject *self, PyObject *args, PyObject *kwargs)
{
  auto node = std::make_shared<TextNode>(&todo_fix_inst);

  char * kwlist[] ={"text","size","font","spacing","direction","language","script","halign","valign","fn","fa","fs",NULL};

  double size=1.0, spacing = 1.0 ;
  double fn=-1,fa=-1,fs=-1;

   get_fnas(fn,fa,fs);

  const char *text="", *font=NULL, *direction ="ltr", *language = "en", *script = "latin", *valign = "baseline", *halign = "left";

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|dsdsssssddd", kwlist, 
                           &text,&size, &font,
			   &spacing, &direction,&language,
			   &script, &valign, &halign,
			   &fn,&fa,&fs
                           )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
        return NULL;
  }

  node->params.set_fn(fn);
  node->params.set_fa(fa);
  node->params.set_fs(fs);
  node->params.set_size(size);
  if(text != NULL) node->params.set_text(text);
  node->params.set_spacing(spacing);
  if(font != NULL) node->params.set_font(font);
  if(direction != NULL) node->params.set_direction(direction);
  if(language != NULL) node->params.set_language(language);
  if(script != NULL) node->params.set_script(script);
  if(valign != NULL) node->params.set_halign(halign);
  if(halign != NULL) node->params.set_valign(valign);
  node->params.set_loc(todo_fix_inst.location());

/*
  node->params.set_documentPath(session->documentRoot());
  node->params.detect_properties();
}
*/

	return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}


PyObject* python_textmetrics(PyObject *self, PyObject *args, PyObject *kwargs)
{
  auto node = std::make_shared<TextNode>(&todo_fix_inst);

  char * kwlist[] ={"text","size","font","spacing","direction","language","script","halign","valign",NULL};

  double size=1.0, spacing = 1.0 ;

  const char *text="", *font=NULL, *direction ="ltr", *language = "en", *script = "latin", *valign = "baseline", *halign = "left";

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|dsdsssss", kwlist, 
                           &text,&size, &font,
			   &spacing, &direction,&language,
			   &script, &valign, &halign
                           )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
        return NULL;
  }

  FreetypeRenderer::Params ftparams;

  ftparams.set_size(size);
  if(text != NULL) ftparams.set_text(text);
  ftparams.set_spacing(spacing);
  if(font != NULL) ftparams.set_font(font);
  if(direction != NULL) ftparams.set_direction(direction);
  if(language != NULL) ftparams.set_language(language);
  if(script != NULL) ftparams.set_script(script);
  if(valign != NULL) ftparams.set_halign(halign);
  if(halign != NULL) ftparams.set_valign(valign);
  ftparams.set_loc(todo_fix_inst.location());

  FreetypeRenderer::TextMetrics metrics(ftparams);
  if (!metrics.ok) {
    PyErr_SetString(PyExc_TypeError,"Invalid Metric\n");
    return NULL;
  }
  PyObject *offset = PyList_New(2);
  PyList_SetItem(offset,0,PyFloat_FromDouble(metrics.x_offset));
  PyList_SetItem(offset,1,PyFloat_FromDouble(metrics.y_offset));

  PyObject *advance = PyList_New(2);
  PyList_SetItem(advance,0,PyFloat_FromDouble(metrics.advance_x));
  PyList_SetItem(advance,1,PyFloat_FromDouble(metrics.advance_y));

  PyObject *position = PyList_New(2);
  PyList_SetItem(position,0,PyFloat_FromDouble(metrics.bbox_x));
  PyList_SetItem(position,1,PyFloat_FromDouble(metrics.bbox_y));

  PyObject *dims = PyList_New(2);
  PyList_SetItem(dims,0,PyFloat_FromDouble(metrics.bbox_w));
  PyList_SetItem(dims,1,PyFloat_FromDouble(metrics.bbox_h));

  PyObject *dict;
  dict = PyDict_New();
  PyDict_SetItemString(dict,"ascent",PyFloat_FromDouble(metrics.ascent));
  PyDict_SetItemString(dict,"descent",PyFloat_FromDouble(metrics.descent));
  PyDict_SetItemString(dict,"offset",offset);
  PyDict_SetItemString(dict,"advance",advance);
  PyDict_SetItemString(dict,"position",position);
  PyDict_SetItemString(dict,"size",dims);
  return (PyObject *)dict;
}

std::vector<const Geometry *> TextNode::createGeometryList() const
{
  FreetypeRenderer renderer;
  return renderer.render(this->get_params());
}

FreetypeRenderer::Params TextNode::get_params() const
{
  return params;
}

std::string TextNode::toString() const
{
  return STR(name(), "(", this->params, ")");
}

void register_builtin_text()
{
  Builtins::init("text", new BuiltinModule(builtin_text),
  {
    R"(text(text = "", size = 10, font = "", halign = "left", valign = "baseline", spacing = 1, direction = "ltr", language = "en", script = "latin"[, $fn]))",
  });
}
