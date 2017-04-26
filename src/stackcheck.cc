#include <cstdlib>

#include "stackcheck.h"
#include "PlatformUtils.h"

StackCheck *StackCheck::self = nullptr;

StackCheck::StackCheck() : ptr(nullptr)
{
}

StackCheck::~StackCheck()
{
}

void StackCheck::init()
{
	unsigned char c;
	ptr = &c;
}

unsigned long StackCheck::size()
{
	unsigned char c;
	return std::labs(ptr - &c);
}

bool StackCheck::check()
{
	return size() >= PlatformUtils::stackLimit();
}

StackCheck *StackCheck::inst()
{
	if (self == 0) {
		self = new StackCheck();
	}
	return self;
}
