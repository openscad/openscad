#include "scadapi.h"
#include "builtin.h"

ScadApi::ScadApi(QsciScintilla *qsci, QsciLexer *lexer) : QsciAbstractAPIs(lexer), qsci(qsci)
{
	QMap<QString, QStringList>::const_iterator iter = Builtins::keywordList.constBegin();
    auto end = Builtins::keywordList.constEnd();
    while (iter != end) {
        funcs.append(ApiFunc(iter.key(), iter.value()));
        ++iter;
    }

// 	/*
// 	 * 2d primitives
// 	 */
// 	QStringList circle;
// 	circle
// 		<< "circle(radius)"
// 		<< "circle(r = radius)"
// 		<< "circle(d = diameter)";
// 	funcs.append(ApiFunc("circle", circle));

// 	QStringList square;
// 	square
// 		<< "square(size, center = true)"
// 		<< "square([width,height], center = true)";
// 	funcs.append(ApiFunc("square", square));

// 	QStringList polygon;
// 	polygon
// 		<< "polygon([points])"
// 		<< "polygon([points], [paths])";
// 	funcs.append(ApiFunc("polygon", polygon));

// 	/*
// 	 * 3d primitives
// 	 */
// 	QStringList cube;
// 	cube
// 		<< "cube(size)"
// 		<< "cube([width, depth, height])"
// 		<< "cube(size = [width, depth, height], center = true)";
// 	funcs.append(ApiFunc("cube", cube));

// 	QStringList sphere;
// 	sphere
// 		<< "sphere(radius)"
// 		<< "sphere(r = radius)"
// 		<< "sphere(d = diameter)";
// 	funcs.append(ApiFunc("sphere", sphere));

// 	QStringList cylinder;
// 	cylinder
// 		<< "cylinder(h, r1, r2)"
// 		<< "cylinder(h = height, r = radius, center = true)"
// 		<< "cylinder(h = height, r1 = bottom, r2 = top, center = true)"
// 		<< "cylinder(h = height, d = diameter, center = true)"
// 		<< "cylinder(h = height, d1 = bottom, d2 = top, center = true)";
// 	funcs.append(ApiFunc("cylinder", cylinder));

// 	funcs.append(ApiFunc("polyhedron", "polyhedron(points, triangles, convexity)"));

// 	/*
// 	 * operations
// 	 */
// 	funcs.append(ApiFunc("translate", "translate([x, y, z])"));
// 	funcs.append(ApiFunc("rotate", "rotate([x, y, z])"));
// 	funcs.append(ApiFunc("scale", "scale([x, y, z])"));
// 	funcs.append(ApiFunc("resize", "resize([x, y, z], auto)"));
// 	funcs.append(ApiFunc("mirror", "mirror([x, y, z])"));
// 	funcs.append(ApiFunc("multmatrix", "multmatrix(m)"));

// 	funcs.append(ApiFunc("module", "module"));

// 	funcs.append(ApiFunc("difference", "difference()"));
// 	funcs.append(ApiFunc("union", "union()"));
// 	funcs.append(ApiFunc("use", "use"));
// 	funcs.append(ApiFunc("include", "include"));
// 	funcs.append(ApiFunc("function", "function"));

// 	funcs.append(ApiFunc("abs", "abs(number) -> number"));
// 	funcs.append(ApiFunc("sign", "sign(number) -> -1, 0 or 1"));
// 	funcs.append(ApiFunc("sin", "sin(degrees) -> number"));
// 	funcs.append(ApiFunc("cos", "cos(degrees) -> number"));
// 	funcs.append(ApiFunc("tan", "tan(degrees) -> number"));
// 	funcs.append(ApiFunc("acos", "acos(number) -> degrees"));
// 	funcs.append(ApiFunc("asin", "asin(number) -> degrees"));
// 	funcs.append(ApiFunc("atan", "atan(number) -> degrees"));
// 	funcs.append(ApiFunc("atan2", "atan2(number, number) -> degrees"));
// 	funcs.append(ApiFunc("floor", "floor(number) -> number"));
// 	funcs.append(ApiFunc("round", "round(number) -> number"));
// 	funcs.append(ApiFunc("ceil", "ceil(number) -> number"));
// 	funcs.append(ApiFunc("ln", "ln(number) -> number"));
// 	funcs.append(ApiFunc("len", "len(string) -> number", "len(array) -> number"));
// 	funcs.append(ApiFunc("log", "log(number) -> number"));
// 	funcs.append(ApiFunc("pow", "pow(base, exponent) -> number"));
// 	funcs.append(ApiFunc("sqrt", "sqrt(number) -> number"));
// 	funcs.append(ApiFunc("exp", "exp(number) -> number"));
// 	funcs.append(ApiFunc("rands", "rands(min, max, num_results) -> array", "rands(min, max, num_results, seed) -> array"));
// 	funcs.append(ApiFunc("min", "min(number, number, ...) -> number", "min(array) -> number"));
// 	funcs.append(ApiFunc("max", "max(number, number, ...) -> number", "max(array) -> number"));
}

ScadApi::~ScadApi()
{
}

void ScadApi::updateAutoCompletionList(const QStringList &context, QStringList &list)
{
	const QString c = context.last();
	for (int a = 0;a < funcs.size();a++) {
		const ApiFunc &func = funcs.at(a);
		const QString &name = func.get_name();
		if (name.startsWith(c)) {
			if (!list.contains(name)) {
				list << name;
			}
		}
	}
}

void ScadApi::autoCompletionSelected (const QString &selection)
{
}

QStringList ScadApi::callTips (const QStringList &context, int commas, QsciScintilla::CallTipsStyle style, QList< int > &shifts)
{
	QStringList callTips;
	for (int a = 0;a < funcs.size();a++) {
		if (funcs.at(a).get_name() == context.at(context.size() - 2)) {
			callTips = funcs.at(a).get_params();
			break;
		}
	}
	return callTips;
}