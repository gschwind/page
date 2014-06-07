/*
 * page_internal_compositor.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "utils.hxx"
#include "display.hxx"
#include "compositor.hxx"
#include "time.hxx"

using namespace page;

int main(int argc, char * * argv) {

	display_t * dpy = new display_t();

	/*damage event handler */
	int damage_opcode, damage_event, damage_error;

	/* composite event handler */
	int composite_opcode, composite_event, composite_error;

	/* xshape extension handler */
	int xshape_opcode, xshape_event, xshape_error;

	/* xrandr extension handler */
	int xrandr_opcode, xrandr_event, xrandr_error;

	if (!dpy->check_shape_extension(&xshape_opcode, &xshape_event,
			&xshape_error)) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	}

	if (!dpy->check_randr_extension(&xrandr_opcode, &xrandr_event,
			&xrandr_error)) {
		throw std::runtime_error(RANDR_NAME " extension is not supported");
	}

	if (!dpy->check_composite_extension(&composite_opcode, &composite_event,
			&composite_error)) {
		throw std::runtime_error("X Server doesn't support Composite 0.4");
	}

	if (!dpy->check_damage_extension(&damage_opcode, &damage_event,
			&damage_error)) {
		throw std::runtime_error("Damage extension is not supported");
	}

	if (!dpy->check_shape_extension(&xshape_opcode, &xshape_event,
			&xshape_error)) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	}

	if (!dpy->check_randr_extension(&xrandr_opcode, &xrandr_event,
			&xrandr_error)) {
		throw std::runtime_error(RANDR_NAME " extension is not supported");
	}

	compositor_t * compositor = new page::compositor_t(dpy, damage_event, xshape_event, xrandr_event);

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

