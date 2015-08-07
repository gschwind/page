bin_PROGRAMS = page 

AM_CXXFLAGS =  \
	-std=c++11 \
	-rdynamic \
	$(X11_CFLAGS) \
	$(XCB_CFLAGS) \
	$(X11XCB_CFLAGS) \
	$(XCBUTIL_CFLAGS) \
	$(XCBFIXES_CFLAGS) \
	$(XCBDMG_CFLAGS) \
	$(XCBRR_CFLAGS) \
	$(XCBCOMP_CFLAGS) \
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
	tree.cxx \
	client_base.cxx \
	client_not_managed.cxx \
	client_managed.cxx \
	grab_handlers.cxx \
	notebook.cxx \
	split.cxx \
	viewport.cxx \
	desktop.cxx \
	main.cxx \
	page.cxx \
	display.cxx \
	tree.cxx \
	compositor.cxx \
	simple2_theme.cxx \
	tiny_theme.cxx \
	config_handler.cxx \
	composite_surface_manager.cxx \
	leak_checker.cxx 

page_LDADD = \
	$(X11_LIBS) \
	$(XCB_LIBS) \
	$(X11XCB_LIBS) \
	$(XCBUTIL_LIBS) \
	$(XCBFIXES_LIBS) \
	$(XCBDMG_LIBS) \
	$(XCBRR_LIBS) \
	$(XCBCOMP_LIBS) \
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


