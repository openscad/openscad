#pragma once
#undef DIFFERENCE //#defined in winuser.h

enum class OpenSCADOperator {
	UNION,
	INTERSECTION,
	DIFFERENCE,
	MINKOWSKI,
	HULL,
	RESIZE
};
