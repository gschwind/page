bin_PROGRAMS = page page_region_test

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
	$(GLIB_CFLAGS) \
	-fno-strict-aliasing

page_SOURCES = \
	utils.cxx \
	pixmap.cxx \
	tree.cxx \
	client_base.cxx \
	client_not_managed.cxx \
	client_managed.cxx \
	grab_handlers.cxx \
	notebook.cxx \
	split.cxx \
	viewport.cxx \
	workspace.cxx \
	page_root.cxx \
	page.cxx \
	display.cxx \
	compositor.cxx \
	simple2_theme.cxx \
	tiny_theme.cxx \
	config_handler.cxx \
	client_proxy.cxx \
	popup_alt_tab.cxx \
	main.cxx \
	blur_image_surface.cxx \
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
	$(GLIB_LIBS) \
	$(RT_LIBS) 

page_region_test_SOURCES = \
	page_region_test.cxx 

page_region_test_LDADD = \
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
	$(GLIB_LIBS) \
	$(RT_LIBS) 
