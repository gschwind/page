/*
 * transition.hxx
 *
 *  Created on: 15 août 2015
 *      Author: gschwind
 */

#ifndef SRC_TRANSITION_HXX_
#define SRC_TRANSITION_HXX_

#include "time.hxx"
#include "utils.hxx"

namespace page {


class transition_t {
public:
	signal_t<transition_t *> on_finish;
	signal_t<transition_t *> on_update;

	virtual ~transition_t() { }
	virtual void update(time64_t time) = 0;
	virtual time64_t end() = 0;
	virtual void * target() = 0;
};

template<typename T1, typename T0>
class transition_linear_t : public transition_t {
	time64_t start;
	time64_t _end;
	T0 start_value;
	T0 target_value;
	T1 * _target;
	T0 T1::* target_member;

public:

	transition_linear_t(T1 * target, T0 T1::* target_member, T0 target_value, time64_t duration) {
		start = time64_t::now();
		_end = start + duration;
		start_value = target->*target_member;
		this->target_value = target_value;
		this->_target = target;
		this->target_member = target_member;
	}

	virtual void update(time64_t time) {
		if(time > _end) {
			_target->*target_member = target_value;
			on_finish.signal(this);
		} else {
			_target->*target_member = start_value
					+ (target_value - start_value)
							* static_cast<double>(time - start)
							/ static_cast<double>(_end - start);
			on_update.signal(this);
		}
	}

	virtual time64_t end() {
		return _end;
	}

	virtual void * target() {
		return &(_target->*target_member);
	}

};


}




#endif /* SRC_TRANSITION_HXX_ */
