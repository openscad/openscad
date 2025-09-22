#include "glview/Camera.h"
class BuiltinContext;
template <typename T>
class ContextHandle;

class RenderVariables
{
public:
  bool preview;
  double time;
  Camera camera;
  void applyToContext(ContextHandle<BuiltinContext>& context) const;
};
