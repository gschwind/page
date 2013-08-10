/*
 * smart_pointer.hxx
 *
 *  Created on: 9 août 2013
 *      Author: bg
 */

#ifndef SMART_POINTER_HXX_
#define SMART_POINTER_HXX_



namespace page {

template<typename T>
class smart_pointer_t {
	unsigned int * _nref;
	T * value;

	void _incr_ref() {
		*_nref += 1;
	}

	void _decr_ref() {
		*_nref -= 1;
		if (_nref == 0) {
			delete value;
			delete _nref;
		}
	}

public:

	smart_pointer_t() {
		value = new T();
		_nref = new unsigned int(1);
	}

	smart_pointer_t(T * x) {
		value = x;
		_nref = new unsigned int(1);
	}

	smart_pointer_t(smart_pointer_t<T> const & x) {
		_nref = x._nref;
		value = x.value;
		_incr_ref();
	}

	smart_pointer_t & operator=(smart_pointer_t const & x) {
		if (&x != this) {
			_decr_ref();
			_nref = x._nref;
			value = x.value;
			_incr_ref();
		}

		return *this;
	}

	T * operator->() {
		return value;
	}

	T const * operator->() const {
		return value;
	}

	T & operator*() {
		return *value;
	}

	T const & operator*() const {
		return *value;
	}

	T * get() const {
		return value;
	}

};

}


#endif /* SMART_POINTER_HXX_ */
