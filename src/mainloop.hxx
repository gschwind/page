/*
 * mainloop.hxx
 *
 * copyright (2015) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */
#ifndef SRC_MAINLOOP_HXX_
#define SRC_MAINLOOP_HXX_

#include <unistd.h>
#include <poll.h>

#include <list>
#include <queue>
#include <vector>
#include <functional>
#include <iostream>

#include "time.hxx"

namespace page {

class poll_callback_t {
	struct pollfd _pfd;
	std::function<void(struct pollfd const &)> _callback;
public:

	poll_callback_t() : _pfd{-1, 0, 0}, _callback{[](struct pollfd const & x) { } } { }
	template<typename F>
	poll_callback_t(int fd, short events, F f) : _pfd{fd, events, 0}, _callback{f} { }
	void call(struct pollfd const & x) const { _callback(x); }
	struct pollfd pfd() const { return _pfd; }
};

class timeout_t {
	time_t _timebound;
	time_t _delta;
	std::function<bool(void)> _callback;

public:

	/* create new timeout from function */
	template<typename F>
	timeout_t(time_t cur, time_t delta, F f) : _delta{delta}, _timebound{cur+delta}, _callback{f} { }

	/* copy/update timeout with curent time */
	timeout_t(timeout_t const & x, time_t cur) : _delta{x._delta}, _timebound{cur+x._delta}, _callback{x._callback} { }

	/* copy timeout with curent time */
	timeout_t(timeout_t const & x) : _delta{x._delta}, _timebound{x._timebound}, _callback{x._callback} { }


	bool call() const { return _callback(); }

	void renew(time_t cur) {
		_timebound = cur + _delta;
	}

	bool operator>(timeout_t const & x) const {
		return _timebound > x._timebound;
	}

	time_t get_bound() const {
		return _timebound;
	}

};


class mainloop_t {

	std::vector<struct pollfd> poll_list;
	std::priority_queue<timeout_t, std::vector<timeout_t>, std::greater<timeout_t>> timeout_list;
	std::map<int, poll_callback_t> poll_callback;

	time_t curtime;

	bool running;

	void run_timeout() {
		curtime.update_to_current_time();
		while (not timeout_list.empty()) {
			auto const & x = timeout_list.top();
			if (x.get_bound() > curtime)
				break;
			if (x.call()) {
				timeout_list.push(timeout_t{x, time_t::now()});
			}
			timeout_list.pop();
		}
	}

	void run_poll_callback() {
		for(auto & pfd: poll_list) {
			if(pfd.revents != 0) {
				poll_callback[pfd.fd].call(pfd);
			}
		}
	}

	void update_poll_list() {
		poll_list.clear();
		for(auto const & x: poll_callback) {
			poll_list.push_back(x.second.pfd());
		}
	}

public:

	mainloop_t() : running{false} { }

	void run() {

		run_timeout();

		running = true;
		while (running) {

			time_t wait = 1000000000L;
			if(not timeout_list.empty()) {
				auto const & next = timeout_list.top();
				wait = next.get_bound() - curtime;
			}

			if(poll_list.size() > 0) {
				poll(&poll_list[0], poll_list.size(), wait.milliseconds());
			} else {
				usleep(wait.microseconds());
			}
			run_poll_callback();
			run_timeout();

		}


	}

	template<typename T>
	void add_timeout(time_t timeout, T func) {
		timeout_list.push(timeout_t{time_t::now(), timeout, func});
	}


	template<typename T>
	void add_poll(int fd, short events, T callback) {
		poll_callback[fd] = poll_callback_t{fd, events, callback};
		update_poll_list();
	}

	void remove_poll(int fd) {
		poll_callback.erase(fd);
		update_poll_list();
	}

	void stop() {
		running = false;
	}


};


}



#endif /* SRC_MAINLOOP_HXX_ */
