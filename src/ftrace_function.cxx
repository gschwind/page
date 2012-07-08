/*
 * ftrace_function.cpp
 *
 * copyright (2012) Benoit Gschwind
 *
 */


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <bfd.h>
#include "execinfo.h"
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#include <cxxabi.h>

#ifdef ENABLE_TRACE

extern "C" {

bfd * g_bfd = 0;
asymbol ** g_symbol_table = 0;
symbol_info * g_symbol_info;
char const ** demangled_symbol;
int g_nsyms;

void load_file(char const * filename, char * data, int size) __attribute__((no_instrument_function));
void trace_init() __attribute__((no_instrument_function));
char const * get_symbol(void * addr) __attribute__((no_instrument_function));


void load_file(char const * filename, char * data, int size) {
	int fd = open(filename, O_RDONLY);
	read(fd, data, size);
	close(fd);
}

void trace_init() {
	bfd_init();

	char string_buff[2000];
	snprintf(string_buff, 2000, "/proc/%d/cmdline", getpid());
	printf("opening %s\n", string_buff);
	char x[1000];
	snprintf(x, 1000, "cat %s", string_buff);
	//system(x);
	char cmdline[2000];
	int size;
	load_file(string_buff, cmdline, 2000);
	cmdline[1999] = 0;
	printf("found execfile %s\n", cmdline);

	g_bfd = bfd_openr(cmdline, 0);

	if (g_bfd == NULL) {
		printf("bfd_openr error\n");
		return;
	} else {
		printf("bfd is opened\n");
	}

	char **matching;
	if (!bfd_check_format_matches(g_bfd, bfd_object, &matching)) {
		printf("format_matches\n");
	}

	long storage_needed;
	long number_of_symbols;
	long i;

	storage_needed = bfd_get_symtab_upper_bound(g_bfd);

	if (storage_needed < 0) {
		throw std::runtime_error("trace fail");
	}

	if (storage_needed == 0) {
		return;
	}

	int nsize = bfd_get_symtab_upper_bound (g_bfd);
	g_symbol_table = (asymbol **) malloc(nsize);
	g_nsyms = bfd_canonicalize_symtab(g_bfd, g_symbol_table);

	g_symbol_info = (symbol_info *)malloc(sizeof(symbol_info) * g_nsyms);
	demangled_symbol = (char const **)malloc(sizeof(char *) * g_nsyms);

	for(int i = 0; i < g_nsyms; ++i) {
		bfd_symbol_info(g_symbol_table[i], &g_symbol_info[i]);
		int status = 0;
		demangled_symbol[i] = abi::__cxa_demangle(g_symbol_info[i].name, 0, 0, &status);
		if(status != 0)
			demangled_symbol[i] = "Error in demangle";
	}

}

char const * get_symbol(void * addr) {
	for (int i = 0; i < g_nsyms; i++) {
		if (g_symbol_info[i].value == (unsigned long) addr) {
			return demangled_symbol[i];
		}
	}

	return "UNKNOW";
}

void __cyg_profile_func_enter(void *, void *)
		__attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *, void *)
		__attribute__((no_instrument_function));

int depth = -1;

void __cyg_profile_func_enter(void *func, void *caller) {

	depth++;
	for (int n = 0; n < depth; n++)
		printf(" ");

	if (!g_bfd) {
		printf("%p\n", func);
	} else {
		char const * symb = get_symbol(func);
		printf("%p %s\n", func, symb);
	}
}

void __cyg_profile_func_exit(void *func, void *caller) {
//	int n;
//	for (n = 0; n < depth; n++)
//		printf(" ");
//	if (!g_bfd) {
//		printf("<- %p\n", func);
//	} else {
//		char const * symb = get_symbol(func);
//		printf("<- %p %s\n", func, symb);
//	}
	depth--;
}

}

#endif
