#include "gui/ScadLexer.h"

#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>
#include <iterator>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>


#if !ENABLE_LEXERTL

ScadLexer::ScadLexer(QObject *parent) : QsciLexerCPP(parent)
{
  // -> Style: Keyword (lexer.l)
  keywordSet[0] =
    "if else let for each module function true false undef "
    "include use assert";

  // -> Style: KeywordSet2 (func.cc)
  keywordSet[1] =
    "abs sign rands min max sin cos asin acos tan atan atan2 "
    "round ceil floor pow sqrt exp len log ln str chr ord concat "
    "is_undef is_list is_num is_bool is_string is_function "
    "lookup search version version_num norm cross parent_module "
    "dxf_dim dxf_cross";

  // -> used in comments only like /*! \cube */
  keywordSet[2] =
    "struct union enum fn var def typedef file namespace package "
    "interface param see return class brief";

  // -> Style: GlobalClass
  keywordSet[3] =
    "cube sphere cylinder polyhedron square circle polygon text "
    "minkowski hull resize child children echo union difference "
    "intersection linear_extrude rotate_extrude import group "
    "projection render surface scale rotate mirror translate "
    "multmatrix color offset intersection_for roof fill";

  setFoldComments(true);
  setFoldAtElse(true);
}

const char *ScadLexer::language() const
{
  return "SCAD";
}

void ScadLexer::setKeywords(int set, const std::string& keywords)
{
  if ((set < 1) || (set > 4)) {
    return;
  }

  std::string trimmedKeywords(keywords);
  boost::algorithm::trim(trimmedKeywords);
  if (trimmedKeywords.empty()) {
    return;
  }

  keywordSet[set - 1] = trimmedKeywords;
}

const char *ScadLexer::keywords(int set) const
{
  if ((set < 1) || (set > 4)) {
    return nullptr;
  }
  return keywordSet[set - 1].c_str();
}

QStringList ScadLexer::autoCompletionWordSeparators() const
{
  QStringList wl;
  wl << "."; // dot notation, not used yet, but preparation for object support
  wl << "<"; // for include/use auto complete
  wl << "/"; // for include/use directory auto complete
  return wl;
}

#endif // if !ENABLE_LEXERTL

/***************************************************************/
/***************************************************************/
/***************************************************************/

#if ENABLE_LEXERTL

#include <Qsci/qscilexercustom.h>
#include <Qsci/qsciscintilla.h>

#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"

//#define DEBUG_LEXERTL   1

#if DEBUG_LEXERTL
#include "lexertl/debug.hpp"
#include <fstream>
#include <iostream>
#endif

/// See original attempt at https://github.com/openscad/openscad/tree/lexertl/src

void Lex::default_rules()
{
  rules_.push_state("PATH");
  rules_.push_state("COMMENT");

  std::string keywords("module function echo import projection render "
                       "return if else let for each assert");
  defineRules(keywords, ekeyword);

  //include and use have a unique syntax
  rules_.push("INITIAL", "use", ekeyword, "PATH");
  rules_.push("INITIAL", "include", ekeyword, "PATH");
  rules_.push("PATH", ".|\n", etext, "INITIAL"); //leave this state; "use" and "include" can also be used as variable names
  rules_.push("PATH", "[ \t\r\n]*<[^>]*>", eQuotedString, "INITIAL");

  std::string transformations("translate rotate scale linear_extrude "
                              "rotate_extrude resize mirror multmatrix color "
                              "offset hull minkowski children");
  defineRules(transformations, etransformation);

  std::string booleans("union difference intersection intersection_for");
  defineRules(booleans, eboolean);

  std::string functions("abs sign rands min max sin cos asin acos tan atan atan2 round "
                        "ceil floor pow sqrt exp len log ln str chr ord concat lookup "
                        "search version version_num norm cross parent_module dxf_dim "
                        "dxf_cross is_undef is_list is_num is_bool is_string "
                        "is_function is_object");
  defineRules(functions, efunction);

  std::string models("sphere cube cylinder polyhedron square polygon text circle surface roof");
  defineRules(models, emodel);

  // Operators and Modifier Characters
  std::string operators(R"(\+ - \* \/ % \^ < <= >= == != >= > && \|\| ! = #)");
  defineRules(operators, eoperator);

  rules_.push(R"(["](([\\]["])|[^"])*["])", eQuotedString);

  std::string values("true false undef PI");
  defineRules(values, enumber);
  rules_.push("([-+]?((([0-9]+[.]?|([0-9]*[.][0-9]+))([eE][-+]?[0-9]+)?)))", enumber);

  // comments and variables come later, after any custom keywords are added
}

void Lex::defineRules(const std::string& keyword_list, int id)
{
  std::string trimmedKeywords(keyword_list);
  boost::algorithm::trim(trimmedKeywords);
  if (trimmedKeywords.empty()) return;

  std::vector<std::string> words;
  boost::split(words, trimmedKeywords, boost::is_any_of(" "));
  for (const auto& keyword : words) {
    rules_.push(keyword, id);
  }
}

// default and custom rules must be set before this
void Lex::finalize_rules()
{
  // These need to come after keywords, so they don't accidentally match.
  // Sadly, order of definition matters, as well as enum.
  rules_.push("[a-zA-Z0-9_]+", evariable);
  rules_.push("[$][a-zA-Z0-9_]+", especialVariable);

  rules_.push("INITIAL", "\"/*\"",  ecomment, "COMMENT");
  rules_.push("COMMENT", "[^*]+|.", ecomment,  "COMMENT");
  rules_.push("COMMENT", "\"*/\"", ecomment, "INITIAL");
  rules_.push("[/][/].*$", ecomment);

  rules_.push(".|\n", etext);

  // build our lexer
  lexertl::generator::build(rules_, sm);

#if DEBUG_LEXERTL
  std::ofstream fout("file1.txt", std::fstream::trunc);
  lexertl::debug::dump(sm, fout);
#endif
}

void Lex::lex_results(const std::string& input, int start, LexInterface *const obj)
{
#if DEBUG_LEXERTL
  std::cout << "called lexer" << std::endl;
#endif
  lexertl::smatch results(input.begin(), input.end());

  //The editor can ask to only lex from a starting point.
  //This can be faster the lexing the whole text,
  //but requires the lexer to try to restore the lexer state.
  //We currently handle comments (COMMENT State) pretty well.
  //We currently do not handle include/use (PATH State).
  int isstyle = obj->getStyleAt(start - 1);
  if (isstyle == ecomment) results.state = rules_.state("COMMENT");

  lexertl::lookup(sm, results);
  while (results.id != eEOF) {
    obj->highlighting(start, input, results);
    lexertl::lookup(sm, results);
  }
}

/***************************************************************/

ScadLexer2::ScadLexer2(QObject *parent) : QsciLexerCustom(parent), LexInterface()
{
  my_lexer = new Lex();
  my_lexer->default_rules();
}

ScadLexer2::~ScadLexer2()
{
  delete my_lexer;
}

void ScadLexer2::styleText(int start, int end)
{
#if DEBUG_LEXERTL
  std::cout << "start: " << start << std::endl;
#endif
  if (!editor()) return;

  char *data = new char[end - start + 1];
  editor()->SendScintilla(QsciScintilla::SCI_GETTEXTRANGE, start, end, data);
  QString source(data);
  const std::string input(source.toStdString());

#if DEBUG_LEXERTL
  auto pos = editor()->SendScintilla(QsciScintilla::SCI_GETCURRENTPOS);
  std::cout << "its being called" << std::endl;
#endif

  my_lexer->lex_results(input, start, this);
  this->fold(start, end);

  delete [] data;
  if (source.isEmpty()) return;
}

void ScadLexer2::autoScroll(int error_pos)
{
  editor()->SendScintilla(QsciScintilla::SCI_GOTOPOS, error_pos);
  editor()->SendScintilla(QsciScintilla::SCI_SCROLLCARET);
}

void ScadLexer2::fold(int start, int end)
{
  char chNext = editor()->SendScintilla(QsciScintilla::SCI_GETCHARAT, start);
  int lineCurrent = editor()->SendScintilla(QsciScintilla::SCI_LINEFROMPOSITION, start);
  int levelPrev = editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent) & QsciScintilla::SC_FOLDLEVELNUMBERMASK;
  int levelCurrent = levelPrev;
  for (int i = start; i < end; i++) {
    char ch = chNext;
    chNext = editor()->SendScintilla(QsciScintilla::SCI_GETCHARAT, i + 1);

    bool atEOL = ((ch == '\r' && chNext != '\n') || (ch == '\n'));

    int prevStyle = editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, i - 1);
    int currStyle = editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, i);

    bool currStyleIsOtherText = (currStyle == OtherText);
    if (currStyleIsOtherText) {
      if ((ch == '{') || (ch == '[') ) {
        levelCurrent++;
      } else if ((ch == '}') || (ch == ']') ) {
        levelCurrent--;
      }
    }


    bool prevStyleIsComment = (prevStyle == Comment);
    bool currStyleIsComment = (currStyle == Comment);
    bool isStartOfComment = (!prevStyleIsComment) && (currStyleIsComment);
    bool isEndOfComment = (prevStyleIsComment) && (!currStyleIsComment);

    if (isStartOfComment) {
      levelCurrent++;
    }
    if (isEndOfComment) {
      levelCurrent--;
    }

    if (atEOL || (i == (end - 1))) {
      int lev = levelPrev;

      if (levelCurrent > levelPrev) {
        lev |= QsciScintilla::SC_FOLDLEVELHEADERFLAG;
      }

      if (lev != editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent)) {
        editor()->SendScintilla(QsciScintilla::SCI_SETFOLDLEVEL, lineCurrent, lev);
      }

      lineCurrent++;
      levelPrev = levelCurrent;
    }
  }

  int flagsNext = editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent) & QsciScintilla::SC_FOLDLEVELNUMBERMASK;
  editor()->SendScintilla(QsciScintilla::SCI_SETFOLDLEVEL, lineCurrent, levelPrev | flagsNext);
}


int ScadLexer2::getStyleAt(int pos)
{
  int sstyle = editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, pos);
  return sstyle;
}

void ScadLexer2::highlighting(int start, const std::string& input, lexertl::smatch results)
{
  std::string token = results.str();
  int style = results.id;

#if DEBUG_LEXERTL
  QString glyphs = QString::fromStdString(token);
  std::cout << "highlighting ( " << style << " ):" << token << " [ " << token.length() << " bytes, " << glyphs.length() << " glyphs ]" << std::endl;
#endif

  startStyling(start + std::distance(input.begin(), results.first));
  setStyling(token.length(), style);
}

QColor ScadLexer2::defaultColor(int style) const
{
  switch (style) {
  case Keyword:
    return Qt::blue;
  case Comment:
    return Qt::green;
  case Number:
    return Qt::red;
  case Transformation:
    return "#f32222";
  case Boolean:
    return "#22f322";
  case Function:
    return "#2222f3";
  case Model:
    return Qt::blue;
  case Default:
    return Qt::black;
  }
  return Qt::black;
}

QString ScadLexer2::description(int style) const
{
  switch (style) {
  case Default:
    return "Default";
  case Keyword:
    return "Keyword";
  case Transformation:
    return "Transformation";
  case Boolean:
    return "Boolean";
  case Function:
    return "Function";
  case Model:
    return "Model";
  case Operator:
    return "Operator";
  case String:
    return "String";
  case Number:
    return "Number";
  case Custom1:
    return "Custom1";
  case Custom2:
    return "Custom2";
  case Custom3:
    return "Custom3";
  case Custom4:
    return "Custom4";
  case Custom5:
    return "Custom5";
  case Custom6:
    return "Custom6";
  case Custom7:
    return "Custom7";
  case Custom8:
    return "Custom8";
  case Custom9:
    return "Custom9";
  case Custom10:
    return "Custom10";
  case Variable:
    return "Variable";
  case SpecialVariable:
    return "SpecialVariable";
  case Comment:
    return "Comment";
  }
  return {QString::number(style)};
}

const char *ScadLexer2::language() const
{
  return "SCAD";
}

QStringList ScadLexer2::autoCompletionWordSeparators() const
{
  QStringList wl;
  wl << "."; // dot notation, not used yet, but preparation for object support
  wl << "<"; // for include/use auto complete
  wl << "/"; // for include/use directory auto complete
  return wl;
}

#endif // if ENABLE_LEXERTL
