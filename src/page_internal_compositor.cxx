/*
 * page_internal_compositor.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "utils.hxx"
#include "compositor.hxx"
#include "time.hxx"

using namespace page;

int main(int argc, char * * argv) {

	Display * dpy = XOpenDisplay(NULL);
	compositor_t * compositor = new page::compositor_t(dpy);

	fd_set fds_read;
	fd_set fds_intr;

	page::time_t default_wait(1000000000/30);
	page::time_t next_frame;
	next_frame.get_time();

	timespec _max_wait;

	bool running = true;
	while (running) {

		FD_ZERO(&fds_read);
		FD_ZERO(&fds_intr);

		/** listen for compositor events **/
		FD_SET(compositor->fd(), &fds_read);
		FD_SET(compositor->fd(), &fds_intr);

		/**
		 * wait for data in both X11 connection streams (compositor and page)
		 **/
		_max_wait = default_wait;
		//printf("%ld %ld\n", _max_wait.tv_sec, _max_wait.tv_nsec);
		int nfd = pselect(compositor->fd() + 1, &fds_read, NULL, &fds_intr, &_max_wait,
		NULL);

		compositor->process_events();
		/** limit FPS **/
		page::time_t cur_tic;
		cur_tic.get_time();
		if (cur_tic > next_frame) {
			next_frame = cur_tic + default_wait;
			_max_wait = default_wait;
			compositor->render();
		} else {
			_max_wait = next_frame - cur_tic;
		}
		compositor->xflush();


	}
}

