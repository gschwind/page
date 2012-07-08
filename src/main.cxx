/*
 * main.cxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#include "page.hxx"

int main(int argc, char * * argv) {
	page::page_t * m = new page::page_t(argc, argv);
	m->run();
	delete m;
	return 0;
}
