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

	while (true) {

		int max = compositor->get_connection_fd();

		FD_ZERO(&fds_read);
		FD_ZERO(&fds_intr);

		FD_SET(compositor->get_connection_fd(), &fds_read);
		FD_SET(compositor->get_connection_fd(), &fds_intr);

		compositor->xflush();

		/**
		 * wait for data in both X11 connection streams (compositor and page)
		 **/
		int nfd = select(compositor->get_connection_fd() + 1, &fds_read, 0, &fds_intr, 0);
		compositor->process_events();
		//compositor->render_flush();
	}
}

