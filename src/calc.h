#pragma once

namespace Calc {
double lerp(double a, double b, double t);
int get_fragments_from_r(double r, double fn, double fs, double fa);
int get_helix_slices(double r_sqr, double h, double twist, double fn, double fs, double fa);
int get_conical_helix_slices(double r, double height, double twist, double scale, double fn, double fs, double fa);
int get_diagonal_slices(double delta_sqr, double height, double fn, double fs);
}
