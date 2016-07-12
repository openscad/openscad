#include "parameterset.h"

#include "FileModule.h"
#include "modcontext.h"

#include<QDebug>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


// Short alias for this namespace
namespace pt = boost::property_tree;

ParameterSet::ParameterSet()
{
}

bool ParameterSet::getParameterSet(string filename){

    try{
            std::fstream myfile;
            myfile.open (filename);
            // send your JSON above to the parser below, but populate ss first

            pt::ptree root;
            pt::read_json(myfile, root);

            for(pt::ptree::value_type &v : root.get_child("SET")) {

                // The data function is used to access the data stored in a node.
                SetOfParameter setofparameter;
                std::cout<<"model name"<<v.first<<std::endl;
                for(pt::ptree::value_type &vi : v.second){

                      setofparameter[vi.first]=vi.second.data();
                      std::cout<<vi.first<<std::endl;
                }
                parameterSet[v.first]=setofparameter;
            }

            myfile.close();
            print();
       }
       catch (std::exception const& e)
       {
           std::cerr << e.what() << std::endl;
       }
}

void ParameterSet::print(){

    for(Parameterset::iterator it=parameterSet.begin();it!=parameterSet.end();it++){
        SetOfParameter setofparameter=(*it).second;
        for(SetOfParameter::iterator i=setofparameter.begin();i!=setofparameter.end(); i++){
            std::cout<<i->first<<" ";
        }
    }

}


void ParameterSet::applyParameterSet(FileModule *fileModule,string setName)
{
    if (fileModule == NULL) {
        return;
    }

    Parameterset::iterator set=parameterSet.find(setName);
    if(set==parameterSet.end()){
        qWarning("no set");
        return ;
     }

    SetOfParameter setofparameter=set->second;

    for (AssignmentList::iterator it = fileModule->scope.assignments.begin();it != fileModule->scope.assignments.end();it++) {

        for(SetOfParameter::iterator i = setofparameter.begin();i!=setofparameter.end();i++){
            if(i->first== (*it).name){
                Assignment *assignment;
                    assignment=&(*it);
                    assignment->expr = shared_ptr<Expression>(new Literal(ValuePtr(i->second)));
             }

         }
    }
}

