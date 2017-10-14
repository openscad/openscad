#pragma once

class StackCheck
{
public:
	StackCheck();
	virtual ~StackCheck();
	
	static StackCheck * inst();
	
	void init();
	bool check();
	unsigned long size();
  
private:
	unsigned char * ptr;
  
	static StackCheck *self;
};
