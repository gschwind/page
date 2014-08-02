bin_PROGRAMS = page test_region list_visual page_internal_compositor test_grab_key

AM_CXXFLAGS =  \
	-std=c++11 \
	-rdynamic \
	$(X11_CFLAGS) \
	$(XCB_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(PANGO_CFLAGS) \
	$(XCOMPOSITE_CFLAGS) \
	$(XDAMAGE_CFLAGS) \
	$(XRENDER_CFLAGS) \
	$(XRANDR_CFLAGS) \
	$(XEXT_CFLAGS) \
	$(FT2_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GL_CFLAGS) \
	-fno-strict-aliasing

page_SOURCES = \
	main.cxx \
	page.cxx \
	display.cxx \
	tree.cxx \
	notebook.cxx \
	split.cxx \
	compositor.cxx \
	viewport.cxx \
	simple2_theme.cxx \
	managed_window.cxx \
	unmanaged_window.cxx \
	window_icon_handler.cxx \
	config_handler.cxx \
	composite_surface_manager.cxx 

page_LDADD = \
	$(X11_LIBS) \
	$(XCB_LIBS) \
	$(CAIRO_LIBS) \
	$(PANGO_LIBS) \
	$(XCOMPOSITE_LIBS) \
	$(XDAMAGE_LIBS) \
	$(XRENDER_LIBS) \
	$(XRANDR_LIBS) \
	$(XEXT_LIBS) \
	$(FT2_LIBS) \
	$(GLIB_LIBS) \
	$(RT_LIBS) 

page_internal_compositor_SOURCES = \
	display.cxx \
	page_internal_compositor.cxx \
	compositor.cxx \
	composite_surface_manager.cxx \
	tree.cxx

page_internal_compositor_LDADD = \
	$(X11_LIBS) \
	$(XCB_LIBS) \
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
	$(RT_LIBS) 

test_region_SOURCES = test_region.cxx
test_region_LDADD = $(X11_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) 

list_visual_SOURCES = list_visual.cxx
list_visual_LDADD = $(X11_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) 

test_grab_key_SOURCES = test_grab_key.cxx
test_grab_key_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS)

