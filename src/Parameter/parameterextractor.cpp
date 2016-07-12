#include "parameterextractor.h"

#include "FileModule.h"
#include "modcontext.h"
#include<QJsonDocument>
#include<QFile>
#include<QJsonObject>
#include<QJsonValue>
#include<QJsonArray>
#include<QDebug>

ParameterExtractor::ParameterExtractor()
{
}

ParameterExtractor::~ParameterExtractor()
{
}

void ParameterExtractor::applyParameters(FileModule *fileModule)
{
    if (fileModule == NULL) {
        return;
    }

    for (AssignmentList::iterator it = fileModule->scope.assignments.begin();it != fileModule->scope.assignments.end();it++) {
        entry_map_t::iterator entry = entries.find((*it).name);
        if (entry == entries.end()) {
            continue;
        }
            (*entry).second->applyParameter(&(*it));
            (*entry).second->set=false;

    }
}

void ParameterExtractor::setParameters(const FileModule* module)
{
    getParameterSet("save.json");
    if (module == NULL) {
        return;
    }

    ModuleContext ctx;

    foreach(Assignment assignment, module->scope.assignments)
    {
        const Annotation *param = assignment.annotation("Parameter");
        if (!param) {
            continue;
        }

        const ValuePtr defaultValue = assignment.expr.get()->evaluate(&ctx);
        if (defaultValue->type() == Value::UNDEFINED) {
            continue;
        }

        ParameterObject *entryObject = new ParameterObject();
        entryObject->setAssignment(&ctx, &assignment, defaultValue);

        if(entries.find(assignment.name) == entries.end() || !(*entryObject==*entries[assignment.name]) ){
            entries[assignment.name] = entryObject;
        }
        else{
               entryObject=entries[assignment.name];
        }

        entryObject->set=true;
    }
    connectWidget();
}

bool ParameterExtractor::getParameterSet(string filename){
        QFile loadFile(QStringLiteral("save.json"));

        if (!loadFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open save file.");
            return false;
        }

        QByteArray saveData = loadFile.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
        QJsonObject SetObject = loadDoc.object();

        for (QJsonObject::iterator value = SetObject.begin(); value!=SetObject.end();value++){
            QJsonValueRef setArray=value.value();
            SetOfParameter setofparameter;
            for(int i=0; i<setArray.toArray().size();i++){
                setofparameter[setArray.toArray().at(i).toObject().begin().key().toStdString()]=setArray.toArray().at(i).toObject().begin().value();
            }
            parameterSet[value.key().toStdString()]=setofparameter;
        }
        print();
        return true;
}

void ParameterExtractor::print(){

    for(ParameterSet::iterator it=parameterSet.begin();it!=parameterSet.end();it++){
        SetOfParameter setofparameter=(*it).second;
        for(SetOfParameter::iterator i=setofparameter.begin();i!=setofparameter.end(); i++){
            std::cout<<i->first<<" ";
        }
    }

}


void ParameterExtractor::applyParameterSet(FileModule *fileModule,string setName)
{
    if (fileModule == NULL) {
        return;
    }

    ParameterSet::iterator set=parameterSet.find(setName);
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
                    assignment->expr = shared_ptr<Expression>(new Literal(ValuePtr(i->second.toString().toStdString())));
             }

         }
    }
}

