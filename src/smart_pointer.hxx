/*
 * smart_pointer.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef SMART_POINTER_HXX_
#define SMART_POINTER_HXX_

//
//
//namespace page {
//
//template<typename T>
//class smart_pointer_t {
//
//	struct _intern_pointer {
//		unsigned int nref;
//		T * value;
//	};
//
//
//	_intern_pointer * p;
//
//	void _incr_ref() {
//		p->nref += 1;
//	}
//
//	void _decr_ref() {
//		p->nref -= 1;
//		if (p->nref == 0) {
//			delete p->value;
//			delete p;
//		}
//	}
//
//public:
//
//	smart_pointer_t() {
//		p = new _intern_pointer;
//		p->value = new T();
//		p->nref = 1;
//	}
//
//	smart_pointer_t(T * x) {
//		p = new _intern_pointer;
//		p->value = x;
//		p->nref = 1;
//	}
//
//	smart_pointer_t(smart_pointer_t<T> const & x) {
//		p = x.p;
//		_incr_ref();
//	}
//
//	smart_pointer_t & operator=(smart_pointer_t const & x) {
//		if (&x != this) {
//			_decr_ref();
//			p = x.p;
//			_incr_ref();
//		}
//
//		return *this;
//	}
//
//	T * operator->() {
//		return p->value;
//	}
//
//	T const * operator->() const {
//		return p->value;
//	}
//
//	T & operator*() {
//		return *(p->value);
//	}
//
//	T const & operator*() const {
//		return *(p->value);
//	}
//
//	T * get() const {
//		return (p->value);
//	}
//
//};
//
//}


#endif /* SMART_POINTER_HXX_ */
