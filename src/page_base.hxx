/*
 * page_base.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef PAGE_BASE_HXX_
#define PAGE_BASE_HXX_

#include "window.hxx"
#include "tree.hxx"
#include "xconnection.hxx"
#include "render_context.hxx"

namespace page {

class page_base_t {
public:

	typedef std::map<Window, window_t *> window_map_t;
	window_map_t windows_map;
	window_map_t clients_map;
	/* all know windows */
	window_list_t windows_stack;
	/* top_level_windows */
	window_list_t managed_windows;

	tree_t * default_window_pop;
	std::string page_base_dir;
	std::string font;
	std::string font_bold;

	Time last_focus_time;

	virtual ~page_base_t() { };
	virtual void set_focus(window_t * w) = 0;
	virtual render_context_t & get_render_context() = 0;
	virtual xconnection_t & get_xconnection() = 0;
	virtual void process_event(XDamageNotifyEvent const & ev) = 0;
};

}


#endif /* PAGE_BASE_HXX_ */
