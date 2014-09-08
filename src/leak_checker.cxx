/*
 * leak_checker.cxx
 *
 *  Created on: 8 sept. 2014
 *      Author: gschwind
 */

#include "leak_checker.hxx"

void * page::leak_checker::_leak_check[page::leak_checker::COUNT] = { nullptr };

