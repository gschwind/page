bin_PROGRAMS = page query_tree listen_root_event simple_window_test send_page_quit test_region list_visual page_internal_compositor test_grab_key

AM_CXXFLAGS =  \
	-rdynamic \
	$(X11_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(PANGO_CFLAGS) \
	$(XCOMPOSITE_CFLAGS) \
	$(XDAMAGE_CFLAGS) \
	$(XRENDER_CFLAGS) \
	$(XRANDR_CFLAGS) \
	$(XEXT_CFLAGS) \
	$(FT2_CFLAGS) \
	$(GLIB_CFLAGS) \
	-fno-strict-aliasing \
	-DDATA_DIR="\"$(datadir)\""

page_SOURCES = \
	main.cxx page.cxx \
	tree.cxx notebook.cxx \
	split.cxx root.cxx \
	ftrace_function.cxx \
	compositor.cxx \
	viewport.cxx \
	simple2_theme.cxx \
	simple_theme.cxx \
	managed_window.cxx \
	unmanaged_window.cxx \
	window_icon_handler.cxx \
	config_handler.cxx \
	xprop.c \
	dsimple.c \
	clientwin.c

page_LDADD = \
	$(X11_LIBS) \
	$(CAIRO_LIBS) \
	$(PANGO_LIBS) \
	$(XCOMPOSITE_LIBS) \
	$(XDAMAGE_LIBS) \
	$(XRENDER_LIBS) \
	$(XRANDR_LIBS) \
	$(XEXT_LIBS) \
	$(FT2_LIBS) \
	$(GLIB_LIBS) \
	$(RT_LIBS) \
	-lbfd \
	-ldl

page_internal_compositor_SOURCES = \
	page_internal_compositor.cxx \
	compositor.cxx 

page_internal_compositor_LDADD = \
	$(X11_LIBS) \
	$(CAIRO_LIBS) \
	$(PANGO_LIBS) \
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
query_tree_LDADD = $(X11_LIBS) 

listen_root_event_SOURCES = listen_root_event.cxx
listen_root_event_LDADD = $(X11_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS)

simple_window_test_SOURCES = simple_window_test.cxx
simple_window_test_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS)

send_page_quit_SOURCES = send_page_quit.cxx
send_page_quit_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) 

test_region_SOURCES = test_region.cxx
test_region_LDADD = $(X11_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) 

list_visual_SOURCES = list_visual.cxx
list_visual_LDADD = $(X11_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) 

test_grab_key_SOURCES = test_grab_key.cxx
test_grab_key_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS)

