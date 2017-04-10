/*
 * main.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "execinfo.h"
#include "stdint.h"
#include "page.hxx"


int main(int argc, char * * argv) {

	//signal(SIGSEGV, sig_handler);
	//signal(SIGABRT, sig_handler);

	try {
		auto m = make_shared<page::page_t>(argc, argv);
		m->run();
		m = nullptr;
	} catch (page::wrong_config_file_t & e) {
		fprintf(stderr, "Error: the page config file may be wrong ($HOME/.page.conf)\n");
		fprintf(stderr, "Error: %s\n", e.what());
	} catch (page::exception & e) {
		fprintf(stderr, "%s\n", e.what());
	}
	return 0;
}
