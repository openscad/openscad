#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../src/lex.h"

using namespace std;

class lexer
{
	public:
	string token;
	Lex *lex;
	
	lexer(){
		lex = new Lex();	
		lex->rules();
/*		vector <std::string> keywords{"var", "module", "function", "use", "echo", "include", "import", "group", "projection", "render", "surface","def", "enum", "struct", "def", "return"};
		vector <std::string> transformations{"translate", "rotate", "child", "scale", "linear_extrude", "rotate_extrude", "resize", "mirror", "multmatrix", "color", "offset", "hull", "minkowsi"};
		vector <std::string> booleans{"union", "difference", "interaction", "true", "false"};
		vector <std::string> models{"sphere", "cube", "cylinder", "polyhedron", "square", "polygon", "text", "circle"};
		
//	 rules_.push_state("COMMENT");
	 defineRules(keywords, 5);
	 defineRules(transformations, 6);
	 defineRules(booleans, 7);
	 defineRules(models, 8);*/
//	 rules_.push("[0-9]+", 2);
//	 rules_.push("[a-zA-Z0-9_]+", 1);
//	 rules_.push(".",3);
//	 rules_.push("\n", 4);
//	 rules_.push("INITIAL", "\"/*\"", "COMMENT");
//	 rules_.push("COMMENT", "[^*]+|.", ".");
//	 rules_.push("COMMENT", "\"*/\"",9, "INITIAL");
//	 rules_.push("[/][/].*$", 9);
//	lexertl::generator::build(lex->rules_, lex->sm);
	}

/*	void defineRules(vector<string> keywords, int id){
		for(vector<string>::iterator it = keywords.begin(); it != keywords.end(); ++it){
			rules_.push(*it, id);	
		}
	}*/
};
int main(int argv, char* argc[]){

	ifstream newfile(argc[1]);
	ofstream output(argc[2]);
	string line;
 	getline (newfile, line, '\0');
	lexer l;
	const std::string input(line);
	lexertl::smatch results(input.begin(), input.end());
	lexertl::lookup(l.lex->sm, results);
	while(results.id != 0)	{
		if(results.id == 1){
			output<< "<var>"<< results.str()<<"</var>";
		}
		if(results.id == 2){
			output << "<num>"<<results.str()<<"</num>";
		}
		 if(results.id == 3){
			output << results.str();
		}
		 if(results.id == 4){
			output << results.str();
		}
		 if(results.id == 5){
			output << "<keyword>" << results.str() << "</keyword>";
		}
		 if(results.id == 6){
			output << "<transformation>" << results.str() << "</transformation>";
		}
		 if(results.id == 7){
			output << "<booleans>" << results.str() << "</booleans>";
		}
		 if(results.id == 8){
			output << "<models>" << results.str() << "</models>";
		}
		 if(results.id == 9){
			output << "<Comment>" << results.str() << "</Comment>";
		}
	lexertl::lookup(l.lex->sm, results);
	}
	newfile.close();
}	
