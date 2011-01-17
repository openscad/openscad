#include "mathc99.h"

double found(double a) {
    return a > 0 ? floor(a+0.5) : ceil(a-0.5);
}
