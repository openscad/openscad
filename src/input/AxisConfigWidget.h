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

class AxisConfigWidget : public QWidget
{
public:
	void updateButtonState(int,bool) const;

public slots:
	// Input Driver
        void on_AxisTrim();
        void on_AxisTrimReset();
        void on_comboBoxTranslationX_activated(int val);
        void on_comboBoxTranslationY_activated(int val);
        void on_comboBoxTranslationZ_activated(int val);
        void on_comboBoxTranslationXVPRel_activated(int val);
        void on_comboBoxTranslationYVPRel_activated(int val);
        void on_comboBoxTranslationZVPRel_activated(int val);
        void on_comboBoxRotationX_activated(int val);
        void on_comboBoxRotationY_activated(int val);
        void on_comboBoxRotationZ_activated(int val);
        void on_comboBoxZoom_activated(int val);
        void on_comboBoxZoom2_activated(int val);


	void on_doubleSpinBoxRotateGain_valueChanged(double val);
	void on_doubleSpinBoxTranslationGain_valueChanged(double val);
	void on_doubleSpinBoxTranslationVPRelGain_valueChanged(double val);
	void on_doubleSpinBoxZoomGain_valueChanged(double val);

	void on_doubleSpinBoxDeadzone0_valueChanged(double);
	void on_doubleSpinBoxDeadzone1_valueChanged(double);
	void on_doubleSpinBoxDeadzone2_valueChanged(double);
	void on_doubleSpinBoxDeadzone3_valueChanged(double);
	void on_doubleSpinBoxDeadzone4_valueChanged(double);
	void on_doubleSpinBoxDeadzone5_valueChanged(double);
	void on_doubleSpinBoxDeadzone6_valueChanged(double);
	void on_doubleSpinBoxDeadzone7_valueChanged(double);
	void on_doubleSpinBoxDeadzone8_valueChanged(double);

	void on_doubleSpinBoxTrim0_valueChanged(double);
	void on_doubleSpinBoxTrim1_valueChanged(double);
	void on_doubleSpinBoxTrim2_valueChanged(double);
	void on_doubleSpinBoxTrim3_valueChanged(double);
	void on_doubleSpinBoxTrim4_valueChanged(double);
	void on_doubleSpinBoxTrim5_valueChanged(double);
	void on_doubleSpinBoxTrim6_valueChanged(double);
	void on_doubleSpinBoxTrim7_valueChanged(double);
	void on_doubleSpinBoxTrim8_valueChanged(double);

signals:
private:
};
