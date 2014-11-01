#ifndef Timer_h
#define Timer_h

/**
 * Timer starts measuring the specified time period with start(), and reports on whether
 * the period has expired with isElapsed()
 */
class Timer {
public:
    enum State { RUNNING, STOPPED };
    Timer(unsigned long milliSeconds) { _timingPeriod = milliSeconds; }
    void start(); // Start the timer
    bool isElapsed(); // Check if _timingPeriod elapsed
    enum State state() { return _state; }
    bool isRunning() { return _state == Timer::RUNNING; }

private:
    enum State _state = Timer::STOPPED;
    unsigned long _timingPeriod;
    unsigned long _triggerTime = 0; // millis() value when the timer should go off
};

/**
 * Start/re-start the timer
 */
void Timer::start()
{
	_state = Timer::RUNNING;
	_triggerTime = millis() + _timingPeriod; // Unsigned math, so overflow expected
}

/**
 * Checks if the specified period has elapsed. If so, returns true and turns off the timer
 */
bool Timer::isElapsed() {
	bool elapsed = true;

    if (_state == Timer::RUNNING) {
    	elapsed = (long)( millis() - _triggerTime ) >= 0; // Signed math, so overflows become negative
    }

    if ( elapsed ) {
    	_state = Timer::STOPPED;
    }

    return elapsed;
}


#endif
