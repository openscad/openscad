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

 Speed Note: setFormat() is very slow, making 'full re-highlight' impractical.
 Thus QT only updates 'blocks' (usually lines).

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

6. action: open example001, put a single '=' after first {
   expected result: error highlight of '=' you added

7. action: open example001, remove first ')'
   expected result: highlight should appear appropriately

8. action: create a large file (50,000 lines). eg at a bash prompt:
     for i in {1..1000}; do cat examples/example001.scad >> test5k.scad ; done
   action: open file in openscad
   expected result: there should not be a slowdown due to highlighting
   action: scroll to bottom, put '=' after last ;
   expected result: there should be a highlight, and a report of syntax error
   action: comment out the highlighter code from mainwin.cc, recompile, put '=' after last ;
   expected result: there should be almost no difference in speed

9. action: open any file, and hold down 'f5' key to repeatedly reparse
   expected result: no crashing!

*/

#include "highlighter.h"
#include <QTextDocument>

#include <iostream>
Highlighter::Highlighter(QTextDocument *parent)
		: QSyntaxHighlighter(parent)
{
	tokentypes["operator"] << "=" << "!" << "&&" << "||" << "+" << "-" << "*" << "/" << "%" << "!" << "#" << ";";
	typeformats["operator"].setForeground(Qt::blue);

	tokentypes["keyword"] << "for" << "intersection_for" << "if" << "assign" << "module" << "function";
	typeformats["keyword"].setForeground(Qt::darkGreen);

	tokentypes["prim3d"] << "cube" << "cylinder" << "sphere" << "polyhedron";
	typeformats["prim3d"].setForeground(Qt::darkBlue);

	tokentypes["prim2d"] << "square" << "polygon" << "circle";
	typeformats["prim2d"].setForeground(Qt::blue);

	tokentypes["transform"] << "scale" << "translate" << "rotate" << "multmatrix" << "color" << "projection";
	typeformats["transform"].setForeground(Qt::darkGreen);

	tokentypes["import"] << "include" << "use" << "import_stl";
	typeformats["import"].setForeground(Qt::darkYellow);

	tokentypes["special"] << "$children" << "child" << "$fn" << "$fa" << "$fb";
	typeformats["special"].setForeground(Qt::darkGreen);

	tokentypes["csgop"]	<< "union" << "intersection" << "difference" << "render";
	typeformats["csgop"].setForeground(Qt::darkGreen);

	tokentypes["extrude"] << "linear_extrude" << "rotate_extrude";
	typeformats["extrude"].setForeground(Qt::darkGreen);

	// for speed - put all tokens into single QHash, mapped to their format
	QList<QString>::iterator ki;
	QList<QString> toktypes = tokentypes.keys();
	for ( ki=toktypes.begin(); ki!=toktypes.end(); ++ki ) {
		QString toktype = *ki;
		QStringList::iterator it;
		for ( it = tokentypes[toktype].begin(); it < tokentypes[toktype].end(); ++it) {
			QString token = *it;
			//std::cout << token.toStdString() << "\n";
			formatMap[ token ] = typeformats [ toktype ];
		}
	}

	quoteFormat.setForeground(Qt::darkMagenta);
	commentFormat.setForeground(Qt::darkCyan);
	errorFormat.setBackground(Qt::red);

	// format tweaks
	formatMap[ "%" ].setFontWeight(QFont::Bold);

	lastErrorBlock = parent->begin();
}

void Highlighter::highlightError(int error_pos)
{
	QTextBlock err_block = document()->findBlock(error_pos);
	//std::cout << "error pos: "  << error_pos << " doc len: " << document()->characterCount() << "\n";
	errorPos = error_pos;
	errorState = true;

	while (err_block.text().remove(QRegExp("\\s+")).size()==0) {
		//std::cout << "special case - errors at end of file w whitespace\n";
		err_block = err_block.previous();
		errorPos = err_block.position()+err_block.length() - 2;
	}
	if ( errorPos == document()->characterCount()-1 ) {
		errorPos--;
	}

	int block_last_pos = err_block.position() + err_block.length() - 1;
	if ( errorPos == block_last_pos ) {
		errorPos--;
		//std::cout << "special case - errors at ends of certain blocks\n";
	}
	err_block = document()->findBlock(errorPos);

	rehighlightBlock( err_block ); // QT 4.6
	errorState = false;
	lastErrorBlock = err_block;
}

void Highlighter::unhighlightLastError()
{
	rehighlightBlock( lastErrorBlock ); // QT 4.6
}

#include <iostream>
void Highlighter::highlightBlock(const QString &text)
{
	int block_first_pos = currentBlock().position();
	int block_last_pos = block_first_pos + currentBlock().length() - 1;
	//std::cout << "block[" << block_first_pos << ":" << block_last_pos << "]"
	//  << ", err:" << errorPos << "," << errorState
	//  << ", text:'" << text.toStdString() << "'\n";

	// Split the block into pieces and highlight each as appropriate
	QString newtext = text;
	QStringList splitHelpers;
	QStringList::iterator sh, token;
	int tokindex = -1; // tokindex helps w duplicate tokens in a single block
	splitHelpers << tokentypes["operator"] << "(" << ")" << "[" << "]";
	for ( sh = splitHelpers.begin(); sh!=splitHelpers.end(); ++sh ) {
		// so "a+b" is treated as "a + b" and formatted
		newtext = newtext.replace( *sh, " " + *sh + " ");
	}
	QStringList tokens = newtext.split(QRegExp("\\s"));
	for ( token = tokens.begin(); token!=tokens.end(); ++token ){
		if ( formatMap.contains( *token ) ) {
			tokindex = text.indexOf( *token, tokindex+1 );
			setFormat( tokindex, token->size(), formatMap[ *token ]);
			// std::cout  << "found tok '" << (*token).toStdString() << "' at " << tokindex << "\n";
		}
	}

	// Quoting and Comments.
	state_e state = (state_e) previousBlockState();
	for (int n = 0; n < text.size(); ++n){
		if (state == NORMAL){
			if (text[n] == '"'){
				state = QUOTE;
				setFormat(n,1,quoteFormat);
			} else if (text[n] == '/'){
				if (text[n+1] == '/'){
					setFormat(n,text.size(),commentFormat);
					break;
				} else if (text[n+1] == '*'){
					setFormat(n++,2,commentFormat);
					state = COMMENT;
				}
			}
		} else if (state == QUOTE){
			setFormat(n,1,quoteFormat);
			if (text[n] == '"' && text[n-1] != '\\')
				state = NORMAL;
		} else if (state == COMMENT){
			setFormat(n,1,commentFormat);
			if (text[n] == '*' && text[n+1] == '/'){
				setFormat(++n,1,commentFormat);
				state = NORMAL;
			}
		}
	}
	setCurrentBlockState((int) state);

	// Highlight an error. Do it last to 'overwrite' other formatting.
	if (errorState) {
		setFormat( errorPos - block_first_pos, 1, errorFormat);
	}

}

