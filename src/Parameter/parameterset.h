#ifndef PARAMETERSET_H
#define PARAMETERSET_H

#include"expression.h"
#include "FileModule.h"
#include<map>

using namespace std;

class ParameterSet
{

    typedef map<string,string >SetOfParameter;
    typedef map<string,SetOfParameter> Parameterset;
     Parameterset parameterSet;

public:

     ParameterSet();
     bool getParameterSet(string filename);
     void print();
     void applyParameterSet(FileModule *fileModule,string setName);

};

#endif // PARAMETERSET_H
