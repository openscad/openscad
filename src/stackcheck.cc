#include <cstdlib>

#include "stackcheck.h"
#include "PlatformUtils.h"

StackCheck * StackCheck::self = 0;

StackCheck::StackCheck() : ptr(0)
{
}

StackCheck::~StackCheck()
{
}

void StackCheck::init()
{
    ptr = sp();
}

unsigned long StackCheck::size()
{
    return std::labs(ptr - sp());
}

bool StackCheck::check()
{
    return size() >= PlatformUtils::stackLimit();
}

StackCheck * StackCheck::inst()
{
    if (self == 0) {
        self = new StackCheck();
    }
    return self;
}
