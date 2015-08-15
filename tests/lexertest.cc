#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/lex.h"

using namespace std;

class lexer : public LexInterface
{
	public:
	Lex *lex;
	ofstream output;
	lexer(){
		lex = new Lex();	
		lex->rules();
	}
	~lexer(){
		delete lex;
	}
	void highlighting(int start, const std::string& input, lexertl::smatch results)
	{
		output << "<" << results.id << ">" << results.str() <<"</"<< results.id<<">";
	}

	int getStyleAt(int pos)
	{
		return 0;
	}
};
int main(int argv, char* argc[]){

	ifstream newfile(argc[1]);
	lexer l;
	l.output.open(argc[2]);
	stringstream buffer;
	buffer << newfile.rdbuf();
	const string line = buffer.str();
	const string subline = line.substr(0, string::npos);
	int start = 0;
	l.lex->lex_results(subline, start ,&l);
	newfile.close();
}	
