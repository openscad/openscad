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
		int token_pos = std::distance(input.begin(), results.start);
		if(token_pos >= start){
			output << "<" << results.id << ">" << results.str() <<"</"<< results.id<<">";
		 } else {
			output << results.str();
		}
	}

	int getStyleAt(int pos)
	{
		return 0;
	}
};
int main(int argv, char* argc[]){

	ifstream newfile(argc[1]);
	std::string extension;
	lexer l;
	int start;
	const std::string  &fileName = argc[1];
	if(fileName.find_last_of(".") != std::string::npos) {
		extension =  fileName.substr(fileName.find_last_of(".")+1);
	}
	l.output.open(argc[2]);
	stringstream buffer;
	buffer << newfile.rdbuf();
	const string line = buffer.str();
	const string subline = line.substr(0, string::npos);
	string word = line.substr(6, '\n');
	if(extension == "scad") {
		start = 0;
	}
	else if(extension == "test") {
		start = atoi(word.c_str()) + 8;
	}
	l.lex->lex_results(subline, start ,&l);
	newfile.close();
}	
