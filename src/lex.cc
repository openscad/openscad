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

	rules_.push("INITIAL", "#", 12, "MODIFIER");
	rules_.push("MODIFIER","[^;\\{]+",12,  ".");
	rules_.push("MODIFIER","\\{", 13, "BLOCK");
	rules_.push("BLOCK", "[^\\}]+",13,".");	
	rules_.push("BLOCK", "\\}", 13, "INITIAL");
	rules_.push("MODIFIER", ";", 12, "INITIAL");

	rules_.push("INITIAL", "!", 14,"MODIFIER2");
	rules_.push("MODIFIER2","[^;\\{]+",14, ".");
	rules_.push("MODIFIER2","[\\{]", 15, "BLOCK2");
	rules_.push("BLOCK2", "[^\\}]+",15, ".");	
	rules_.push("BLOCK2", "[\\}]", 15, "INITIAL");
	rules_.push("MODIFIER2", ";", 14, "INITIAL");

	rules_.push("INITIAL", "[*]", 16,"MODIFIER3");
	rules_.push("MODIFIER3","[^;\\{]+", 16,"MODIFIER3");
	rules_.push("MODIFIER3","[\\{]", 17,"BLOCK3");
	rules_.push("BLOCK3", "[^\\}]+" , 17,"BLOCK3");	
	rules_.push("BLOCK3", "[\\}]", 17, "INITIAL");
	rules_.push("MODIFIER3", ";", 16, "INITIAL");

	rules_.push("INITIAL", "%", 18,"MODIFIER4");
	rules_.push("MODIFIER4","[^;\\{]+", 18,"MODIFIER4");
	rules_.push("MODIFIER4","[\\{]", 19,"BLOCK4");
	rules_.push("BLOCK4", "[^\\}]+" , 19,"BLOCK4");	
	rules_.push("BLOCK4", "[\\}]", 19, "INITIAL");
	rules_.push("MODIFIER4", ";", 18, "INITIAL");

	rules_.push("INITIAL", "\"/*\"",  ecomment, "COMMENT");
	rules_.push("COMMENT", "[^*]+|.", ecomment,  "COMMENT");
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

void Lex::lex_results(const std::string& input, int start, LexInterface* const obj){

	lexertl::smatch results (input.begin(), input.end());

	int isstyle = obj->getStyleAt(start-1);
	switch(isstyle)
	{
		case 10:
		 results.state = 1;
		break;

		case 14:
		 results.state = 3;
		break;

		case 15:
		 results.state = 7;
		break;

		case 12:
		 results.state = 2;
		break;

		case 13:
		 results.state = 6;
		break;
	
		case 16:
		 results.state = 4;
		break;

		case 17:
		 results.state = 8;
		break;

		case 18:
		 results.state = 5;
		break;

		case 19:
		 results.state = 9;
		break;

	}
	lexertl::lookup(sm, results);	

	while(results.id != eEOF)
	{
		obj->highlighting(start, input, results);
		lexertl::lookup(sm, results);
	}
}
