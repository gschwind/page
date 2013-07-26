/*
 * page_internal_compositor.cxx
 *
 *  Created on: 20 juil. 2013
 *      Author: bg
 */

#include "compositor.hxx"

int main(int argc, char * * argv) {

	page::compositor_t * compositor = new page::compositor_t();

	fd_set fds_read;
	fd_set fds_intr;

	timespec max_wait;
	max_wait.tv_sec = 0;
	max_wait.tv_nsec = 15000000;

	int max = compositor->get_connection_fd();

	while (true) {

		FD_ZERO(&fds_read);
		FD_ZERO(&fds_intr);

		FD_SET(compositor->get_connection_fd(), &fds_read);
		FD_SET(compositor->get_connection_fd(), &fds_intr);

		/**
		 * wait for data in both X11 connection streams (compositor and page)
		 **/
		int nfd = pselect(max + 1, &fds_read, 0,
				&fds_intr, &max_wait, NULL);
		compositor->process_events();
		compositor->xflush();

	}
}

