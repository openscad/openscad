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
		int s = std::distance(input.begin(), results.start);
		static int p = 0;
		static bool flag = 0;
		if (s == 0 && results.str() == "start"){
			p = 6;
			flag = 1;
			std::cout << "ppp: "<<results.str()<<std::endl;
		}
			std::cout << "flag: "<< flag<<std::endl;
			if(flag && s == 6){
				std::cout<<s<<'\t'<<results.str()<<std::endl;
				p = atoi(results.str().c_str());
				std::cout << "sss:"<<p <<std::endl;
				
		}
				std::cout << "ooo:"<<p <<std::endl;
	
		if(s >= p){
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
	lexer l;
	l.output.open(argc[2]);
	stringstream buffer;
	buffer << newfile.rdbuf();
	const string line = buffer.str();
	const string subline = line.substr(0, string::npos);
	int start = 50;
	l.lex->lex_results(subline, start ,&l);
	newfile.close();
}	
