/*
 * ftrace_function.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef FTRACE_FUNCTION_HXX_
#define FTRACE_FUNCTION_HXX_


#include "config.hxx"

#include <string>

extern "C" void trace_init();
extern "C" std::string get_symbol(void * addr);
extern "C" void sig_handler(int sig);

#endif /* FTRACE_FUNCTION_HXX_ */
