/*
 * page_interface.hxx
 *
 *  Created on: 25 mars 2014
 *      Author: gschwind
 */

#ifndef PAGE_INTERFACE_HXX_
#define PAGE_INTERFACE_HXX_

#include <string>
#include <list>
#include <vector>
#include <map>

//namespace page {
//
//class page_interface {
//
//
//public:
//
//
//	virtual ~page_interface() { }
//
//	virtual void set_default_pop(notebook_t * x);
//	virtual void set_focus(client_managed_t * w, Time tfocus);
//
//	virtual compositor_t * get_render_context();
//	virtual display_t * get_xconnection();
//
//	virtual void run();
//
//	virtual void update_client_list();
//	virtual void update_net_supported();
//
//	virtual void print_window_attributes(Window w, XWindowAttributes &wa);
//
//	virtual client_managed_t * manage(managed_window_type_e type, Atom net_wm_type, Window w, XWindowAttributes const & wa);
//	virtual void unmanage(client_managed_t * mw);
//
//	virtual void remove_window_from_tree(client_managed_t * x);
//	virtual void insert_window_in_tree(client_managed_t * x, notebook_t * n, bool prefer_activate);
//	virtual void iconify_client(client_managed_t * x);
//	virtual void update_allocation();
//
//	virtual void destroy(Window w);
//
//	virtual void fullscreen(client_managed_t * c, viewport_t * v = 0);
//	virtual void unfullscreen(client_managed_t * c);
//	virtual void toggle_fullscreen(client_managed_t * c);
//
//	virtual void split(notebook_t * nbk, split_type_e type);
//	virtual void split_left(notebook_t * nbk, client_managed_t * c);
//	virtual void split_right(notebook_t * nbk, client_managed_t * c);
//	virtual void split_top(notebook_t * nbk, client_managed_t * c);
//	virtual void split_bottom(notebook_t * nbk, client_managed_t * c);
//	virtual void notebook_close(notebook_t * src);
//
//	virtual void update_popup_position(popup_notebook0_t * p, i_rect & position);
//	virtual void update_popup_position(popup_frame_move_t * p, i_rect & position);
//
//	virtual void fix_allocation(viewport_t & v);
//
//	virtual split_t * new_split(split_type_e type);
//	virtual void destroy(split_t * x);
//
//	virtual notebook_t * new_notebook();
//	virtual void destroy(notebook_t * x);
//
//	virtual void destroy_managed_window(client_managed_t * mw);
//
//	virtual void process_net_vm_state_client_message(Window c, long type, Atom state_properties);
//
//	virtual void update_transient_for(Window w);
//	virtual Window get_transient_for(Window w);
//	virtual void logical_raise(Window w);
//	virtual void cleanup_transient_for_for_window(Window w);
//
//
//
//	virtual void safe_raise_window(Window w);
//	virtual void clear_transient_for_sibbling_child(Window w);
//
//	virtual Window find_root_window(Window w);
//	virtual Window find_client_window(Window w);
//
//	virtual void compute_client_size_with_constraint(Window c,
//			unsigned int max_width, unsigned int max_height, unsigned int & width,
//			unsigned int & height);
//
//
//	virtual void print_tree_windows();
//
//	virtual void bind_window(client_managed_t * mw);
//	virtual void unbind_window(client_managed_t * mw);
//
//	virtual void grab_pointer();
//
//	virtual void cleanup_grab(client_managed_t * mw);
//
//	virtual notebook_t * get_another_notebook(tree_t * base = 0, tree_t * nbk = 0);
//
//
//	virtual void get_notebooks(std::vector<notebook_t *> & l);
//	virtual void get_splits(std::vector<split_t *> & l);
//
//	virtual notebook_t * find_notebook_for(client_managed_t * mw);
//
//	virtual void get_managed_windows(std::list<client_managed_t *> & l);
//	virtual client_managed_t * find_managed_window_with(Window w);
//	virtual client_not_managed_t * find_unmanaged_window_with(Window w);
//
//	virtual viewport_t * find_viewport_for(notebook_t * n);
//
//	virtual bool is_valid_notebook(notebook_t * n);
//
//	virtual void set_window_cursor(Window w, Cursor c);
//
//	virtual bool is_focussed(client_managed_t * mw);
//
//	virtual void update_windows_stack();
//
//	virtual void update_viewport_layout();
//	virtual void remove_viewport(viewport_t * v);
//	virtual void destroy_viewport(viewport_t * v);
//
//	virtual Atom find_net_wm_type(Window w, bool override_redirect);
//
//	virtual bool onmap(Window w);
//
//	virtual void create_managed_window(Window w, Atom type, XWindowAttributes const & wa);
//
//	virtual void ackwoledge_configure_request(XConfigureRequestEvent const & e);
//
//	virtual void create_unmanaged_window(Window w, Atom type);
//	virtual viewport_t * find_mouse_viewport(int x, int y);
//
//	virtual bool get_safe_net_wm_user_time(Window w, Time & time);
//
//	virtual std::list<viewport_t *> viewports();
//
//	virtual void update_page_areas();
//
//	static void get_notebooks(tree_t * base, std::vector<notebook_t *> & l);
//	static void get_splits(tree_t * base, std::vector<split_t *> & l);
//
//
//	virtual void print_tree();
//
//	virtual void set_desktop_geometry(long width, long height);
//	virtual void set_opaque_region(Window w, region & region);
//
//
//	virtual std::string get_window_std::string(Window w);
//
//
//
//};
//
//}



#endif /* PAGE_INTERFACE_HXX_ */
