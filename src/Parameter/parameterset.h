#ifndef PARAMETERSET_H
#define PARAMETERSET_H

#include"expression.h"
#include "FileModule.h"

#include<map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

using namespace std;

class ParameterSet
{

protected:
    pt::ptree root;
    fstream myfile;
    typedef map<string,pt::ptree::value_type> Parameterset;
    Parameterset parameterSet;

public:
    ParameterSet();
    ~ParameterSet();
    void getParameterSet(string filename);
    void applyParameterSet(FileModule *fileModule,string setName);

};

#endif // PARAMETERSET_H
