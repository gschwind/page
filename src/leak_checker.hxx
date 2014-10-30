/*
 * leack_checker.hxx
 *
 *  Created on: 8 sept. 2014
 *      Author: gschwind
 */

#ifndef LEACK_CHECKER_HXX_
#define LEACK_CHECKER_HXX_

#include <cstdlib>
#include <cstdio>
#include <utility>
#include <stdexcept>

namespace page {

class leak_checker {
	static const unsigned COUNT = 8000;
	static void * _leak_check[COUNT];

	static void * alloc(std::size_t size) {
		void * ret = ::malloc(size);
		for(int k = 0; k < COUNT; ++k) {
			if(_leak_check[k] == nullptr) {
				_leak_check[k] = ret;
				return ret;
			}
		}
		throw std::bad_alloc();
	}

	static void free(void * ptr) {
		for(int k = 0; k < COUNT; ++k) {
			if(_leak_check[k] == ptr) {
				_leak_check[k] = nullptr;
				::free(ptr);
				return;
			}
		}

		printf("Bad delete %p\n", ptr);
	}

public:

	void * operator new(std::size_t size) {
		return alloc(size);
	}

	void * operator new[](std::size_t size) {
		return alloc(size);
	}

	void operator delete (void * ptr) {
		free(ptr);
	}

	void operator delete[] (void * ptr) {
		free(ptr);
	}

	static void check_everinthing_delete() {
		for(int k = 0; k < COUNT; ++k) {
			if(_leak_check[k] != nullptr) {
				printf("%p not deleted as expected\n", _leak_check[k]);
			}
		}
	}


};

}



#endif /* LEACK_CHECKER_HXX_ */
