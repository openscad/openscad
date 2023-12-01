#include "RenderVariables.h"
#include "Context.h"
#include "BuiltinContext.h"
#include "Value.h"

void
RenderVariables::setRenderVariables(ContextHandle<BuiltinContext>& context) const
{
  context->set_variable("$preview", Value(preview));
  context->set_variable("$t", Value(time));

  auto vpr = camera.getVpr();
  context->set_variable("$vpr", Value(VectorType(context->session(),
    vpr.x(), vpr.y(), vpr.y())));
  auto vpt = camera.getVpt();
  context->set_variable("$vpt", Value(VectorType(context->session(),
    vpt.x(), vpt.y(), vpt.z())));
  auto vpd = camera.zoomValue();
  context->set_variable("$vpd", Value(vpd));
  auto vpf = camera.fovValue();
  context->set_variable("$vpf", Value(vpf));
}
