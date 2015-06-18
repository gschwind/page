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

#include "time.hxx"

namespace page {


class poll_callback_t {
public:
	short events;

	poll_callback_t(short events) : events{events} { }

	virtual void call(struct pollfd const &) = 0;
	virtual ~poll_callback_t() { }
};

template<typename T>
class poll_callback_impl_t : public poll_callback_t {
	T func;
public:
	virtual void call(struct pollfd const & x) {
		func(x);
	}

	virtual ~poll_callback_impl_t() { }

	poll_callback_impl_t(T const & func, short events) : poll_callback_t{events}, func{func} { }

};

class timeout_t {

protected:
	time_t timebound;
	time_t delta;

public:

	timeout_t(time_t cur, time_t delta) : delta{delta}, timebound{cur+delta} { }
	virtual ~timeout_t() { }

	virtual bool call() = 0;

	void renew(time_t cur) {
		timebound = cur + delta;
	}

	bool operator<(timeout_t const & x) {
		return timebound < x.timebound;
	}

	bool operator>(timeout_t const & x) {
		return timebound > x.timebound;
	}

	bool operator<=(timeout_t const & x) {
		return timebound <= x.timebound;
	}

	bool operator>=(timeout_t const & x) {
		return timebound >= x.timebound;
	}

	time_t get_bound() {
		return timebound;
	}

};


template<typename T>
class timeout_impl_t : public timeout_t {
	T func;
public:

	virtual ~timeout_impl_t() { }

	bool call() {
		return func();
	}

	timeout_impl_t(T const & func, time_t cur, time_t delta) : timeout_t(cur, delta), func{func} {

	}

};


class mainloop_t {

	struct _timeout_comp_t {
		bool operator()(std::shared_ptr<timeout_t> const & x, std::shared_ptr<timeout_t> const & y) { return *x < *y; }
	};

	std::vector<struct pollfd> poll_list;
	std::priority_queue<std::shared_ptr<timeout_t>, std::vector<std::shared_ptr<timeout_t>>, _timeout_comp_t> timeout_list;
	std::map<int, std::shared_ptr<poll_callback_t>> poll_callback;

	time_t curtime;

	bool running;

	void run_timeout() {
		curtime.update_to_current_time();

		while(not timeout_list.empty()) {
			if(timeout_list.top()->get_bound() <= curtime) {
				auto x = timeout_list.top();
				timeout_list.pop();
				if(x->call()) {
					time_t cur;
					cur.update_to_current_time();
					x->renew(cur);
					timeout_list.push(x);
				}
			} else {
				break;
			}
		}

	}

	void run_poll_callback() {
		for(auto & pfd: poll_list) {
			if(pfd.revents != 0) {
				poll_callback[pfd.fd]->call(pfd);
			}
		}
	}

	void update_poll_list() {
		poll_list.clear();
		for(auto & x: poll_callback) {
			struct pollfd tmp;
			tmp.fd = x.first;
			tmp.events = x.second->events;
			tmp.revents = 0;
			poll_list.push_back(tmp);
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
				wait = next->get_bound() - curtime;
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
	std::weak_ptr<timeout_t> add_timeout(time_t timeout, T func) {
		std::shared_ptr<timeout_impl_t<T>> x{new timeout_impl_t<T>(func, curtime, timeout)};
		timeout_list.push(dynamic_pointer_cast<timeout_t>(x));
		return x;
	}


	template<typename T>
	void add_poll(int fd, short events, T callback) {
		std::shared_ptr<poll_callback_impl_t<T>> x{new poll_callback_impl_t<T>(callback, events)};
		poll_callback[fd] = dynamic_pointer_cast<poll_callback_t>(x);
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
