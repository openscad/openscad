#include "lex.h"
#include <fstream>
#include <iostream>
Lex::Lex()
{
	
}

void Lex::rules(){

	int s_size = sizeof(std::string);
	std::string keywords[] = {"var", "module", "function", "use", "echo", "include", "import", "group", "projection", "render", "surface","def", "enum", "struct", "fn", "typedef", "file", "namespace", "package", "interface", "param", "see","return", "class", "brief", "if", "else", "let", "for", "undef"};
	int keywords_count = (sizeof(keywords)/s_size);

	std::string transformations[] = {"translate", "rotate", "child", "scale", "linear_extrude", "rotate_extrude", "resize", "mirror", "multmatrix", "color", "offset", "hull", "minkowski", "children", "assign", "intersection_for"};
	int transformations_count = (sizeof(transformations)/s_size);

	 std::string booleans[] = {"union", "difference", "intersection", "true", "false"};
	int booleans_count = (sizeof(booleans)/s_size);

	std::string functions[] = {"abs", "sign", "rands", "min", "max", "sin", "cos", "asin", "acos", "tan", "atan", "atan2", "round", "ceil", "floor", "pow", "sqrt", "exp", "len", "log", "ln", "str", "chr", "concat", "lookup", "search", "version", "version_num", "norm", "cross", "parent_module", "dxf_dim", "dxf_cross"};
	int functions_count = (sizeof(functions)/s_size);
	 
	std::string models[] = {"sphere", "cube", "cylinder", "polyhedron", "square", "polygon", "text", "circle"};
	int models_count = (sizeof(models)/s_size);

	 std::string operators[] = {"<=", ">=", "==", "!=", "&&", "="};
	int operators_count = (sizeof(operators)/s_size);


	rules_.push_state("COMMENT");
	rules_.push_state("MODIFIER");
	rules_.push_state("MODIFIER2");
	rules_.push_state("MODIFIER3");
	rules_.push_state("MODIFIER4");
	rules_.push_state("BLOCK");
	rules_.push_state("BLOCK2");
	rules_.push_state("BLOCK3");
	rules_.push_state("BLOCK4");
	defineRules(keywords, keywords_count, ekeyword);
	defineRules(transformations, transformations_count, etransformation);
	defineRules(booleans, booleans_count, eboolean);
	defineRules(functions, functions_count, efunction); 
	defineRules(models, models_count, emodel);
	defineRules(operators, operators_count, eoperator);
 
	rules_.push("[0-9]+", enumber);
	rules_.push("[a-zA-Z0-9_]+", evariable);
	rules_.push("[$][a-zA-Z0-9_]+", especialVariable);

	rules_.push("INITIAL", "#", "MODIFIER");
	rules_.push("MODIFIER","[^;\\{]+", "MODIFIER");
	rules_.push("MODIFIER","[{]", "BLOCK");
	rules_.push("BLOCK", "[^\\}]+" , "BLOCK");	
	rules_.push("BLOCK", "[}]", 12, "INITIAL");
	rules_.push("MODIFIER", ";", 13, "INITIAL");

	rules_.push("INITIAL", "!", "MODIFIER2");
	rules_.push("MODIFIER2","[^;\\{]+", "MODIFIER2");
	rules_.push("MODIFIER2","[{]", "BLOCK2");
	rules_.push("BLOCK2", "[^\\}]+" , "BLOCK2");	
	rules_.push("BLOCK2", "[}]", 14, "INITIAL");
	rules_.push("MODIFIER2", ";", 15, "INITIAL");

	//rules_.push("INITIAL", "*", "MODIFIER3");
	rules_.push("MODIFIER3","[^;\\{]+", "MODIFIER3");
	rules_.push("MODIFIER3","[{]", "BLOCK3");
	rules_.push("BLOCK3", "[^\\}]+" , "BLOCK3");	
	rules_.push("BLOCK3", "[}]", 16, "INITIAL");
	rules_.push("MODIFIER3", ";", 17, "INITIAL");

	rules_.push("INITIAL", "%", "MODIFIER4");
	rules_.push("MODIFIER4","[^;\\{]+", "MODIFIER4");
	rules_.push("MODIFIER4","[{]", "BLOCK4");
	rules_.push("BLOCK4", "[^\\}]+" , "BLOCK4");	
	rules_.push("BLOCK4", "[}]", 18, "INITIAL");
	rules_.push("MODIFIER4", ";", 19, "INITIAL");

	rules_.push("INITIAL", "\"/*\"",  "COMMENT");
	rules_.push("COMMENT", "[^*]+|.",  "COMMENT");
	rules_.push("COMMENT", "\"*/\"", ecomment , "INITIAL");

	rules_.push("[/][/].*$", ecomment);
	rules_.push(".|\n", etext);
	lexertl::generator::build(rules_, sm);
	std::ofstream fout("file1.txt", std::fstream::trunc);
	lexertl::debug::dump(sm, fout);

}

void Lex::defineRules(std::string words[], int size, int id){

	for(int it = 0; it < size; it++){
		rules_.push(words[it], id);	
	}
}

void Lex::lex_results(const std::string& input, int start, LexInterface* const obj, int startState, int posState){
	
	lexertl::smatch results (input.begin(), input.end());
	if((posState == 10) || (startState == 10)){ results.state = 1; } 
 	else if((posState == 12) || (startState == 12)){ results.state = 2; }
	else if((posState == 13) || (startState == 13)){ results.state = 6; }
	else if((posState == 14) || (startState == 14)){ results.state = 3; } 
	else if((posState == 15) || (startState == 15)){ results.state = 7; }
	else if((posState == 16) || (startState == 16)){ results.state = 4; }
	else if((posState == 17) || (startState == 17)){ results.state = 8; }
	else if((posState == 18) || (startState == 18)){ results.state = 5; }
	else if((posState == 19) || (startState == 19)){ results.state = 9; }
	
	lexertl::lookup(sm, results);	

	while(results.id != eEOF)
	{
		switch(results.id)
		{
			case 1:
		  	token = results.str();
			 break;

			case 2:
		 	token = results.str();
		 	 break;
					
			case 3:
		  	token = results.str();
			 break;

			case 4:
		  	token = results.str();
			 break;
			
			case 5:
		  	token = results.str();
			 break;

			case 6:
		  	token = results.str();
			 break;
			
			case 7:
			token = results.str();
			break;

			case 8:
			token = results.str();
			break;
		
			case 9:
			token = results.str();
			break;
			
			case 10:
			token = results.str();
			break;

			case 11:
			token = results.str();
			break;

			case 12:
		  	token = results.str();
			 break;
			
			case 13:
			token = results.str();
			break;

			case 14:
			token = results.str();
			break;
		
			case 15:
			token = results.str();
			break;
			
			case 16:
			token = results.str();
			break;

			case 17:
			token = results.str();
			break;

			case 18:
			token = results.str();
			break;

			case 19:
			token = results.str();
			break;
	       }

		obj->highlighting(start, input, results, results.id);
		lexertl::lookup(sm, results);
	}
}
