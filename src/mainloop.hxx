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

#include <signal.h>

#include "time.hxx"
#include "utils.hxx"

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
	function<void(void)> _callback;


	void _call() const { _callback(); }

public:

	/* create new timeout from function */
	template<typename F>
	timeout_t(time64_t timebound, F f) : _timebound{timebound}, _callback{f} { }

	/* copy timeout with curent time */
	timeout_t(timeout_t const & x) : _timebound{x._timebound}, _callback{x._callback} { }

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
	static bool got_sigterm;

	vector<struct pollfd> poll_list;
	list<weak_ptr<timeout_t>> timeout_list;
	map<int, poll_callback_t> poll_callback;

	bool running;

	static void handle_sigterm(int sig) {
		if (sig == SIGTERM) {
			got_sigterm = true;
		}
	}

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

	int64_t run_timeout() {
		while (not timeout_list.empty()) {
			if (timeout_list.front().expired()) {
				timeout_list.pop_front();
			} else {
				auto next = timeout_list.front().lock();
				int64_t wait = next->get_bound() - time64_t::now();
				if (wait <= 1000000L) {
					timeout_list.pop_front();
					next->_call();
					return 0L;
				} else {
					return wait;
				}
			}
		}
		return 10000000000L;
	}

	void run_poll_callback() {
		for(auto & pfd: poll_list) {
			if(pfd.revents&pfd.events) {
				poll_callback[pfd.fd].call(pfd);
				pfd.revents = 0;
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

		sighandler_t old_sigterm = signal(SIGTERM, &handle_sigterm);

		running = true;
		while (running and not got_sigterm) {
			int64_t wait = run_timeout();
			if(poll_list.size() > 0) {
				poll(&poll_list[0], poll_list.size(), wait/1000000L);
			} else {
				if(wait > 1000L)
					usleep(wait/1000L);
			}

			run_poll_callback();
		}

		running = false;
		got_sigterm = false;
		signal(SIGTERM, old_sigterm);

	}

	template<typename T>
	shared_ptr<timeout_t> add_timeout(time64_t timeout, T func) {
		auto x = make_shared<timeout_t>(time64_t::now() + timeout, func);
		_insert_sorted(x);
		return x;
	}

	template<typename T>
	shared_ptr<timeout_t> add_timebound(time64_t timebound, T func) {
		auto x = make_shared<timeout_t>(timebound, func);
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
