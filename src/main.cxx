/*
 * main.cxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#include "page.hxx"

int main(int argc, char * * argv) {
	page_next::main_t * m = new page_next::main_t(argc, argv);
	m->run();
	return 0;
}
