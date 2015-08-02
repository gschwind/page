/*
 * time.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * Time in 64 bits, it relative time from some point in time troncated to 64 bits in nano second.
 *
 * can last for more than 200 years before warp.
 *
 */

#ifndef TIME_HXX_
#define TIME_HXX_

#include <time.h>

namespace page {

class time64_t {
	int64_t nsec;

public:

	static time64_t now() {
		time64_t ret;
		ret.update_to_current_time();
		return ret;
	}

	/* create new time in current time */
	time64_t() : nsec(0) { }
	time64_t(time64_t const & t) : nsec(t.nsec) { }

	time64_t(double sec) : nsec{static_cast<int64_t>(sec * 1000000000.0)} { }

	/* convert from int64_t */
	time64_t(int64_t nsec) : nsec(nsec) { }

	time64_t(long sec, long nsec) {
		this->nsec = static_cast<int64_t>(sec) * static_cast<int64_t>(1000000000L) + static_cast<int64_t>(nsec);
	}

	operator timespec() {
		timespec t;
		t.tv_nsec = nsec % 1000000000L;
		t.tv_sec = nsec / 1000000000L;
		return t;
	}

	/**
	 * Caution: limited to 54 bits while nsec store 64 bits
	 */
	explicit operator double() const {
		return static_cast<double>(nsec);
	}

	operator int64_t() const {
		return nsec;
	}

	void update_to_current_time() {
		timespec cur_tic;
		clock_gettime(CLOCK_MONOTONIC, &cur_tic);
		nsec = cur_tic.tv_sec * 1000000000L + cur_tic.tv_nsec;
		//printf("time = %ld, %ld %ld\n", nsec, cur_tic.tv_sec, cur_tic.tv_nsec);
		// May be for safe truncate
		// nsec = (cur_tic.tv_sec - ((cur_tic.tv_sec / 1000000000L) * 1000000000L)) * 1000000000L + cur_tic.tv_nsec;
	}

	time64_t & operator= (int64_t x) {
		nsec = x;
		return *this;
	}

	time64_t & operator= (time64_t const & t) {
		// do not check for self copying, since self copying is safe
		nsec = t.nsec;
		return *this;
	}

	time64_t operator+ (time64_t const & t) const {
		time64_t x;
		x.nsec = nsec + t.nsec;
		return x;
	}

	time64_t operator- (time64_t const & t) const {
		time64_t x;
		x.nsec = nsec - t.nsec;
		return x;
	}

	time64_t & operator+= (time64_t const & t) {
		nsec += t.nsec;
		return *this;
	}

	time64_t & operator-= (time64_t const & t) {
		nsec -= t.nsec;
		return *this;
	}


	bool operator> (time64_t const & t) const {
		return nsec > t.nsec;
	}

	bool operator< (time64_t const & t) const {
		return nsec < t.nsec;
	}

	bool operator>= (time64_t const & t) const {
		return nsec >= t.nsec;
	}

	bool operator<= (time64_t const & t) const {
		return nsec <= t.nsec;
	}

	int64_t operator/ (int64_t x) const {
		return nsec/x;
	}

	int64_t milliseconds() {
		return nsec / 1000000L;
	}

	int64_t microseconds() {
		return nsec / 1000L;
	}


};

}


#endif /* TIME_HXX_ */
