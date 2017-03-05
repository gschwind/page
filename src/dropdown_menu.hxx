/*
 * popup_alt_tab.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef DROPDOWN_MENU_HXX_
#define DROPDOWN_MENU_HXX_

#include <cairo.h>
#include <cairo-xcb.h>

#include <string>
#include <memory>
#include <vector>

#include "region.hxx"
#include "page-types.hxx"
#include "theme.hxx"
#include "tree.hxx"

namespace page {

using namespace std;

class dropdown_menu_entry_t {
	friend class dropdown_menu_t;

	theme_dropdown_menu_entry_t _theme_data;
	function<void(xcb_timestamp_t time)> _on_click;

	dropdown_menu_entry_t(dropdown_menu_entry_t const &) = delete;
	dropdown_menu_entry_t & operator=(dropdown_menu_entry_t const &) = delete;

public:
	dropdown_menu_entry_t(shared_ptr<icon16> icon,
			string const & label, function<void(xcb_timestamp_t time)> on_click);
	~dropdown_menu_entry_t();
	shared_ptr<icon16> icon() const;
	string const & label() const;
	theme_dropdown_menu_entry_t const & get_theme_item();

};

struct dropdown_menu_overlay_t : public tree_t {
	page_t * _ctx;
	xcb_pixmap_t _pix;
	cairo_surface_t * _surf;
	rect _position;
	xcb_window_t _wid;
	bool _is_durty;

	dropdown_menu_overlay_t(tree_t * root, rect position);
	~dropdown_menu_overlay_t();
	void map();
	rect const & position();
	xcb_window_t id() const;

	void expose();
	void expose(region const & r);

	virtual void render(cairo_t * cr, region const & area) override;
	virtual region get_opaque_region() override;
	virtual region get_visible_region() override;
	virtual region get_damaged() override;
	virtual void expose(xcb_expose_event_t const * ev) override;

};

class dropdown_menu_t : public grab_handler_t {
	static uint32_t const DEFAULT_BUTTON_EVENT_MASK =
			 XCB_EVENT_MASK_BUTTON_PRESS
			|XCB_EVENT_MASK_BUTTON_RELEASE
			|XCB_EVENT_MASK_BUTTON_MOTION;

public:
	using item_t = dropdown_menu_entry_t;

protected:
	page_t * _ctx;
	vector<shared_ptr<item_t>> _items;
	int _selected;
	shared_ptr<dropdown_menu_overlay_t> pop;
	rect _start_position;
	xcb_button_t _button;
	xcb_timestamp_t _time;
	bool has_been_released;

public:

	dropdown_menu_t(tree_t * ref, vector<shared_ptr<item_t>> items,
			xcb_button_t button, int x, int y, int width, rect start_position);
	~dropdown_menu_t();

	int selected();
	xcb_timestamp_t time();

	void update_backbuffer();
	void update_items_back_buffer(cairo_t * cr, int n);
	void set_selected(int s);
	void update_cursor_position(int x, int y);

	virtual void button_press(xcb_button_press_event_t const * e) override;
	virtual void button_motion(xcb_motion_notify_event_t const * e) override;
	virtual void button_release(xcb_button_release_event_t const * e) override;
	virtual void key_press(xcb_key_press_event_t const * ev) override;
	virtual void key_release(xcb_key_release_event_t const * ev) override;

};



}



#endif /* POPUP_ALT_TAB_HXX_ */
