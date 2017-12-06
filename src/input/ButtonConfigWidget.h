/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2014 Clifford Wolf <clifford@clifford.at> and
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
 
class ButtonConfigWidget : public QWidget
{
public:
	void updateButtonState(int,bool) const;

public slots:
        void on_comboBoxButton0_activated(int val);
        void on_comboBoxButton1_activated(int val);
        void on_comboBoxButton2_activated(int val);
        void on_comboBoxButton3_activated(int val);
        void on_comboBoxButton4_activated(int val);
        void on_comboBoxButton5_activated(int val);
        void on_comboBoxButton6_activated(int val);
        void on_comboBoxButton7_activated(int val);
        void on_comboBoxButton8_activated(int val);
        void on_comboBoxButton9_activated(int val);
        void on_comboBoxButton10_activated(int val);
        void on_comboBoxButton11_activated(int val);
        void on_comboBoxButton12_activated(int val);
        void on_comboBoxButton13_activated(int val);
        void on_comboBoxButton14_activated(int val);
        void on_comboBoxButton15_activated(int val);

signals:
private:
};
