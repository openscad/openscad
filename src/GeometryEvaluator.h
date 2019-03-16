#ifdef ENABLE_CGALNEF
#include "CGALNefEvaluator.h"
typedef class CGALNefEvaluator geom_eval_t;
#else
#include "BaseGeometryEvaluator.h"
typedef class GeometryEvaluator geom_eval_t;
#endif
