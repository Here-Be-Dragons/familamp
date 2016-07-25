//============================================================
//	Touch Sensing Demo on Spark Core
//
//============================================================
//	Copyright (c) 2014 Tangibit Studios LLC.  All rights reserved.
//  Copyright (c) 2014 David Greaves <david@dgreaves.com>
//
//	This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//	License as published by the Free Software Foundation, either
//	version 3 of the License, or (at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//	Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public
//	License along with this program; if not, see <http://www.gnu.org/licenses/>.
//
//============================================================

#ifndef CAPTOUCH_H
#define CAPTOUCH_H
// Polling times in [ms]
#define CAPTOUCH_POLL_TIME 20

#define CAPTOUCH_TOUCH_POOL 100
#define CAPTOUCH_JITTER_POOL 40


// Useful debugging output
#define CAPTOUCH_DEBUG 1

class CapTouch
{
    public:
        explicit CapTouch(int sensorPin, int driverPin);
        
        // Touch events
        enum Event {
            NoEvent,
            TouchEvent,
            ReleaseEvent
        };
        enum State {
			Touched,
            NotTouched,
        };
        
        void setup();
        void setPoll(int pollinterval) { m_pollTime = pollinterval; }

		// In case the interrupts need to be disabled
        void detachIntr();
        void attachIntr();
        bool intrIsAttached() { return m_intrIsAttached; }

		// Get transition information
        Event getEvent();
		// Get state information
//        State getState();
        
    protected:
		// Currently this is static until we can pass a lambda to the
		// attachInterrupt() call
		void touchSense();
        
        long touchSampling();
        Event touchEventCheck();
        
        bool m_intrIsAttached;
        int m_pollTime;

        // touch sensor pins
        int m_driverPin;
        int m_sensorPin;
        
        // reading and baseline
        long m_tReading;
        long m_tBaseline;
        long m_tBaselineSum;
        long m_tLastDelay;
        long m_tJitterSum;
        long m_tJitter;
        
        // timestamps
        unsigned long m_tS;
        unsigned long m_lastUpdate;
        // Debounce
        unsigned long m_touchSenseLast;
        unsigned long m_touchDebounceTimeLast;
        unsigned long m_touchNow;  // current debounced state
        unsigned long m_touchLast; // last debounced state

};
#endif // CAPTOUCH_H
