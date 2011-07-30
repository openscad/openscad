#include "PolySetEvaluator.h"
#include "printutils.h"
#include "polyset.h"

PolySetEvaluator *PolySetEvaluator::global_evaluator = NULL;

PolySetEvaluator::cache_entry::cache_entry(PolySet *ps) :
		ps(ps), msg(print_messages_stack.last())
{
}

PolySetEvaluator::cache_entry::~cache_entry()
{
	ps->unlink();
}
