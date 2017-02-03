/*
 * page_region_test.cxx
 *
 * copyright (2016) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <unistd.h>
#include <poll.h>
#include <cmath>

#include <cstdlib>
#include <xcb/xcb.h>
#include <cairo.h>
#include <cairo-xcb.h>

#include "region.hxx"
#include "time.hxx"

using namespace page;

const int WIDTH = 800;
const int HEIGHT = 800;

xcb_visualtype_t * get_visual_type(xcb_screen_t * _screen, xcb_visualid_t id) {
	/* you init the connection and screen_nbr */
	if (_screen != nullptr) {
		xcb_depth_iterator_t depth_iter;
		depth_iter = xcb_screen_allowed_depths_iterator(_screen);
		for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
			xcb_visualtype_iterator_t visual_iter;
			visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
			for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
				if(visual_iter.data->visual_id == id)
					return visual_iter.data;
			}
		}
	}

	return nullptr;
}

static void _draw_crossed_box(cairo_t * cr, i_rect_t<int> const & box, double r, double g,
		double b) {

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	cairo_identity_matrix(cr);

	cairo_set_source_rgb(cr, r, g, b);
	cairo_set_line_width(cr, 1.0);
	cairo_rectangle(cr, box.x + 0.5, box.y + 0.5, box.w - 1.0, box.h - 1.0);
	cairo_reset_clip(cr);
	cairo_stroke(cr);

	cairo_new_path(cr);
	cairo_move_to(cr, box.x + 0.5, box.y + 0.5);
	cairo_line_to(cr, box.x + box.w - 1.0, box.y + box.h - 1.0);

	cairo_move_to(cr, box.x + box.w - 1.0, box.y + 0.5);
	cairo_line_to(cr, box.x + 0.5, box.y + box.h - 1.0);
	cairo_stroke(cr);

	cairo_restore(cr);
}

void paint_test_0(cairo_surface_t * dsurf, time64_t time) {
	region_t a;
	region_t b;
	region_t c;
	region_t d;

	//cout << "yyya " <<  a.dump_data() << endl;
	//cout << "yyyb " <<  b.dump_data() << endl;

	//cout << "xxxa " << a.to_string() << endl;
	//cout << "xxxb " << b.to_string() << endl;

	a += region_t{50,50,100,100};
	b += region_t{40,40,10,10};
	b += region_t{50,50,10,10};
	b += region_t{55,70,10,10};
	b += region_t{80,14,10,50};
	b += region_t{50,60,10,10};

	b.translate(sin((double)time / 1000000000.0)*100.0+100.0,
			    cos((double)time / 1000000000.0)*100.0+100.0);

	c = a;
	d = b;

	c.translate(300.0, 0.0);
	d.translate(300.0, 0.0);

	//cout << "xxxa " << a.to_string() << endl;
	//cout << "xxxb " << b.to_string() << endl;

	auto surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
	auto cr = cairo_create(surf);

	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_paint(cr);

	auto e = a - b;
	for(auto r: e.rects()) {
		_draw_crossed_box(cr, r, 0.0, 1.0, 0.0);
	}

	for(auto r: b.rects()) {
		_draw_crossed_box(cr, r, 1.0, 0.0, 0.0);
	}

	auto f = c + d;
	for(auto r: f.rects()) {
		_draw_crossed_box(cr, r, 0.0, 1.0, 0.0);
	}

	for(auto r: d.rects()) {
		_draw_crossed_box(cr, r, 1.0, 0.0, 0.0);
	}

	cairo_destroy(cr);

	cr = cairo_create(dsurf);
	cairo_set_source_surface(cr, surf, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	cairo_surface_destroy(surf);

}

int main() {

	xcb_connection_t * cnx = xcb_connect(nullptr, nullptr);
	xcb_screen_t * screen = xcb_setup_roots_iterator(xcb_get_setup(cnx)).data;

	/** root window of screen 0 **/
	xcb_window_t root = screen->root;

	xcb_window_t wid = xcb_generate_id(cnx);

	uint32_t value_mask = 0;
	uint32_t value[5];

	value_mask |= XCB_CW_BACK_PIXEL;
	value[0] = screen->black_pixel;

	value_mask |= XCB_CW_BORDER_PIXEL;
	value[1] = screen->black_pixel;

	value_mask |= XCB_CW_EVENT_MASK;
	value[2] = XCB_EVENT_MASK_EXPOSURE|XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	xcb_create_window(cnx, screen->root_depth, wid, root, 0, 0, WIDTH, HEIGHT, 0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value);

	/** hard coded 9 (i.e. Escape) **/
	xcb_grab_key(cnx, 1, wid, XCB_MOD_MASK_ANY, 9, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	xcb_map_window(cnx, wid);

	auto wsurf = cairo_xcb_surface_create(cnx, wid,
			get_visual_type(screen, screen->root_visual), WIDTH, HEIGHT);


	bool run = true;
	while(run) {
		xcb_flush(cnx);
		usleep(23000);
		xcb_generic_event_t * ev = xcb_poll_for_event(cnx);
		paint_test_0(wsurf, time64_t::now());

		if(ev != nullptr) {
			std::cout << "ev type " << (int)ev->response_type << endl;
			if(ev->response_type == XCB_EXPOSE) {

			} else if (ev->response_type == XCB_CLIENT_MESSAGE) {
				run = false;
			} else if (ev->response_type == XCB_UNMAP_NOTIFY) {
				run = false;
			} else if (ev->response_type == XCB_KEY_PRESS) {
				run = false;
			}


			free(ev);
		}

	}

	cairo_surface_destroy(wsurf);

	cairo_debug_reset_static_data();
	xcb_disconnect(cnx);

	return 0;
}


