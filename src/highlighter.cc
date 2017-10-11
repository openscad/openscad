/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 Syntax Highlighter for OpenSCAD
 based on Syntax Highlight code by Christopher Olah

 Speed Note:

 setFormat() is very slow. normally this doesnt matter because we
 only highlight a block or two at once. But when OpenSCAD first starts,
 QT automagically calls 'highlightBlock' on every single textblock in the file
 even if it's not visible in the window. On a large file (50,000 lines) this
 can take several seconds.

 Also, QT 4.5 and lower do not have rehighlightBlock(), so they will be slow
 on large files as well, as they re-highlight everything after each compile.

 The vast majority of OpenSCAD files, however, are not 50,000 lines and
 most machines have Qt > 4.5

 See Also:

 Giles Bathgate's Rapcad lexer-based highlighter: rapcad.org

 Test suite:

1. action: open example001, remove first {, hit f5
   expected result: error highlight appears on last }, cursor moves there
   action: replace first {, hit f5
   expected result: error highlight disappears

1a. action: open example001, remove first {, hit f5
   expected result: error highlight appears on last }, cursor moves there
   action: replace first { with the letter 'P', hit f5
   expected result: error highlight on last } disappears, appears on elsewhere
   action: replace first {, hit f5
   expected result: error highlight disappears

2. action: type a=b into any file
   expected result: '=' is highlighted with its appropriate format

2a. action: type a=b=c=d=e=f= into any file
   expected result: each '=' is highlighted with its appropriate format

3. action: open example001, put '===' after first ; hit f5
   expected result: error highlight appears in ===
   action: remove '==='
   expected result: error highlight disappears

3a. action: open example001, put '=' after first ; hit f5
   expected result: error highlight appears
   action: remove '='
   expected result: error highlight disappears

3b. action: open example001, put '=' after first {
   expected result: error highlight appears
   action: remove '='
   expected result: error highlight disappears

3c. action: open example001, replace first { with '='
   expected result: error highlight appears
   action: remove '=', replace with {
   expected result: error highlight disappears

4. action: open example001, remove last ';' but not trailing whitespace/\n
   expected result: error highlight appears somewhere near end
   action: replace last ';'
   expected result: error highlight disappears

5. action: open file, type in a multi-line comment
   expected result: multiline comment should be highlighted appropriately

6. action: open example001, remove first ')'
   expected result: highlight should appear appropriately

7. action: create a large file (50,000 lines). eg at a bash prompt:
     for i in {1..2000}; do cat examples/example001.scad >> test5k.scad ; done
   action: open file in openscad
   expected result: it should load in a reasonable amount of time
   action: scroll to bottom, put '=' after last ;
   expected result: there should be a highlight, and a report of syntax error
    and it should be almost instantaneous.

8. action: open any file, and hold down 'f5' key to repeatedly reparse
   expected result: no crashing!

9. action: for i in examples/ex* ; do ./openscad $i ; done
    expected result: make sure the colors look harmonious

10. action: type random string of [][][][]()()[][90,3904,00,000]
    expected result: all should be highlighted correctly

11. action: type a single slash (/) or slash-star-star (/x**, remove x)
     into a blank document.
    expected result: don't crash esp. on mac

12. action: start openscad, open example 001. open edit/prefs/editor/
     syntax-highlighter, 'for light background'. make your OS use a light
     (white) background for the openscad text editor.
    expected result: text is clearly visible, colors have good contrast

13. repeat test 12, but with 'for dark background' and dark background
    expected result: text is clearly visible, colors have good contrast

14. repeat test 12, but turn the syntax highlighter off.
    expected result: text is clearly visible, single color has good contrast

14. repeat test 13, but turn the syntax highlighter off.
    expected result: text is clearly visible, single color has good contrast

*/

#include "highlighter.h"
#include "Preferences.h"
#include <QTextDocument>
#include <QTextCursor>
#include <QColor>
//#include "printutils.h"

void format_colors_for_light_background(QMap<QString,QTextCharFormat> &formats)
{
	//PRINT("format for light");
	formats["operator"].setForeground(Qt::blue);
	formats["math"].setForeground(QColor("Green"));
	formats["keyword"].setForeground(QColor("Green"));
	formats["keyword"].setToolTip("Keyword");
	formats["transform"].setForeground(QColor("Indigo"));
	formats["csgop"].setForeground(QColor("DarkGreen"));
	formats["prim3d"].setForeground(QColor("DarkBlue"));
	formats["prim2d"].setForeground(QColor("MidnightBlue"));
	formats["import"].setForeground(Qt::darkYellow);
	formats["special"].setForeground(Qt::darkGreen);
	formats["extrude"].setForeground(Qt::darkGreen);
	formats["bracket"].setForeground(QColor("Green"));
	formats["curlies"].setForeground(QColor(32,32,20));
	formats["bool"].setForeground(QColor("DarkRed"));

	formats["_$quote"].setForeground(Qt::darkMagenta);
	formats["_$comment"].setForeground(Qt::darkCyan);
	formats["_$number"].setForeground(QColor("DarkRed"));
}

void format_colors_for_dark_background(QMap<QString,QTextCharFormat> &formats)
{
	//PRINT("format for dark");
	formats["operator"].setForeground(QColor("SkyBlue"));
	formats["math"].setForeground(Qt::green);
	formats["keyword"].setForeground(QColor("LightGreen"));
	formats["keyword"].setToolTip("Keyword");
	formats["transform"].setForeground(QColor("Thistle"));
	formats["csgop"].setForeground(QColor("LightGreen"));
	formats["prim3d"].setForeground(QColor("LightBlue"));
	formats["prim2d"].setForeground(QColor("LightBlue"));
	formats["import"].setForeground(QColor("LightYellow"));
	formats["special"].setForeground(QColor("LightGreen"));
	formats["extrude"].setForeground(QColor("PaleGreen"));
	formats["bracket"].setForeground(QColor("LimeGreen"));
	formats["curlies"].setForeground(QColor("Lavender"));
	formats["bool"].setForeground(QColor("Red"));

	formats["_$quote"].setForeground(Qt::magenta);
	formats["_$comment"].setForeground(Qt::cyan);
	formats["_$number"].setForeground(Qt::red);
}

void Highlighter::assignFormatsToTokens(const QString &s)
{
	//PRINTB("assign fmts %s",s.toStdString());
	if (s=="For Light Background") {
		format_colors_for_light_background(this->typeformats);
	} else if (s=="For Dark Background") {
		format_colors_for_dark_background(this->typeformats);
	} else return;

	// Put each token into single QHash, and map it to it's appropriate
	// qtextchar format (color, bold, etc). For example, '(' is type 
	// 'bracket' so it should get the 'bracket' format from 
	// typeformats[] which is, maybe, Green.

	QList<QString>::iterator ki;
	QList<QString> toktypes = tokentypes.keys();
	for ( ki=toktypes.begin(); ki!=toktypes.end(); ++ki ) {
		QString toktype = *ki;
		QStringList::iterator it;
		for ( it = tokentypes[toktype].begin(); it < tokentypes[toktype].end(); ++it) {
			QString token = *it;
			//PRINTB("set format for %s: type %s", token.toStdString()%toktype.toStdString() );;
			tokenFormats[ token ] = typeformats [ toktype ];
		}
	}
}

Highlighter::Highlighter(QTextDocument *parent)
		: QSyntaxHighlighter(parent)
{
	tokentypes["operator"] << "=" << "!" << "&&" << "||" << "+" << "-" << "*" << "/" << "%" << "!" << "#" << ";";
	tokentypes["math"] << "abs" << "sign" << "acos" << "asin" << "atan" << "atan2" << "sin" << "cos" << "floor" << "round" << "ceil" << "ln" << "log" << "lookup" << "min" << "max" << "pow" << "sqrt" << "exp" << "rands";
	tokentypes["keyword"] << "module" << "function" << "for" << "intersection_for" << "if" << "assign" << "echo"<< "search" << "str" << "let" << "each" << "assert";
	tokentypes["transform"] << "scale" << "translate" << "rotate" << "multmatrix" << "color" << "projection" << "hull" << "resize" << "mirror" << "minkowski";
	tokentypes["csgop"]	<< "union" << "intersection" << "difference" << "render";
	tokentypes["prim3d"] << "cube" << "cylinder" << "sphere" << "polyhedron";
	tokentypes["prim2d"] << "square" << "polygon" << "circle";
	tokentypes["import"] << "include" << "use" << "import_stl" << "import" << "import_dxf" << "dxf_dim" << "dxf_cross" << "surface";
	tokentypes["special"] << "$children" << "child" << "children" << "$fn" << "$fa" << "$fs" << "$t" << "$preview" << "$vpt" << "$vpr" << "$vpd";
	tokentypes["extrude"] << "linear_extrude" << "rotate_extrude";
	tokentypes["bracket"] << "[" << "]" << "(" << ")";
	tokentypes["curlies"] << "{" << "}";
	tokentypes["bool"] << "true" << "false";

	tokentypes["_$comment"] << "_$comment"; // bit of a kludge here
	tokentypes["_$quote"] << "_$quote";
	tokentypes["_$number"] << "_$number";

	errorFormat.setBackground(Qt::red);
	errorState = false;
	errorPos = -1;
	lastErrorBlock = parent->begin();
}

void Highlighter::highlightError(int error_pos)
{
	errorState = true;
	errorPos = error_pos;

	QTextBlock err_block = document()->findBlock( errorPos );
	//PRINTB( "error pos: %i doc len: %i ", error_pos % document()->characterCount() );

	while (err_block.text().remove(QRegExp("\\s+")).size()==0) {
		//PRINT("special case - errors at end of file w whitespace";
		err_block = err_block.previous();
		errorPos = err_block.position()+err_block.length() - 2;
	}
	if ( errorPos == lastDocumentPos()-1 ) {
		errorPos--;
	}

	int block_last_pos = err_block.position() + err_block.length() - 1;
	if ( errorPos == block_last_pos ) {
		//PRINT( "special case - errors at ends of certain blocks");
		errorPos--;
	}
	err_block = document()->findBlock(errorPos);

	portable_rehighlightBlock( err_block );

	errorState = false;
	lastErrorBlock = err_block;
}

void Highlighter::unhighlightLastError()
{
	portable_rehighlightBlock( lastErrorBlock );
}

void Highlighter::portable_rehighlightBlock( const QTextBlock &block )
{
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
	rehighlightBlock( block );
#else
	rehighlight(); // slow on very large files
#endif
}

int Highlighter::lastDocumentPos()
{
#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
	return document()->characterCount();
#else
	QTextBlock lastblock = document()->lastBlock();
	return lastblock.position() + lastblock.length();
#endif
}

void Highlighter::highlightBlock(const QString &text)
{
	int block_first_pos = currentBlock().position();
	//int block_last_pos = block_first_pos + currentBlock().length() - 1;
	//std::cout << "block[" << block_first_pos << ":" << block_last_pos << "]"
	//  << ", err:" << errorPos << "," << errorState
	//  << ", text:'" << text.toStdString() << "'\n";

	// If desired, skip all highlighting .. except for error highlighting.
	if (Preferences::inst()->getValue("editor/syntaxhighlight").toString()==QString("Off")) {
		if (errorState)
			setFormat( errorPos - block_first_pos, 1, errorFormat);
		return;
	}

	// bit of a kludge (for historical convenience)
	QTextCharFormat &quoteFormat = tokenFormats["_$quote"];
	QTextCharFormat &commentFormat = tokenFormats["_$comment"];
	QTextCharFormat &numberFormat = tokenFormats["_$number"];

	// Split the block into chunks (tokens), based on whitespace,
	// and then highlight each token as appropriate
	QString newtext = text;
	QStringList splitHelpers;
	QStringList::iterator sh, token;
	// splitHelpers - so "{[a+b]}" is treated as " { [ a + b ] } "
	splitHelpers << tokentypes["operator"] << tokentypes["bracket"]
	  << tokentypes["curlies"] << ":" << ",";
	for ( sh = splitHelpers.begin(); sh!=splitHelpers.end(); ++sh ) {
		newtext = newtext.replace( *sh, " " + *sh + " ");
	}
	//PRINTB("\nnewtext: %s", newtext.toStdString() );
	QStringList tokens = newtext.split(QRegExp("\\s"));
	int tokindex = 0; // tokindex helps w duplicate tokens in a single block
	bool numtest;
	for ( token = tokens.begin(); token!=tokens.end(); ++token ){
		if ( tokenFormats.contains( *token ) ) {
			tokindex = text.indexOf( *token, tokindex );
			setFormat( tokindex, token->size(), tokenFormats[ *token ]);
			std::string tokprint = (*token).toStdString();
			//PRINTB("found tok '%s' at %i", tokprint % tokindex );
			tokindex += token->size();
		} else {
			(*token).toDouble( &numtest );
			if ( numtest ) {
				tokindex = text.indexOf( *token, tokindex );
				setFormat( tokindex, token->size(), numberFormat );
				std::string tokprint = (*token).toStdString();
				//PRINTB("found num '%s' at %i", tokprint % tokindex );
				tokindex += token->size();
			}
		}
	}

	// Quoting and Comments.
	state_e state = static_cast<state_e>(previousBlockState());
	int quote_esc_state = 0;
	for (int n = 0; n < text.size(); ++n){
		if (state == state_e::NORMAL){
			if (text[n] == '"'){
				state = state_e::QUOTE;
				setFormat(n,1,quoteFormat);
			} else if (text[n] == '/'){
				if ( n+1 < text.size() && text[n+1] == '/'){
					setFormat(n,text.size(),commentFormat);
					break;
				} else if ( n+1 < text.size() && text[n+1] == '*'){
					setFormat(n++,2,commentFormat);
					state = state_e::COMMENT;
				}
			}
		} else if (state == state_e::QUOTE){
			setFormat(n,1,quoteFormat);
			if (quote_esc_state > 0)
				quote_esc_state = 0;
			else if (text[n] == '\\')
				quote_esc_state = 1;
			else if (text[n] == '"')
				state = state_e::NORMAL;
		} else if (state == state_e::COMMENT){
			setFormat(n,1,commentFormat);
			if (text[n] == '*' && n+1 < text.size() && text[n+1] == '/'){
				setFormat(++n,1,commentFormat);
				state = state_e::NORMAL;
			}
		}
	}
	setCurrentBlockState(static_cast<int>(state));

	// Highlight an error. Do it last to 'overwrite' other formatting.
	if (errorState) {
		setFormat( errorPos - block_first_pos, 1, errorFormat);
	}

}

