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
    unsigned char * sp() { unsigned char c; return &c; };
    
    unsigned char * ptr;
    
    static StackCheck *self;
};
