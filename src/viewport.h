#ifndef VIEWPORT_H_
#define VIEWPORT_H_

#include "GLView.h"

/*
Viewport::screen is used to communicate between the command parser
and the MainWindow.

has_run_once is used for the starting_viewport() function to make sure
it only runs once per .scad file load.
*/

namespace Viewport
{
	extern GLView *screen;
	extern bool has_run_once;
}

#endif

