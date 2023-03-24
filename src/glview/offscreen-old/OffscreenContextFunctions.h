#pragma once

// Here we implement a 'portability' pattern but since we are mixing
// Objective-C with C++, it is a bit different. The main struct
// isn't defined in the header, but instead inside the source code files

#include <iostream>
#include <fstream>
#include <string>

#include "OffscreenContext.h"

