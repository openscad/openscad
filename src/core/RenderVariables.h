#include "glview/Camera.h"
#include "core/Context.h"
#include "core/BuiltinContext.h"

class RenderVariables
{
public:
  bool preview;
  double time;
  Camera camera;
  void applyToContext(ContextHandle<BuiltinContext>& context) const;
};
