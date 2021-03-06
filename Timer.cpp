/*
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

/*  * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Code by Simon Monk
 http://www.simonmonk.org
* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <Arduino.h>
#include <Timer.h>

 // The 'events' are statically allocated in .bss only if the constructor is called without parameters
Timer::Timer(void)
{
	static Event events[DEFAULT_NUMBER_OF_EVENTS];
	_numberOfEvents = DEFAULT_NUMBER_OF_EVENTS;
	_events = events;
	memset(_events, EVENT_NONE, DEFAULT_NUMBER_OF_EVENTS * sizeof(Event));
}

 // The 'events' are dynamically allocated from the heap if the constructor is called with a parameter
Timer::Timer(byte numberOfEvents)
{
	_numberOfEvents = numberOfEvents;
	_events = (Event*)malloc(_numberOfEvents * sizeof(Event));
	memset(_events, EVENT_NONE, _numberOfEvents * sizeof(Event));
}

// If a timer is ever disposed, the dynamically allocated memory for 'events' should be free'd
Timer::~Timer(void)
{
	if ( _numberOfEvents != DEFAULT_NUMBER_OF_EVENTS ) {
		free(_events);
	}
}

int8_t Timer::every(unsigned long period, void (*callback)(void*), int repeatCount, void* context)
{
    int8_t i = findFreeEventIndex();
    if (i == NO_TIMER_AVAILABLE) return NO_TIMER_AVAILABLE;

    _events[i].eventType = EVENT_EVERY;
    _events[i].period = period;
    _events[i].repeatCount = repeatCount;
    _events[i].callback = callback;
    _events[i].lastEventTime = millis();
    _events[i].count = 0;
    _events[i].context = context;
    return i;
}

int8_t Timer::every(unsigned long period, void (*callback)(void*), void* context)
{
    return every(period, callback, -1, context); // - means forever
}

int8_t Timer::after(unsigned long period, void (*callback)(void*), void* context)
{
    return every(period, callback, 1, context);
}

int8_t Timer::oscillate(uint8_t pin, unsigned long period, uint8_t startingValue, int repeatCount)
{
    int8_t i = findFreeEventIndex();
    if (i == NO_TIMER_AVAILABLE) return NO_TIMER_AVAILABLE;

    _events[i].eventType = EVENT_OSCILLATE;
    _events[i].pin = pin;
    _events[i].period = period;
    _events[i].pinState = startingValue;
    digitalWrite(pin, startingValue);
    _events[i].repeatCount = repeatCount * 2; // full cycles not transitions
    _events[i].lastEventTime = millis();
    _events[i].count = 0;
    _events[i].context = (void*)0;
    _events[i].callback = (void (*)(void*))0;
    return i;
}

int8_t Timer::oscillate(uint8_t pin, unsigned long period, uint8_t startingValue)
{
    return oscillate(pin, period, startingValue, -1); // forever
}

/**
 * This method will generate a pulse of !startingValue, occuring period after the
 * call of this method and lasting for period. The Pin will be left in !startingValue.
 */
int8_t Timer::pulse(uint8_t pin, unsigned long period, uint8_t startingValue)
{
    return oscillate(pin, period, startingValue, 1); // once
}

/**
 * This method will generate a pulse of startingValue, starting immediately and of
 * length period. The pin will be left in the !startingValue state
 */
int8_t Timer::pulseImmediate(uint8_t pin, unsigned long period, uint8_t pulseValue)
{
    int8_t id(oscillate(pin, period, pulseValue, 1));
    // now fix the repeat count
    if (id >= 0 && id < _numberOfEvents) {
        _events[id].repeatCount = 1;
    }
    return id;
}

int8_t Timer::stop(int8_t id)
{
    if (id >= 0 && id < _numberOfEvents) {
        _events[id].eventType = EVENT_NONE;
        return TIMER_NOT_AN_EVENT;
    }
    return id;
}

void Timer::update(void)
{
    for (int8_t i = 0; i < _numberOfEvents; i++)
    {
        if (_events[i].eventType != EVENT_NONE)
        {
            _events[i].update();
        }
    }
}

int8_t Timer::findFreeEventIndex(void)
{
    for (int8_t i = 0; i < _numberOfEvents; i++)
    {
        if (_events[i].eventType == EVENT_NONE)
        {
            return i;
        }
    }
    return NO_TIMER_AVAILABLE;
}
