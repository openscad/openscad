/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2015 Clifford Wolf <clifford@clifford.at> and
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
#pragma once

#include <stdint.h>

#include <QEvent>
#include <QThread>

class InputEventHandler
{
public:

    virtual ~InputEventHandler(void) { };

    virtual void onTranslateEvent(const class InputEventTranslate *event) = 0;
    virtual void onRotateEvent(const class InputEventRotate *event) = 0;
    virtual void onButtonEvent(const class InputEventButton *event) = 0;
    virtual void onZoomEvent(const class InputEventZoom *event) = 0;
};

class InputEvent : public QEvent
{
public:

    InputEvent(void) : QEvent(eventType) { };

    virtual ~InputEvent(void) { };

    virtual void deliver(InputEventHandler *receiver) = 0;

    static const QEvent::Type eventType;
};

class InputEventTranslate : public InputEvent
{
public:
    const double x;
    const double y;
    const double z;
    const bool relative;

    InputEventTranslate(double x, double y, double z, bool relative = true) : x(x), y(y), z(z), relative(relative) { }

    void deliver(InputEventHandler *receiver)
    {
        receiver->onTranslateEvent(this);
    }
};

class InputEventRotate : public InputEvent
{
public:
    const double x;
    const double y;
    const double z;
    const bool relative;

    InputEventRotate(double x, double y, double z, bool relative = true) : x(x), y(y), z(z), relative(relative) { }

    void deliver(InputEventHandler *receiver)
    {
        receiver->onRotateEvent(this);
    }
};

class InputEventZoom : public InputEvent
{
public:
    const double zoom;
    const bool relative;

    InputEventZoom(double zoom, bool relative = true) : zoom(zoom), relative(relative) { }

    void deliver(InputEventHandler *receiver)
    {
        receiver->onZoomEvent(this);
    }
};

class InputEventButton : public InputEvent
{
public:
    const int idx;
    const bool down;

    InputEventButton(int idx, bool down) : idx(idx), down(down) { }

    void deliver(InputEventHandler *receiver)
    {
        receiver->onButtonEvent(this);
    }
};

class InputDriver : public QThread
{
public:
    InputDriver();
    virtual ~InputDriver();

    virtual const std::string & get_name() const = 0;

    virtual bool open() = 0;
    virtual void close() = 0;
};
