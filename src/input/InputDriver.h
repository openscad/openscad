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

#include <QEvent>
#include <QThread>

class InputEventHandler
{
public:

    virtual ~InputEventHandler(void) { };

    virtual void onAxisChanged(class InputEventAxisChanged *event) = 0;
    virtual void onButtonChanged(class InputEventButtonChanged *event) = 0;

    virtual void onTranslateEvent(class InputEventTranslate *event) = 0;
    virtual void onRotateEvent(class InputEventRotate *event) = 0;
    virtual void onActionEvent(class InputEventAction *event) = 0;
    virtual void onZoomEvent(class InputEventZoom *event) = 0;
};

class InputEvent : public QEvent
{
public:
    const bool activeOnly;

    InputEvent(const bool activeOnly = true);

    virtual ~InputEvent(void);

    virtual void deliver(InputEventHandler *receiver) = 0;

    static const QEvent::Type eventType;
};

class GenericInputEvent : public InputEvent
{
public:
    GenericInputEvent(const bool activeOnly = true) : InputEvent(activeOnly) { }
    ~GenericInputEvent() { }
};

/**
 * Generic event for use by input drivers to report the change of
 * one axis. The value is assumed to be an absolute value in the
 * range -1.0 to 1.0.
 */
class InputEventAxisChanged : public GenericInputEvent
{
public:
    const unsigned int axis;
    const double value;
    
    InputEventAxisChanged(const unsigned int axis, const double value, const bool activeOnly = true) : GenericInputEvent(activeOnly), axis(axis), value(value) { }

    void deliver(InputEventHandler *receiver)
    {
        receiver->onAxisChanged(this);
    }
};

/**
 * Generic event for use by input drivers to report button press
 * and button release events.
 */
class InputEventButtonChanged : public GenericInputEvent
{
public:
    const unsigned int button;
    const bool down;

    InputEventButtonChanged(const unsigned int button, const bool down, const bool activeOnly = true) : GenericInputEvent(activeOnly), button(button), down(down) { }

    void deliver(InputEventHandler *receiver)
    {
        receiver->onButtonChanged(this);
    }
};

/**
 * Event to trigger view translation. In general this should be used
 * only by the event mapper to ensure the action can be configured by
 * the user. In special cases it is still possible to create this event
 * type in the driver itself (e.g. the DBus driver uses this to report
 * calls to the translate method).
 */
class InputEventTranslate : public InputEvent
{
public:
    const double x;
    const double y;
    const double z;
    const bool relative;

    InputEventTranslate(const double x, const double y, const double z, const bool relative = true, const bool activeOnly = true) : InputEvent(activeOnly), x(x), y(y), z(z), relative(relative) { }

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

    InputEventRotate(const double x, const double y, const double z, const bool relative = true, const bool activeOnly = true) : InputEvent(activeOnly), x(x), y(y), z(z), relative(relative) { }

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

    InputEventZoom(const double zoom, const bool relative = true, const bool activeOnly = true) : InputEvent(activeOnly), zoom(zoom), relative(relative) { }

    void deliver(InputEventHandler *receiver)
    {
        receiver->onZoomEvent(this);
    }
};

class InputEventAction : public InputEvent
{
public:
    const std::string action;

    InputEventAction(const std::string action, const bool activeOnly = true) : InputEvent(activeOnly), action(action) { }

    void deliver(InputEventHandler *receiver)
    {
        receiver->onActionEvent(this);
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

    /*
     * Return if the driver is currently opened. The default implementation
     * simply returns the {@link #isRunning()} status of the thread.
     */
    virtual bool isOpen();

    /*
     * Drivers that are not connected to a device and can be opened on
     * application start. No attempt to re-open is made.
     */
    virtual bool openOnce();
};
