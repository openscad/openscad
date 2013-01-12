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

// based on Syntax Highlight code by Chris Olah

/* test suite

1. action: open example001, remove first {, hit f5
   expected result: red highlight appears on last }, cursor moves there
   action: replace first {, hit f5
   expected result: red highlight disappears

2. action: type a=b
   expected result: '=' is highlighted as appropriate

3. action: open example001, put '===' after first ;
   expected result: red highlight appears in ===
   action: remove '==='
   expected result: red highlight disappears

4. action: open example001, remove last ';' but not trailing whitespace/\n
   expected result: red highlight appears on last line
   action: replace last ';'
   expected result: red highlight disappears

5. action: open file, type in a multi-line comment
   expected result: multiline comment should be highlighted appropriately

6. action: open example001, put a single '=' after first {
   expected result: red highlight of '=' you added

*/

#include "highlighter.h"
#include <QTextDocument>

#include <iostream>
Highlighter::Highlighter(QTextDocument *parent)
		: QSyntaxHighlighter(parent)
{
	QMap<QString,QStringList> tokentypes;
	QMap<QString,QTextCharFormat> typeformats;

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

	separators << tokentypes["operator"];
	separators << "(" << ")" << "[" << "]";

	lastErrorBlock = parent->begin();
}

void Highlighter::highlightError(int error_pos)
{
	QTextBlock err_block = document()->findBlock(error_pos);
	std::cout << "error pos: "  << error_pos << " doc len: " << document()->characterCount() << "\n";
	errorPos = error_pos;
	errorState = true;
	//if (errorPos == document()->characterCount()-1) errorPos--;
	rehighlightBlock( err_block ); // QT 4.6
	errorState = false;
	lastErrorBlock = err_block;
}

void Highlighter::unhighlightLastError()
{
	rehighlightBlock( lastErrorBlock );
}

#include <iostream>
void Highlighter::highlightBlock(const QString &text)
{
	std::cout << "block[" << currentBlock().position() << ":"
	  << currentBlock().length() + currentBlock().position() << "]"
	  << ", err:" << errorPos << ", text:'" << text.toStdString() << "'\n";

	// Split the block into pieces and highlight each as appropriate
	QString newtext = text;
	QStringList::iterator sep, token;
	int tokindex = -1; // deals w duplicate tokens in a single block
	for ( sep = separators.begin(); sep!=separators.end(); ++sep ) {
		// so a=b will have '=' highlighted
		newtext = newtext.replace( *sep, " " + *sep + " ");
	}
	QStringList tokens = newtext.split(QRegExp("\\s"));
	for ( token = tokens.begin(); token!=tokens.end(); ++token ){
		if ( formatMap.contains( *token ) ) {
			tokindex = text.indexOf( *token, tokindex+1 );
			// Speed note: setFormat() is the big slowdown in all of this code
			setFormat( tokindex, token->size(), formatMap[ *token ]);
			// std::cout  << "found tok '" << (*token).toStdString() << "' at " << tokindex << "\n";
		}
	}

	// Quoting and Comments.
	// fixme multiline coments dont work
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

	// Highlight an error. Do it last to 'overwrite' other formatting.
	if (errorState) {
		setFormat( errorPos - currentBlock().position() - 1, 1, errorFormat);
	}
}

