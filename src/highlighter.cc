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

#include "highlighter.h"
#include "openscad.h" // extern int parser_error_pos;

#ifdef _QCODE_EDIT_
Highlighter::Highlighter(QDocument *parent)
#else
Highlighter::Highlighter(QTextDocument *parent)
#endif
		: QSyntaxHighlighter(parent)
{
	operators << "!" << "&&" << "||" << "+" << "-" << "*" << "/" << "%" << "!" << "#" << ";";
	KeyWords << "for" << "intersection_for" << "if" << "assign" 
	         << "module" << "function"
		 << "$children" << "child" << "$fn" << "$fa" << "$fb"     // Lump special variables in here
	         << "union" << "intersection" << "difference" << "render"; //Lump CSG in here
	Primitives3D << "cube" << "cylinder" << "sphere" << "polyhedron";
	Primitives2D << "square" << "polygon" << "circle";
	Transforms << "scale" << "translate" << "rotate" << "multmatrix" << "color"
	           << "linear_extrude" << "rotate_extrude"; // Lump extrudes in here.
	Imports << "include" << "use" << "import_stl";

	//this->OperatorStyle.setForeground
	KeyWordStyle.setForeground(Qt::darkGreen);
	TransformStyle.setForeground(Qt::darkGreen);
	PrimitiveStyle3D.setForeground(Qt::darkBlue);
	PrimitiveStyle2D.setForeground(Qt::blue);
	ImportStyle.setForeground(Qt::darkYellow);
	QuoteStyle.setForeground(Qt::darkMagenta);
	CommentStyle.setForeground(Qt::darkCyan);
}


void Highlighter::highlightBlock(const QString &text)
{
	state_e state = (state_e) previousBlockState();

	//Key words and Primitives
	QStringList::iterator it;
	for (it = KeyWords.begin(); it != KeyWords.end(); ++it){
		for (int i = 0; i < text.count(*it); ++i){
			setFormat(text.indexOf(*it), it->size(), KeyWordStyle);
		}
	}
	for (it = Primitives3D.begin(); it != Primitives3D.end(); ++it){
		for (int i = 0; i < text.count(*it); ++i){
			setFormat(text.indexOf(*it), it->size(), PrimitiveStyle3D);
		}
	}
	for (it = Primitives2D.begin(); it != Primitives2D.end(); ++it){
		for (int i = 0; i < text.count(*it); ++i){
			setFormat(text.indexOf(*it), it->size(), PrimitiveStyle2D);
		}
	}
	for (it = Transforms.begin(); it != Transforms.end(); ++it){
		for (int i = 0; i < text.count(*it); ++i){
			setFormat(text.indexOf(*it), it->size(), TransformStyle);
		}
	}
	for (it = Imports.begin(); it != Imports.end(); ++it){
		for (int i = 0; i < text.count(*it); ++i){
			setFormat(text.indexOf(*it), it->size(), ImportStyle);
		}
	}


	// Quoting and Comments.
	for (int n = 0; n < text.size(); ++n){
		if (state == NORMAL){
			if (text[n] == '"'){
				state = QUOTE;
				setFormat(n,1,QuoteStyle);
			} else if (text[n] == '/'){
				if (text[n+1] == '/'){
					setFormat(n,text.size(),CommentStyle);
					break;
				} else if (text[n+1] == '*'){
					setFormat(n++,2,CommentStyle);
					state = COMMENT;
				}
			}
		} else if (state == QUOTE){
			setFormat(n,1,QuoteStyle);
			if (text[n] == '"' && text[n-1] != '\\')
				state = NORMAL;
		} else if (state == COMMENT){
			setFormat(n,1,CommentStyle);
			if (text[n] == '*' && text[n+1] == '/'){
				setFormat(++n,1,CommentStyle);
				state = NORMAL;
			}
		}
	}

	//Save State
	setCurrentBlockState((int) state);

}

