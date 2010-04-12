#include "PolySetRenderer.h"
#include "printutils.h"
#include "polyset.h"

PolySetRenderer *PolySetRenderer::global_renderer = NULL;

PolySetRenderer::cache_entry::cache_entry(PolySet *ps) :
		ps(ps), msg(print_messages_stack.last())
{
}

PolySetRenderer::cache_entry::~cache_entry()
{
	ps->unlink();
}
