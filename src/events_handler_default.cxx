/*
 * events_handler_default.cxx
 *
 *  Created on: 25 mars 2014
 *      Author: gschwind
 */

#include "events_handler_default.hxx"

namespace page {

events_handler_default::~events_handler_default() {
}

void events_handler_default::process_event(XKeyEvent const & e) {

	if (e.type == KeyPress) {
		fprintf(stderr, "KeyPress key = %d, mod4 = %s, mod1 = %s\n", e.keycode,
				e.state & Mod4Mask ? "true" : "false",
				e.state & Mod1Mask ? "true" : "false");
	} else {
		fprintf(stderr, "KeyRelease key = %d, mod4 = %s, mod1 = %s\n",
				e.keycode, e.state & Mod4Mask ? "true" : "false",
				e.state & Mod1Mask ? "true" : "false");
	}

	int n;
	KeySym * k = XGetKeyboardMapping(cnx->dpy, e.keycode, 1, &n);

	if (k == 0)
		return;
	if (n == 0) {
		XFree(k);
		return;
	}

//	printf("key : %x\n", (unsigned) k[0]);

	if (XK_f == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
//		if (client_focused != 0) {
//			if (has_key(orig_window_to_notebook_window, client_focused))
//				toggle_fullscreen(orig_window_to_notebook_window[client_focused]);
//		}
	}

	if (XK_q == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		running = false;
	}

	if (XK_w == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		_page->update_windows_stack();
		_page->print_tree_windows();

		list<managed_window_t *> lst;
		_page->get_managed_windows(lst);
		for (list<managed_window_t *>::iterator i = lst.begin(); i != lst.end();
				++i) {
			switch ((*i)->get_type()) {
			case MANAGED_NOTEBOOK:
				printf("[%lu] notebook : %s\n", (*i)->orig(),
						(*i)->title());
				break;
			case MANAGED_FLOATING:
				printf("[%lu] floating : %s\n", (*i)->orig(),
						(*i)->title());
				break;
			case MANAGED_FULLSCREEN:
				printf("[%lu] fullscreen : %s\n", (*i)->orig(),
						(*i)->title());
				break;
			}
		}

		//printf("fast_region_surf = %g (%.2f)\n", rnd->fast_region_surf, rnd->fast_region_surf / (rnd->fast_region_surf + rnd->slow_region_surf) * 100.0);
		//printf("slow_region_surf = %g (%.2f)\n", rnd->slow_region_surf, rnd->slow_region_surf / (rnd->fast_region_surf + rnd->slow_region_surf) * 100.0);


	}

	if (XK_s == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		/* free short cut */
	}

	if (XK_r == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		printf("rerender background\n");
		rpage->add_damaged(_root_position);
	}

	if (XK_z == k[0] and e.type == KeyPress and (e.state & Mod4Mask)) {
		if(rnd != 0) {
			if(rnd->get_render_mode() == compositor_t::COMPOSITOR_MODE_AUTO) {
				rnd->set_render_mode(compositor_t::COMPOSITOR_MODE_MANAGED);
			} else {
				rnd->set_render_mode(compositor_t::COMPOSITOR_MODE_AUTO);
			}
		}
	}

	if (XK_Tab == k[0] && e.type == KeyPress && ((e.state & 0x0f) == Mod1Mask)) {

		if (key_press_mode == KEY_PRESS_NORMAL and not managed_window.empty()) {

			/* Grab keyboard */
			XGrabKeyboard(e.display, cnx->get_root_window(), False, GrabModeAsync, GrabModeAsync,
					e.time);

			/** Continue to play event as usual (Alt+Tab is in Sync mode) **/
			XAllowEvents(e.display, AsyncKeyboard, e.time);

			key_press_mode = KEY_PRESS_ALT_TAB;
			key_mode_data.selected = _client_focused.front();

			int sel = 0;

			vector<cycle_window_entry_t *> v;
			int s = 0;
			for(set<managed_window_t *>::iterator i = managed_window.begin();
					i != managed_window.end(); ++i) {
				window_icon_handler_t * icon = new window_icon_handler_t(cnx, (*i)->orig(), 64, 64);
				cycle_window_entry_t * cy = new cycle_window_entry_t(*i, icon);
				v.push_back(cy);

				if(*i == _client_focused.front()) {
					sel = s;
				}

				++s;

			}

			pat->update_window(v, sel);

			viewport_t * viewport = viewport_outputs.begin()->second;

			int y = v.size() / 4 + 1;

			pat->move_resize(
					i_rect(
							viewport->raw_aera.x
									+ (viewport->raw_aera.w - 80 * 4) / 2,
							viewport->raw_aera.y
									+ (viewport->raw_aera.h - y * 80) / 2,
							80 * 4, y * 80));
			pat->show();

		} else {
			XAllowEvents(e.display, ReplayKeyboard, e.time);
		}


		pat->select_next();
		pat->mark_durty();
		pat->expose();
		pat->mark_durty();

	}



	if ((XK_Alt_L == k[0] or XK_Alt_R == k[0]) && e.type == KeyRelease
			&& key_press_mode == KEY_PRESS_ALT_TAB) {

		/** Ungrab Keyboard **/
		XUngrabKeyboard(e.display, e.time);

		key_press_mode = KEY_PRESS_NORMAL;

		/**
		 * do not use dynamic_cast because managed window can be already
		 * destroyed.
		 **/
		managed_window_t * mw = reinterpret_cast<managed_window_t *> (pat->get_selected());
		set_focus(mw, e.time);

		pat->hide();
	}

	XFree(k);

}





void events_handler_default::process_event(XButtonEvent const & e) {
	//fprintf(stderr, "Xpress event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d), time = %lu\n",
	//		e.window, e.root, e.subwindow, e.x_root, e.y_root, e.time);
	managed_window_t * mw = find_managed_window_with(e.window);

	switch (process_mode) {
	case PROCESS_NORMAL:

		if (e.window == rpage->id() && e.subwindow == None && e.button == Button1) {

			update_page_areas();

			page_event_t * b = 0;
			for (vector<page_event_t>::iterator i = page_areas->begin();
					i != page_areas->end(); ++i) {
				//printf("box = %s => %s\n", (*i)->position.to_string().c_str(), typeid(**i).name());
				if ((*i).position.is_inside(e.x, e.y)) {
					b = &(*i);
					break;
				}
			}

			if (b != 0) {

				if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT) {
					process_mode = PROCESS_NOTEBOOK_GRAB;
					mode_data_notebook.c = _upgrade(b->clt);
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
					mode_data_notebook.zone = SELECT_NONE;

					pn0->move_resize(mode_data_notebook.from->tab_area);
					pn0->update_window(mode_data_notebook.c->orig(),
							mode_data_notebook.c->title());
					set_focus(mode_data_notebook.c, e.time);
					rpage->add_damaged(mode_data_notebook.from->allocation());
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLOSE) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_HSPLIT) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_VSPLIT) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_MARK) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = _upgrade(b->clt);
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = _upgrade(b->clt);
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;

				} else if (b->type == PAGE_EVENT_SPLIT) {

					process_mode = PROCESS_SPLIT_GRAB;

					mode_data_split.split_ratio = b->spt->split();
					mode_data_split.split = _upgrade(b->spt);
					mode_data_split.slider_area =
							mode_data_split.split->get_split_bar_area();

					/* show split overlay */
					ps->move_resize(mode_data_split.slider_area);
					ps->show();
					ps->expose();
				}
			}

		}


		if (mw != 0) {

			if (mw->is(MANAGED_FLOATING) and e.button == Button1
					and (e.state & (Mod1Mask | ControlMask))) {

				mode_data_floating.x_offset = e.x;
				mode_data_floating.y_offset = e.y;
				mode_data_floating.x_root = e.x_root;
				mode_data_floating.y_root = e.y_root;
				mode_data_floating.f = mw;
				mode_data_floating.original_position = mw->get_floating_wished_position();
				mode_data_floating.final_position = mw->get_floating_wished_position();
				mode_data_floating.popup_original_position = mw->get_base_position();

//				pfm->move_resize(mw->get_base_position());
//				pfm->update_window(mw->orig(), mw->title());
//				pfm->show();

				if ((e.state & ControlMask)) {
					process_mode = PROCESS_FLOATING_RESIZE;
					mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
				} else {
					safe_raise_window(mw->orig());
					process_mode = PROCESS_FLOATING_MOVE;
				}


			} else if (mw->is(MANAGED_FLOATING) and e.button == Button1
					and e.subwindow != mw->orig()) {

				mw->update_floating_areas();
				vector<floating_event_t> const * l = mw->floating_areas();

				floating_event_t const * b = 0;
				for (vector<floating_event_t>::const_iterator i = l->begin();
						i != l->end(); ++i) {
					//printf("box = %s => %s\n", (*i)->position.to_string().c_str(), "test");
					if((*i).position.is_inside(e.x, e.y)) {
						b = &(*i);
						break;
					}
				}


				if (b != 0) {

					mode_data_floating.x_offset = e.x;
					mode_data_floating.y_offset = e.y;
					mode_data_floating.x_root = e.x_root;
					mode_data_floating.y_root = e.y_root;
					mode_data_floating.f = mw;
					mode_data_floating.original_position = mw->get_floating_wished_position();
					mode_data_floating.final_position = mw->get_floating_wished_position();
					mode_data_floating.popup_original_position = mw->get_base_position();

					if (b->type == FLOATING_EVENT_CLOSE) {

						mode_data_floating.f = mw;
						process_mode = PROCESS_FLOATING_CLOSE;

					} else if (b->type == FLOATING_EVENT_BIND) {

						mode_data_bind.c = mw;
						mode_data_bind.ns = 0;
						mode_data_bind.zone = SELECT_NONE;

						pn0->move_resize(mode_data_bind.c->get_base_position());
						pn0->update_window(mw->orig(), mw->title());

						process_mode = PROCESS_FLOATING_BIND;

					} else if (b->type == FLOATING_EVENT_TITLE) {

//						pfm->move_resize(mw->get_base_position());
//						pfm->update_window(mw->orig(), mw->title());
//						pfm->show();

						safe_raise_window(mw->orig());
						process_mode = PROCESS_FLOATING_MOVE;
					} else {

//						pfm->move_resize(mw->get_base_position());
//						pfm->update_window(mw->orig(), mw->title());
//						pfm->show();

						if (b->type == FLOATING_EVENT_GRIP_TOP) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP;
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM;
						} else if (b->type == FLOATING_EVENT_GRIP_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_LEFT;
						} else if (b->type == FLOATING_EVENT_GRIP_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_RIGHT;
						} else if (b->type == FLOATING_EVENT_GRIP_TOP_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP_LEFT;
						} else if (b->type == FLOATING_EVENT_GRIP_TOP_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP_RIGHT;
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM_LEFT;
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
						} else {
							safe_raise_window(mw->orig());
							process_mode = PROCESS_FLOATING_MOVE;
						}
					}

				}

			} else if (mw->is(MANAGED_FULLSCREEN) and e.button == (Button1)
					and (e.state & (Mod1Mask))) {
				fprintf(stderr, "start FULLSCREEN MOVE\n");
				/** start moving fullscreen window **/
				viewport_t * v = find_mouse_viewport(e.x_root, e.y_root);

				if (v != 0) {
					fprintf(stderr, "start FULLSCREEN MOVE\n");
					process_mode = PROCESS_FULLSCREEN_MOVE;
					mode_data_fullscreen.mw = mw;
					mode_data_fullscreen.v = v;
					pn0->update_window(mw->orig(), mw->title());
					pn0->show();
					pn0->move_resize(v->raw_aera);
					pn0->expose();
				}
			} else if (mw->is(MANAGED_NOTEBOOK) and e.button == (Button1)
					and (e.state & (Mod1Mask))) {

				notebook_t * n = find_notebook_for(mw);

				process_mode = PROCESS_NOTEBOOK_GRAB;
				mode_data_notebook.c = mw;
				mode_data_notebook.from = n;
				mode_data_notebook.ns = 0;
				mode_data_notebook.zone = SELECT_NONE;

				pn0->move_resize(mode_data_notebook.from->tab_area);
				pn0->update_window(mw->orig(), mw->title());

				mode_data_notebook.from->set_selected(mode_data_notebook.c);
				rpage->add_damaged(mode_data_notebook.from->allocation());

			}
		}

		break;
	default:
		break;
	}


	if (process_mode == PROCESS_NORMAL) {

		XAllowEvents(cnx->dpy, ReplayPointer, CurrentTime);

		/**
		 * This focus is anoying because, passive grab can the
		 * focus itself.
		 **/
//		if(mw == 0)
//			mw = find_managed_window_with(e.subwindow);
//
		managed_window_t * mw = find_managed_window_with(e.window);
		if (mw != 0) {
			set_focus(mw, e.time);
		}

//		fprintf(stderr,
//				"UnGrab event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d), time = %lu\n",
//				e.window, e.root, e.subwindow, e.x_root, e.y_root, e.time);

	} else {
		/**
		 * if no change in process mode, focus the window under the cursor
		 * and replay events for the window this window.
		 **/
		/**
		 * keep pointer events for page.
		 * It's like we XGrabButton with GrabModeASync
		 **/

		XAllowEvents(cnx->dpy, AsyncPointer, e.time);

//		fprintf(stderr,
//				"XXXXGrab event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d), time = %lu\n",
//				e.window, e.root, e.subwindow, e.x_root, e.y_root, e.time);
//
	}

	//fprintf(stderr, "Xrelease event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d)\n",
	//		e.window, e.root, e.subwindow, e.x_root, e.y_root);

	if (e.button == Button1) {
		switch (process_mode) {
		case PROCESS_NORMAL:
			fprintf(stderr, "DDDDDDDDDDD\n");
			break;
		case PROCESS_SPLIT_GRAB:

			process_mode = PROCESS_NORMAL;

			ps->hide();

			mode_data_split.split->set_split(mode_data_split.split_ratio);
			rpage->add_damaged(mode_data_split.split->allocation());

			mode_data_split.split = 0;
			mode_data_split.slider_area = i_rect();
			mode_data_split.split_ratio = 0.5;

			break;
		case PROCESS_NOTEBOOK_GRAB:

			process_mode = PROCESS_NORMAL;

			pn0->hide();

			if (mode_data_notebook.zone == SELECT_TAB
					&& mode_data_notebook.ns != 0
					&& mode_data_notebook.ns != mode_data_notebook.from) {
				remove_window_from_tree(mode_data_notebook.c);
				insert_window_in_tree(mode_data_notebook.c,
						mode_data_notebook.ns, true);
			} else if (mode_data_notebook.zone == SELECT_TOP
					&& mode_data_notebook.ns != 0) {
				remove_window_from_tree(mode_data_notebook.c);
				split_top(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_LEFT
					&& mode_data_notebook.ns != 0) {
				remove_window_from_tree(mode_data_notebook.c);
				split_left(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_BOTTOM
					&& mode_data_notebook.ns != 0) {
				remove_window_from_tree(mode_data_notebook.c);
				split_bottom(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_RIGHT
					&& mode_data_notebook.ns != 0) {
				remove_window_from_tree(mode_data_notebook.c);
				split_right(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_CENTER
					&& mode_data_notebook.ns != 0) {
				unbind_window(mode_data_notebook.c);
			} else {
				mode_data_notebook.from->set_selected(mode_data_notebook.c);
			}

			/* Automatically close empty notebook (disabled) */
//			if (mode_data_notebook.from->_clients.empty()
//					&& mode_data_notebook.from->parent() != 0) {
//				notebook_close(mode_data_notebook.from);
//				update_allocation();
//			}

			set_focus(mode_data_notebook.c, e.time);
			if (mode_data_notebook.from != 0)
				rpage->add_damaged(mode_data_notebook.from->allocation());
			if (mode_data_notebook.ns != 0)
				rpage->add_damaged(mode_data_notebook.ns->allocation());

			mode_data_notebook.start_x = 0;
			mode_data_notebook.start_y = 0;
			mode_data_notebook.zone = SELECT_NONE;
			mode_data_notebook.c = 0;
			mode_data_notebook.from = 0;
			mode_data_notebook.ns = 0;

			break;
		case PROCESS_NOTEBOOK_BUTTON_PRESS:
			process_mode = PROCESS_NORMAL;

			{
				page_event_t * b = 0;
				for (vector<page_event_t>::iterator i = page_areas->begin();
						i != page_areas->end(); ++i) {
					if ((*i).position.is_inside(e.x, e.y)) {
						b = &(*i);
						break;
					}
				}

				if (b != 0) {
					if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT) {
						/** do noting **/
					} else if (b->type == PAGE_EVENT_NOTEBOOK_CLOSE) {
						notebook_close(mode_data_notebook.from);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_HSPLIT) {
						split(mode_data_notebook.from, HORIZONTAL_SPLIT);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_VSPLIT) {
						split(mode_data_notebook.from, VERTICAL_SPLIT);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_MARK) {
						if(default_window_pop != 0) {
							default_window_pop->set_default(false);
							rpage->add_damaged(default_window_pop->allocation());
						}
						default_window_pop = mode_data_notebook.from;
						default_window_pop->set_default(true);
						rpage->add_damaged(default_window_pop->allocation());
					} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE) {
						mode_data_notebook.c->delete_window(e.time);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND) {
						unbind_window(mode_data_notebook.c);
					} else if (b->type == PAGE_EVENT_SPLIT) {
						/** do nothing **/
					}
				}
			}

			mode_data_notebook.start_x = 0;
			mode_data_notebook.start_y = 0;
			mode_data_notebook.zone = SELECT_NONE;
			mode_data_notebook.c = 0;
			mode_data_notebook.from = 0;
			mode_data_notebook.ns = 0;

			break;
		case PROCESS_FLOATING_MOVE:

			//pfm->hide();

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e.time);

			process_mode = PROCESS_NORMAL;

			mode_data_floating.mode = RESIZE_NONE;
			mode_data_floating.x_offset = 0;
			mode_data_floating.y_offset = 0;
			mode_data_floating.x_root = 0;
			mode_data_floating.y_root = 0;
			mode_data_floating.original_position = i_rect();
			mode_data_floating.f = 0;
			mode_data_floating.popup_original_position = i_rect();
			mode_data_floating.final_position = i_rect();

			break;
		case PROCESS_FLOATING_RESIZE:

			//pfm->hide();

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e.time);
			process_mode = PROCESS_NORMAL;

			mode_data_floating.mode = RESIZE_NONE;
			mode_data_floating.x_offset = 0;
			mode_data_floating.y_offset = 0;
			mode_data_floating.x_root = 0;
			mode_data_floating.y_root = 0;
			mode_data_floating.original_position = i_rect();
			mode_data_floating.f = 0;
			mode_data_floating.popup_original_position = i_rect();
			mode_data_floating.final_position = i_rect();

			break;
		case PROCESS_FLOATING_CLOSE: {
			managed_window_t * mw = mode_data_floating.f;
			mw->delete_window(e.time);

			/* cleanup */
			process_mode = PROCESS_NORMAL;

			mode_data_floating.mode = RESIZE_NONE;
			mode_data_floating.x_offset = 0;
			mode_data_floating.y_offset = 0;
			mode_data_floating.x_root = 0;
			mode_data_floating.y_root = 0;
			mode_data_floating.original_position = i_rect();
			mode_data_floating.f = 0;
			mode_data_floating.popup_original_position = i_rect();
			mode_data_floating.final_position = i_rect();

			break;
		}

		case PROCESS_FLOATING_BIND: {

			process_mode = PROCESS_NORMAL;

			pn0->hide();

			set_focus(mode_data_bind.c, e.time);

			if (mode_data_bind.zone == SELECT_TAB && mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				insert_window_in_tree(mode_data_bind.c, mode_data_bind.ns,
						true);

				safe_raise_window(mode_data_bind.c->orig());
				rpage->add_damaged(mode_data_bind.ns->allocation());
				update_client_list();

			} else if (mode_data_bind.zone == SELECT_TOP
					&& mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				split_top(mode_data_bind.ns, mode_data_bind.c);
			} else if (mode_data_bind.zone == SELECT_LEFT
					&& mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				split_left(mode_data_bind.ns, mode_data_bind.c);
			} else if (mode_data_bind.zone == SELECT_BOTTOM
					&& mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				split_bottom(mode_data_bind.ns, mode_data_bind.c);
			} else if (mode_data_bind.zone == SELECT_RIGHT
					&& mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				split_right(mode_data_bind.ns, mode_data_bind.c);
			} else {
				bind_window(mode_data_bind.c);
			}

			process_mode = PROCESS_NORMAL;

			mode_data_bind.start_x = 0;
			mode_data_bind.start_y = 0;
			mode_data_bind.zone = SELECT_NONE;
			mode_data_bind.c = 0;
			mode_data_bind.ns = 0;

			break;
		}

		case PROCESS_FULLSCREEN_MOVE:  {

			process_mode = PROCESS_NORMAL;

			viewport_t * v = find_mouse_viewport(e.x_root, e.y_root);

			if (v != 0) {
				if (v != mode_data_fullscreen.v) {
					pn0->move_resize(v->raw_aera);
					mode_data_fullscreen.v = v;
				}
			}

			pn0->hide();

			if (mode_data_fullscreen.v != 0 and mode_data_fullscreen.mw != 0) {
				if (mode_data_fullscreen.v->fullscreen_client
						!= mode_data_fullscreen.mw) {
					unfullscreen(mode_data_fullscreen.mw);
					fullscreen(mode_data_fullscreen.mw, mode_data_fullscreen.v);
				}
			}

			mode_data_fullscreen.v = 0;
			mode_data_fullscreen.mw = 0;

			break;
		}


		default:
			process_mode = PROCESS_NORMAL;
			break;
		}
	}

}




void events_handler_default::process_event(XMotionEvent const & e)  {

	XEvent ev;
	i_rect old_area;
	i_rect new_position;
	static int count = 0;
	count++;
	switch (process_mode) {
	case PROCESS_NORMAL:
		fprintf(stderr, "Warning: This case should not happen %s:%d\n", __FILE__, __LINE__);
		break;
	case PROCESS_SPLIT_GRAB:

		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev));

		if (mode_data_split.split->get_split_type() == VERTICAL_SPLIT) {
			mode_data_split.split_ratio = (ev.xmotion.x
					- mode_data_split.split->allocation().x)
					/ (double) (mode_data_split.split->allocation().w);
		} else {
			mode_data_split.split_ratio = (ev.xmotion.y
					- mode_data_split.split->allocation().y)
					/ (double) (mode_data_split.split->allocation().h);
		}

		if (mode_data_split.split_ratio > 0.95)
			mode_data_split.split_ratio = 0.95;
		if (mode_data_split.split_ratio < 0.05)
			mode_data_split.split_ratio = 0.05;

		/* Render slider with quite complex render method to avoid flickering */
		old_area = mode_data_split.slider_area;
		mode_data_split.split->compute_split_location(
				mode_data_split.split_ratio, mode_data_split.slider_area.x,
				mode_data_split.slider_area.y);


		ps->move(mode_data_split.slider_area.x, mode_data_split.slider_area.y);

		break;
	case PROCESS_NOTEBOOK_GRAB:
		{
		/* Get latest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev))
			continue;

		/* do not start drag&drop for small move */
		if (ev.xmotion.x_root < mode_data_notebook.start_x - 5
				|| ev.xmotion.x_root > mode_data_notebook.start_x + 5
				|| ev.xmotion.y_root < mode_data_notebook.start_y - 5
				|| ev.xmotion.y_root > mode_data_notebook.start_y + 5
				|| !mode_data_notebook.from->tab_area.is_inside(
						ev.xmotion.x_root, ev.xmotion.y_root)) {

			if(!pn0->is_visible())
				pn0->show();
		}

		++count;

		vector<notebook_t *> ln;
		get_notebooks(ln);
		for(vector<notebook_t *>::iterator i = ln.begin(); i != ln.end(); ++i) {
			if ((*i)->tab_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("tab\n");
				if (mode_data_notebook.zone != SELECT_TAB
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_TAB;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->tab_area);
				}
				break;
			} else if ((*i)->right_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("right\n");
				if (mode_data_notebook.zone != SELECT_RIGHT
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_RIGHT;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_right_area);
				}
				break;
			} else if ((*i)->top_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("top\n");
				if (mode_data_notebook.zone != SELECT_TOP
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_TOP;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_top_area);
				}
				break;
			} else if ((*i)->bottom_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("bottom\n");
				if (mode_data_notebook.zone != SELECT_BOTTOM
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_BOTTOM;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_bottom_area);
				}
				break;
			} else if ((*i)->left_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("left\n");
				if (mode_data_notebook.zone != SELECT_LEFT
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_LEFT;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_left_area);
				}
				break;
			} else if ((*i)->popup_center_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("center\n");
				if (mode_data_notebook.zone != SELECT_CENTER
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_CENTER;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_center_area);
				}
				break;
			}
		}
	}

		break;
	case PROCESS_FLOATING_MOVE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev));

		/* compute new window position */
		i_rect new_position = mode_data_floating.original_position;
		new_position.x += e.x_root - mode_data_floating.x_root;
		new_position.y += e.y_root - mode_data_floating.y_root;
		mode_data_floating.final_position = new_position;
		mode_data_floating.f->set_floating_wished_position(new_position);
		mode_data_floating.f->reconfigure();

//		box_int_t popup_new_position = mode_data_floating.popup_original_position;
//		popup_new_position.x += e.x_root - mode_data_floating.x_root;
//		popup_new_position.y += e.y_root - mode_data_floating.y_root;
//		update_popup_position(pfm, popup_new_position);

		break;
	}
	case PROCESS_FLOATING_RESIZE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev));
		i_rect size = mode_data_floating.original_position;

		if(mode_data_floating.mode == RESIZE_TOP_LEFT) {
			size.w -= e.x_root - mode_data_floating.x_root;
			size.h -= e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_TOP) {
			size.h -= e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_TOP_RIGHT) {
			size.w += e.x_root - mode_data_floating.x_root;
			size.h -= e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_LEFT) {
			size.w -= e.x_root - mode_data_floating.x_root;
		} else if (mode_data_floating.mode == RESIZE_RIGHT) {
			size.w += e.x_root - mode_data_floating.x_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM_LEFT) {
			size.w -= e.x_root - mode_data_floating.x_root;
			size.h += e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM) {
			size.h += e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM_RIGHT) {
			size.w += e.x_root - mode_data_floating.x_root;
			size.h += e.y_root - mode_data_floating.y_root;
		}

		/* apply mornal hints */
		unsigned int final_width = size.w;
		unsigned int final_height = size.h;
		compute_client_size_with_constraint(mode_data_floating.f->orig(),
				(unsigned) size.w, (unsigned) size.h, final_width,
				final_height);
		size.w = final_width;
		size.h = final_height;

		if(size.h < 1)
			size.h = 1;
		if(size.w < 1)
			size.w = 1;

		/* do not allow to large windows */
		if(size.w > _root_position.w - 100)
			size.w = _root_position.w - 100;
		if(size.h > _root_position.h - 100)
			size.h = _root_position.h - 100;

		int x_diff = 0;
		int y_diff = 0;

		if(mode_data_floating.mode == RESIZE_TOP_LEFT) {
			x_diff = mode_data_floating.original_position.w - size.w;
			y_diff = mode_data_floating.original_position.h - size.h;
		} else if (mode_data_floating.mode == RESIZE_TOP) {
			y_diff = mode_data_floating.original_position.h - size.h;
		} else if (mode_data_floating.mode == RESIZE_TOP_RIGHT) {
			y_diff = mode_data_floating.original_position.h - size.h;
		} else if (mode_data_floating.mode == RESIZE_LEFT) {
			x_diff = mode_data_floating.original_position.w - size.w;
		} else if (mode_data_floating.mode == RESIZE_RIGHT) {

		} else if (mode_data_floating.mode == RESIZE_BOTTOM_LEFT) {
			x_diff = mode_data_floating.original_position.w - size.w;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM) {

		} else if (mode_data_floating.mode == RESIZE_BOTTOM_RIGHT) {

		}

		size.x += x_diff;
		size.y += y_diff;
		mode_data_floating.final_position = size;

		mode_data_floating.f->set_floating_wished_position(size);
		mode_data_floating.f->reconfigure();

//		box_int_t popup_new_position = size;
//		popup_new_position.x -= theme->floating_margin.left;
//		popup_new_position.y -= theme->floating_margin.top;
//		popup_new_position.w += theme->floating_margin.left + theme->floating_margin.right;
//		popup_new_position.h += theme->floating_margin.top + theme->floating_margin.bottom;
//
//		update_popup_position(pfm, popup_new_position);

		break;
	}
	case PROCESS_FLOATING_CLOSE:
		break;
	case PROCESS_FLOATING_BIND: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev))
			continue;

		/* do not start drag&drop for small move */
		if (ev.xmotion.x_root < mode_data_bind.start_x - 5
				|| ev.xmotion.x_root > mode_data_bind.start_x + 5
				|| ev.xmotion.y_root < mode_data_bind.start_y - 5
				|| ev.xmotion.y_root > mode_data_bind.start_y + 5) {

			if(!pn0->is_visible())
				pn0->show();
//			if(!pn1->is_visible())
//				pn1->show();
		}

		++count;

//		pn1->move(ev.xmotion.x_root + 10, ev.xmotion.y_root);

		vector<notebook_t *> ln;
		get_notebooks(ln);
		for (vector<notebook_t *>::iterator i = ln.begin(); i != ln.end(); ++i) {
			if ((*i)->tab_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("tab\n");
				if (mode_data_bind.zone != SELECT_TAB
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_TAB;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->tab_area);
				}
				break;
			} else if ((*i)->right_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("right\n");
				if (mode_data_bind.zone != SELECT_RIGHT
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_RIGHT;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_right_area);
				}
				break;
			} else if ((*i)->top_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("top\n");
				if (mode_data_bind.zone != SELECT_TOP
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_TOP;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_top_area);
				}
				break;
			} else if ((*i)->bottom_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("bottom\n");
				if (mode_data_bind.zone != SELECT_BOTTOM
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_BOTTOM;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_bottom_area);
				}
				break;
			} else if ((*i)->left_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("left\n");
				if (mode_data_bind.zone != SELECT_LEFT
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_LEFT;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_left_area);
				}
				break;
			} else if ((*i)->popup_center_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("center\n");
				if (mode_data_bind.zone != SELECT_CENTER
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_CENTER;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_center_area);
				}
				break;
			}
		}
	}

		break;
	case PROCESS_FULLSCREEN_MOVE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev))
			continue;

		viewport_t * v = find_mouse_viewport(ev.xmotion.x_root,
				ev.xmotion.y_root);

		if (v != 0) {
			if (v != mode_data_fullscreen.v) {
				pn0->move_resize(v->raw_aera);
				pn0->expose();
				mode_data_fullscreen.v = v;
			}

		}

		break;
	}

	default:
		fprintf(stderr, "Warning: Unknown process_mode\n");
		break;
	}
}



void events_handler_default::process_event(XCrossingEvent const & e) { }

void events_handler_default::process_event(XFocusChangeEvent const & e) { }

void events_handler_default::process_event(XExposeEvent const & e) {
	managed_window_t * mw = find_managed_window_with(e.xexpose.window);
	if (mw != 0) {
		if (mw->is(MANAGED_FLOATING)) {
			mw->expose();
		}
	}

	if (e.xexpose.window == rpage->id()) {
		rpage->expose(
				i_rect(e.xexpose.x, e.xexpose.y, e.xexpose.width,
						e.xexpose.height));
	} else if (e.xexpose.window == pfm->id()) {
		pfm->expose();
	} else if (e.xexpose.window == ps->id()) {
		ps->expose();
	} else if (e.xexpose.window == pat->id()) {
		pat->expose();
	}

}

void events_handler_default::process_event(XGraphicsExposeEvent const & e) { }

void events_handler_default::process_event(XNoExposeEvent const & e) { }

void events_handler_default::process_event(XVisibilityEvent const & e) { }


void events_handler_default::process_event(XCreateWindowEvent const & e) { }


void events_handler_default::process_event(XDestroyWindowEvent const & e) {

	managed_window_t * mw = find_managed_window_with(e.window);
	if(mw != 0) {
		unmanage(mw);
	}

	unmanaged_window_t * uw = find_unmanaged_window_with(e.window);
	if(uw != 0) {
		delete uw;
		unmanaged_window.erase(uw);
	}

//	if(!_client_focused.empty()) {
//		if(_client_focused.front() != 0) {
//			set_focus(_client_focused.front(), CurrentTime);
//		}
//	}

	cleanup_transient_for_for_window(e.window);

	update_client_list();
	rpage->mark_durty();

}


void events_handler_default::process_event(XUnmapEvent const & e) {
	//printf("Unmap event %lu is send event = %d\n", e.window, e.send_event);

	Window x = e.window;

	/**
	 * Filter own unmap.
	 **/
//	bool expected_event = cnx->find_pending_event(event_t(e.serial, e.type));
//	if (expected_event)
//		return;

	/* if client is managed */

	managed_window_t * mw = find_managed_window_with(e.window);
	if(mw != 0 and e.send_event == True) {
		unmanage(mw);
		cleanup_transient_for_for_window(x);
		update_client_list();
	}

}



void events_handler_default::process_event(XMapEvent const & e) {

	{
		string s = get_window_string(e.window);
		printf("map : %s\n", s.c_str());
	}
	/* if map event does not occur within root, ignore it */
	if (e.event != cnx->get_root_window())
		return;

	/** usefull for windows stacking **/
	update_transient_for(e.window);

	if(e.override_redirect) {
		update_windows_stack();
		return;
	}

	if (onmap(e.window)) {
		rpage->mark_durty();
		update_client_list();
	}

	update_windows_stack();

}



void events_handler_default::process_event(XMapRequestEvent const & e) {

	if (e.send_event == True || e.parent != cnx->get_root_window()) {
		XMapWindow(e.display, e.window);
		return;
	}

	if (onmap(e.window)) {
		rpage->mark_durty();
		update_client_list();
		update_windows_stack();
	} else {
		XMapWindow(e.display, e.window);
	}

}




void events_handler_default::process_event(XReparentEvent const & e) {
	if(e.send_event == True)
		return;
	if(e.window == cnx->get_root_window())
		return;

}




void events_handler_default::process_event(XConfigureEvent const & e)  {

}



void events_handler_default::process_event(XGravityEvent const & e) { }


void events_handler_default::process_event(XResizeRequestEvent const & e) { }


void events_handler_default::process_event(XConfigureRequestEvent const & e) {
	//	printf("ConfigureRequest %dx%d+%d+%d above:%lu, mode:%x, window:%lu \n",
	//			e.width, e.height, e.x, e.y, e.above, e.detail, e.window);

	//	printf("name = %s\n", c->get_title().c_str());
	//
	//	printf("send event: %s\n", e.send_event ? "true" : "false");
	//
	//	if (e.value_mask & CWX)
	//		printf("has x: %d\n", e.x);
	//	if (e.value_mask & CWY)
	//		printf("has y: %d\n", e.y);
	//	if (e.value_mask & CWWidth)
	//		printf("has width: %d\n", e.width);
	//	if (e.value_mask & CWHeight)
	//		printf("has height: %d\n", e.height);
	//	if (e.value_mask & CWSibling)
	//		printf("has sibling: %lu\n", e.above);
	//	if (e.value_mask & CWStackMode)
	//		printf("has stack mode: %d\n", e.detail);
	//	if (e.value_mask & CWBorderWidth)
	//		printf("has border: %d\n", e.border_width);

	managed_window_t * mw = find_managed_window_with(e.window);

	if (mw != 0) {

		if ((e.value_mask & (CWX | CWY | CWWidth | CWHeight)) != 0) {

			/** compute floating size **/
			i_rect new_size = mw->get_floating_wished_position();

			if (e.value_mask & CWX) {
				new_size.x = e.x;
			}

			if (e.value_mask & CWY) {
				new_size.y = e.y;
			}

			if (e.value_mask & CWWidth) {
				new_size.w = e.width;
			}

			if (e.value_mask & CWHeight) {
				new_size.h = e.height;
			}

			printf("new_size = %s\n", new_size.to_string().c_str());

			if ((e.value_mask & (CWX)) and (e.value_mask & (CWY)) and e.x == 0
					and e.y == 0 and !viewport_outputs.empty()) {
				viewport_t * v = viewport_outputs.begin()->second;
				i_rect b = v->get_absolute_extend();
				/* place on center */
				new_size.x = (b.w - new_size.w) / 2 + b.x;
				new_size.y = (b.h - new_size.h) / 2 + b.y;
			}

			unsigned int final_width = new_size.w;
			unsigned int final_height = new_size.h;

			compute_client_size_with_constraint(mw->orig(),
					(unsigned) new_size.w, (unsigned) new_size.h, final_width,
					final_height);

			new_size.w = final_width;
			new_size.h = final_height;

			printf("new_size = %s\n", new_size.to_string().c_str());

			/** only affect floating windows **/
			mw->set_floating_wished_position(new_size);
			mw->reconfigure();
		}

	} else {

		//		if (e.value_mask & CWX)
		//			printf("xxhas x: %d\n", e.x);
		//		if (e.value_mask & CWY)
		//			printf("xxhas y: %d\n", e.y);
		//		if (e.value_mask & CWWidth)
		//			printf("xxhas width: %d\n", e.width);
		//		if (e.value_mask & CWHeight)
		//			printf("xxhas height: %d\n", e.height);
		//		if (e.value_mask & CWSibling)
		//			printf("xxhas sibling: %lu\n", e.above);
		//		if (e.value_mask & CWStackMode)
		//			printf("xxhas stack mode: %d\n", e.detail);
		//		if (e.value_mask & CWBorderWidth)
		//			printf("xxhas border: %d\n", e.border_width);

		/** validate configure when window is not managed **/
		ackwoledge_configure_request(e);
	}

}



void events_handler_default::process_event(XCirculateEvent const & e) { }




void events_handler_default::process_event(XCirculateRequestEvent const & e) { }


void events_handler_default::process_event(XPropertyEvent const & e)  {
	//printf("Atom Name = \"%s\"\n", A_name(e.atom).c_str());

	if(e.window == cnx->get_root_window())
		return;

	Window x = e.window;
	managed_window_t * mw = find_managed_window_with(e.window);

	if (e.atom == A(_NET_WM_USER_TIME)) {

	} else if (e.atom == A(_NET_WM_NAME) || e.atom == A(WM_NAME)) {
		if (mw != 0) {
			mw->mark_title_durty();

			if (mw->is(MANAGED_NOTEBOOK)) {
				notebook_t * n = find_notebook_for(mw);
				rpage->add_damaged(n->allocation());
			}

			if (mw->is(MANAGED_FLOATING)) {
				mw->expose();
			}
		}

	} else if (e.atom == A(_NET_WM_STRUT_PARTIAL)) {
		//printf("UPDATE PARTIAL STRUCT\n");
		for(map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin(); i != viewport_outputs.end(); ++i) {
			fix_allocation(*(i->second));
		}
		rpage->mark_durty();
	} else if (e.atom == A(_NET_WM_STRUT)) {
		//x->mark_durty_net_wm_partial_struct();
		//rpage->mark_durty();
	} else if (e.atom == A(_NET_WM_ICON)) {
		if (mw != 0)
			mw->mark_icon_durty();
	} else if (e.atom == A(_NET_WM_WINDOW_TYPE)) {
		/* window type must be set on map, I guess it should never change ? */
		/* update cache */

		//window_t::page_window_type_e old = x->get_window_type();
		//x->read_transient_for();
		//x->find_window_type();
		/* I do not see something in ICCCM */
		//if(x->get_window_type() == window_t::PAGE_NORMAL_WINDOW_TYPE && old != window_t::PAGE_NORMAL_WINDOW_TYPE) {
		//	manage_notebook(x);
		//}
	} else if (e.atom == A(WM_NORMAL_HINTS)) {
		if (mw != 0) {

			if (mw->is(MANAGED_NOTEBOOK)) {
				find_notebook_for(mw)->update_client_position(mw);
			}

			/* apply normal hint to floating window */
			i_rect new_size = mw->get_wished_position();
			unsigned int final_width = new_size.w;
			unsigned int final_height = new_size.h;
			compute_client_size_with_constraint(mw->orig(),
					(unsigned) new_size.w, (unsigned) new_size.h, final_width,
					final_height);
			new_size.w = final_width;
			new_size.h = final_height;
			mw->set_floating_wished_position(new_size);
			mw->reconfigure();

		}
	} else if (e.atom == A(WM_PROTOCOLS)) {
	} else if (e.atom == A(WM_TRANSIENT_FOR)) {
//		printf("TRANSIENT_FOR = #%ld\n", x->transient_for());

		update_transient_for(x);
		update_windows_stack();

	} else if (e.atom == A(WM_HINTS)) {
	} else if (e.atom == A(_NET_WM_STATE)) {
		/* this event are generated by page */
		/* change of net_wm_state must be requested by client message */
	} else if (e.atom == A(WM_STATE)) {
		/** this is set by page ... don't read it **/
	} else if (e.atom == A(_NET_WM_DESKTOP)) {
		/* this set by page in most case */
		//x->read_net_wm_desktop();
	}
}



void events_handler_default::process_event(XSelectionClearEvent const & e) { }
void events_handler_default::process_event(XSelectionRequestEvent const & e) { }
void events_handler_default::process_event(XSelectionEvent const & e) { }
void events_handler_default::process_event(XColormapEvent const & e) { }


void events_handler_default::process_event(XClientMessageEvent const & e)  {

	Window w = e.window;
	if(w == 0)
		return;

	managed_window_t * mw = find_managed_window_with(e.window);

	if (e.message_type == A(_NET_ACTIVE_WINDOW)) {
		if (mw != 0) {
			if(e.data.l[1] == CurrentTime) {
				printf("Invalid focus request\n");
			} else {
				set_focus(mw, e.data.l[1]);
			}
		}
	} else if (e.message_type == A(_NET_WM_STATE)) {

		/* process first request */
		process_net_vm_state_client_message(w, e.data.l[0], e.data.l[1]);
		/* process second request */
		process_net_vm_state_client_message(w, e.data.l[0], e.data.l[2]);

		for (int i = 1; i < 3; ++i) {
			if (std::find(supported_list.begin(), supported_list.end(),
					e.data.l[i]) != supported_list.end()) {
				switch (e.data.l[0]) {
				case _NET_WM_STATE_REMOVE:
					//w->unset_net_wm_state(e.data.l[i]);
					break;
				case _NET_WM_STATE_ADD:
					//w->set_net_wm_state(e.data.l[i]);
					break;
				case _NET_WM_STATE_TOGGLE:
					//w->toggle_net_wm_state(e.data.l[i]);
					break;
				}
			}
		}
	} else if (e.message_type == A(WM_CHANGE_STATE)) {

		/** When window want to become iconic, just bind them **/
		if (mw != 0) {
			if (mw->is(MANAGED_FLOATING) and e.data.l[0] == IconicState) {
				bind_window(mw);
			}
		}

	} else if (e.message_type == A(PAGE_QUIT)) {
		running = false;
	} else if (e.message_type == A(WM_PROTOCOLS)) {
//		char * name = A_name(e.data.l[0]);
//		printf("PROTOCOL Atom Name = \"%s\"\n", name);
//		XFree(name);
	} else if (e.message_type == A(_NET_CLOSE_WINDOW)) {

		XEvent evx;
		evx.xclient.display = cnx->dpy;
		evx.xclient.type = ClientMessage;
		evx.xclient.format = 32;
		evx.xclient.message_type = A(WM_PROTOCOLS);
		evx.xclient.window = e.window;
		evx.xclient.data.l[0] = A(WM_DELETE_WINDOW);
		evx.xclient.data.l[1] = e.data.l[0];

		cnx->send_event(e.window, False, NoEventMask, &evx);
	} else if (e.message_type == A(_NET_REQUEST_FRAME_EXTENTS)) {
			//w->write_net_frame_extents();
	}
}

void events_handler_default::process_event(XMappingEvent const & e) { }



void events_handler_default::process_event(XErrorEvent const & e) { }


void events_handler_default::process_event(XKeymapEvent const & e) { }


void events_handler_default::process_event(XGenericEvent const & e) { }
void events_handler_default::process_event(XGenericEventCookie const & e) { }

/* extension events */
void events_handler_default::process_event(XDamageNotifyEvent const & e) { }

void events_handler_default::process_event(XRRNotifyEvent const & e) {

	update_viewport_layout();
	update_allocation();

	theme->update();

	delete rpage;
	rpage = new renderable_page_t(cnx, theme, _root_position.w,
			_root_position.h);
	rpage->show();

}

XGrabButton(cnx->dpy, AnyButton, AnyModifier, rpage->id(), False,
		ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
		GrabModeSync, GrabModeAsync, None, None);

/** put rpage behind all managed windows **/
update_windows_stack();

}

void events_handler_default::process_event(XShapeEvent const & e);

}
