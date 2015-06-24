#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../src/lex.h"

using namespace std;

class lexer : public LexInterface
{
	public:
	string token;
	Lex *lex;
	ofstream output;
	lexer(){
		lex = new Lex();	
		lex->rules();
	}
	void highlighting(int start, const std::string& input, lexertl::smatch results, int style)
	{
		output<< "<"<<results.id<<">"<< results.str()<<"</"<<results.id<<">";
	}
};
int main(int argv, char* argc[]){

	ifstream newfile(argc[1]);
	lexer l;
	l.output.open(argc[2]);
	stringstream buffer;
	buffer << newfile.rdbuf();
	const string line = buffer.str();
	int start = 0;
	l.lex->lex_results(line, start ,&l);
	newfile.close();
}	
