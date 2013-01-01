/*
 * client.cxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <cairo.h>
#include <cairo-xlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstring>
#include "client.hxx"

namespace page {

//client_t::client_t(window_t & x) : w(x) {
//	w.add_to_save_set();
//
//	icon_surf = 0;
//	update_icon();
//}
//
//client_t::~client_t() {
//
//	w.write_wm_state(WithdrawnState);
//
//	if (icon_surf != 0) {
//		cairo_surface_destroy(icon_surf);
//		icon_surf = 0;
//	}
//
//	if (icon.data != 0) {
//		free(icon.data);
//		icon.data = 0;
//	}
//
//	w.remove_from_save_set();
//}
//
//
//void client_t::set_fullscreen() {
//	w.set_fullscreen();
//}
//
//void client_t::unset_fullscreen() {
//	w.unset_fullscreen();
//}
//
//void client_t::withdraw_to_X() {
////	printf("Manage #%lu\n", w.get_xwin());
//	XWMHints const * hints = w.read_wm_hints();
//	if (hints) {
//		if (hints->initial_state == IconicState) {
//			w.write_wm_state(IconicState);
//		} else {
//			w.write_wm_state(NormalState);
//		}
//	} else {
//		w.write_wm_state(NormalState);
//	}
//
////	if (w.is_dock()) {
////		printf("IsDock !\n");
////		unsigned int n;
////		long const * partial_struct = w.read_partial_struct();
////		if (partial_struct) {
////			w.set_dock_action();
////			w.map();
////			return;
////		} /* if has not partial struct threat it as normal window */
////	}
//
//	w.set_default_action();
//
//}

}
