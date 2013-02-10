/*
 * ftrace_function.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef FTRACE_FUNCTION_HXX_
#define FTRACE_FUNCTION_HXX_

#include <string>

extern "C" void trace_init();
extern "C" std::string get_symbol(void * addr);
extern "C" void sig_handler(int sig);

#endif /* FTRACE_FUNCTION_HXX_ */
