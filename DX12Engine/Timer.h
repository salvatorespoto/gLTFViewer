#pragma once

/**
 * Timer class is used to compute time intervals
 */
class Timer
{
public:

	/**
	 * Initialize the timer: set to start time to the initialization time.
	 * The timer is in pause state after the creation.
	 */
	Timer();

	/** Reset the timer: set to start time to the reset time */
	void reset();

	/** Return the seconds elapsed from the last call to tick() */
	double tick();

	/** Stop (pause) the timer */
	void stop();

	/** Start the the timer */
	void start();

	/** Return the seconds elapsed from the call to the constructor or the last call to reset() */
	double total();

private:
	__int64  m_countsForSecond;	// Timer frequency: how many counts per seconds
	double m_secondsForCount;	// Inverse of frequency: how much is long a count in seconds, 1 / m_countsForSecond
	__int64 m_startTime;		/*< The time of the last call to reset(), in counts */
	__int64 m_currentTime;		/*< The current time, in counts */
	__int64 m_lastTime;			/*< The time of the last call to tick(), in counts */
	double m_deltaTime;			/*< The difference currentTime - lastTime, in seconds */
	double m_totalTime;			/*< The difference currentTime - starTime, in seconds */
	double m_stopTime;			/*< The time when the timer has been stopper the last time, in counts */
	double m_totalStopTime;		/*< The the total time the timer has been stopper from the last call to reset, in seconds */
	bool m_stopped;				/*< The timer has been stopped */
};

