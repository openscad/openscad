#include "Camera.h"
#include "Context.h"
#include "BuiltinContext.h"

class RenderVariables
{
public:
  bool preview;
  double time;
  Camera camera;
  void applyToContext(ContextHandle<BuiltinContext>& context) const;
};
