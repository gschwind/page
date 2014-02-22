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

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

namespace page {

class time_t {
	int64_t nsec;

public:

	/* create new time in current time */
	time_t() : nsec(0) { }


	time_t(time_t const & t) : nsec(t.nsec) { }

	/* convert from int64_t */
	time_t(int64_t nsec) : nsec(nsec) { }

	operator timespec() {
		timespec t;
		t.tv_nsec = nsec % 1000000000L;
		t.tv_sec = nsec / 1000000000L;
	}

	operator double() {
		return static_cast<double>(nsec);
	}

	operator int64_t() {
		return nsec;
	}

	void get_time() {
		timespec cur_tic;
		clock_gettime(CLOCK_MONOTONIC, &cur_tic);
		nsec = cur_tic.tv_sec * 1000000000L + cur_tic.tv_nsec;
		// May be for safe truncate
		// nsec = (cur_tic.tv_sec - ((cur_tic.tv_sec / 1000000000L) * 1000000000L)) * 1000000000L + cur_tic.tv_nsec;
	}

	time_t & operator= (int64_t x) {
		nsec = x;
		return *this;
	}

	time_t & operator= (time_t const & t) {
		// do not check for self copying, since self copying is safe
		nsec = t.nsec;
		return *this;
	}

	time_t operator+ (time_t const & t) const {
		time_t x;
		x.nsec = nsec + t.nsec;
		return x;
	}

	time_t operator- (time_t const & t) const {
		time_t x;
		x.nsec = nsec - t.nsec;
		return x;
	}

	bool operator> (time_t const & t) const {
		return nsec > t.nsec;
	}

	bool operator< (time_t const & t) const {
		return nsec < t.nsec;
	}

	int64_t operator/ (int64_t x) const {
		return nsec/x;
	}


};

}


#endif /* TIME_HXX_ */
