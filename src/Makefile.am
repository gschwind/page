bin_PROGRAMS = page query_tree listen_root_event simple_window_test send_page_quit test_region 

AM_CXXFLAGS =  \
	-rdynamic \
	$(X11_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(XCOMPOSITE_CFLAGS) \
	$(XDAMAGE_CFLAGS) \
	$(XRENDER_CFLAGS) \
	$(XRANDR_CFLAGS) \
	$(XEXT_CFLAGS) \
	$(XINERAMA_CFLAGS) \
	$(FT2_CFLAGS) \
	$(GLIB_CFLAGS) \
	-fno-strict-aliasing \
	-DDATA_DIR="\"$(datadir)\""

page_SOURCES = \
	main.cxx page.cxx \
	tree.cxx notebook.cxx \
	split.cxx root.cxx \
	client.cxx \
	xconnection.cxx \
	ftrace_function.cxx \
	window.cxx \
	popup_frame_move.cxx \
	popup_notebook0.cxx \
	popup_notebook1.cxx \
	popup_split.cxx \
	compositor.cxx \
	viewport.cxx \
	default_theme.cxx \
	minimal_theme.cxx \
	simple_theme.cxx \
	renderable_page.cxx \
	managed_window.cxx \
	renderable_window.cxx \
	window_icon_handler.cxx \
	config_handler.cxx

page_LDADD = \
	$(X11_LIBS) \
	$(CAIRO_LIBS) \
	$(XCOMPOSITE_LIBS) \
	$(XDAMAGE_LIBS) \
	$(XRENDER_LIBS) \
	$(XRANDR_LIBS) \
	$(XEXT_LIBS) \
	$(XINERAMA_LIBS) \
	$(FT2_LIBS) \
	$(GLIB_LIBS) \
	$(RT_LIBS) \
	-lbfd \
	-ldl

query_tree_SOURCES = query_tree.cxx
query_tree_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS)
#query_tree_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing

listen_root_event_SOURCES = listen_root_event.cxx
listen_root_event_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS) 
#listen_root_event_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing

simple_window_test_SOURCES = simple_window_test.cxx
simple_window_test_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS)
#simple_window_test_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing

send_page_quit_SOURCES = send_page_quit.cxx
send_page_quit_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS)
#send_page_quit_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing

test_region_SOURCES = test_region.cxx
test_region_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS)
#test_region_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing
