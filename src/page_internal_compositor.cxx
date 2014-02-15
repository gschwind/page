/*
 * page_internal_compositor.cxx
 *
 *  Created on: 20 juil. 2013
 *      Author: bg
 */

#include "utils.hxx"
#include "compositor.hxx"

using namespace page;

int main(int argc, char * * argv) {

	compositor_t * compositor = new page::compositor_t();

	fd_set fds_read;
	fd_set fds_intr;

	timespec default_wait = new_timespec(0, 1000000000/30);
	timespec next_frame;

	clock_gettime(CLOCK_MONOTONIC, &next_frame);


	timespec max_wait;


	int max = compositor->fd();

	while (true) {
		timespec cur_tic;

		clock_gettime(CLOCK_MONOTONIC, &cur_tic);

		if(cur_tic > next_frame) {
			next_frame = cur_tic + default_wait;
			max_wait = default_wait;
			compositor->render_simple();
		} else {
			max_wait = pdiff(cur_tic, next_frame);
		}

		compositor->xflush();

		FD_ZERO(&fds_read);
		FD_ZERO(&fds_intr);

		FD_SET(compositor->fd(), &fds_read);
		FD_SET(compositor->fd(), &fds_intr);

		/**
		 * wait for data in both X11 connection streams (compositor and page)
		 **/
		int nfd = pselect(max + 1, &fds_read, 0, &fds_intr, &max_wait, NULL);
		compositor->process_events();

	}
}

