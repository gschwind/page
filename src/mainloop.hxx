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

using namespace std;

class poll_callback_t {
	struct pollfd _pfd;
	function<void(struct pollfd const &)> _callback;
public:

	poll_callback_t() : _pfd{-1, 0, 0}, _callback{[](struct pollfd const & x) { } } { }
	template<typename F>
	poll_callback_t(int fd, short events, F f) : _pfd{fd, events, 0}, _callback{f} { }
	void call(struct pollfd const & x) const { _callback(x); }
	struct pollfd pfd() const { return _pfd; }
};

class mainloop_t;

class timeout_t {

	friend mainloop_t;

	time64_t _timebound;
	time64_t _delta;
	function<bool(void)> _callback;


	bool _call() const { return _callback(); }

	void _renew(time64_t cur) {
		_timebound = cur + _delta;
	}

public:

	/* create new timeout from function */
	template<typename F>
	timeout_t(time64_t cur, time64_t delta, F f) : _delta{delta}, _timebound{cur+delta}, _callback{f} { }

	/* copy/update timeout with curent time */
	timeout_t(timeout_t const & x, time64_t cur) : _delta{x._delta}, _timebound{cur+x._delta}, _callback{x._callback} { }

	/* copy timeout with curent time */
	timeout_t(timeout_t const & x) : _delta{x._delta}, _timebound{x._timebound}, _callback{x._callback} { }

	bool operator>(timeout_t const & x) const {
		return _timebound > x._timebound;
	}

	bool operator<(timeout_t const & x) const {
		return _timebound < x._timebound;
	}

	time64_t get_bound() const {
		return _timebound;
	}

};


class mainloop_t {

	vector<struct pollfd> poll_list;
	list<weak_ptr<timeout_t>> timeout_list;
	map<int, poll_callback_t> poll_callback;

	time64_t curtime;

	bool running;

	void _cleanup_timeout() {
		auto i = timeout_list.begin();
		while(i != timeout_list.end()) {
			if((*i).expired()) {
				i = timeout_list.erase(i);
			} else {
				++i;
			}
		}
	}

	void _insert_sorted(shared_ptr<timeout_t> x) {
		auto i = timeout_list.begin();
		while(i != timeout_list.end()) {
			if((*i).expired()) {
				i = timeout_list.erase(i);
			} else {
				if(*(*i).lock() > *x) {
					break;
				} else {
					++i;
				}
			}
		}

		timeout_list.insert(i, x);

	}

	void run_timeout() {
		curtime.update_to_current_time();
		auto i = timeout_list.begin();
		while(i != timeout_list.end()) {
			if((*i).expired()) {
				i = timeout_list.erase(i);
			} else {
				auto x = (*i).lock();
				if (x->get_bound() > curtime)
					break;

				i = timeout_list.erase(i);
				if (x->_call()) {
					x->_renew(time64_t::now());
					_insert_sorted(x);
				}
				++i;
			}
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

			time64_t wait = 1000000000L;

			while(not timeout_list.empty()) {
				if(timeout_list.front().expired()) {
					timeout_list.pop_front();
				} else {
					break;
				}
			}

			if(not timeout_list.empty()) {
				auto const & next = timeout_list.front();
				wait = next.lock()->get_bound() - curtime;
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
	shared_ptr<timeout_t> add_timeout(time64_t timeout, T func) {
		auto x = make_shared<timeout_t>(time64_t::now(), timeout, func);
		_insert_sorted(x);
		return x;
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
