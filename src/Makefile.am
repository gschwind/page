bin_PROGRAMS = page page_region_test

AM_CXXFLAGS =  \
	-rdynamic \
	$(X11_CFLAGS) \
	$(XCB_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(PANGO_CFLAGS) \
	$(GLIB_CFLAGS) \
	-fno-strict-aliasing

page_SOURCES = \
	dropdown_menu.hxx \
	dropdown_menu.cxx \
	compositor_overlay.hxx \
	compositor_overlay.cxx \
	popup_notebook0.hxx \
	popup_notebook0.cxx \
	popup_split.hxx \
	popup_split.cxx \
	renderable_thumbnail.hxx \
	renderable_thumbnail.cxx \
	renderable_fadeout_pixmap.hxx \
	renderable_fadeout_pixmap.cxx \
	renderable_floating_outer_gradien.hxx \
	renderable_floating_outer_gradien.cxx \
	renderable_notebook_fading.hxx \
	renderable_notebook_fading.cxx \
	renderable_pixmap.hxx \
	renderable_pixmap.cxx \
	renderable_solid.hxx \
	renderable_solid.cxx \
	view.hxx \
	view.cxx \
	view_rebased.hxx \
	view_rebased.cxx \
	view_notebook.hxx \
	view_notebook.cxx \
	view_fullscreen.hxx \
	view_fullscreen.cxx \
	view_dock.hxx \
	view_dock.cxx \
	view_floating.hxx \
	view_floating.cxx \
	view_popup.hxx \
	view_popup.cxx \
	page_component.hxx \
	page_component.cxx \
	client_managed.hxx \
	client_managed.cxx \
	icon_handler.hxx \
	icon_handler.cxx \
	utils.cxx \
	pixmap.cxx \
	tree.cxx \
	grab_handlers.cxx \
	notebook.cxx \
	split.cxx \
	viewport.cxx \
	workspace.cxx \
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
	leak_checker.cxx \
	tree.hxx \
	blur_image_surface.hxx \
	notebook.hxx \
	compositor.hxx \
	client_proxy.hxx \
	config_handler.hxx \
	workspace.hxx \
	split.hxx \
	viewport.hxx \
	grab_handlers.hxx \
	client_managed.hxx \
	display.hxx \
	mainloop.hxx \
	page.hxx \
	utils.hxx

page_LDADD = \
	$(X11_LIBS) \
	$(XCB_LIBS) \
	$(CAIRO_LIBS) \
	$(PANGO_LIBS) \
	$(GLIB_LIBS) \
	$(RT_LIBS) 

page_region_test_SOURCES = \
	page_region_test.cxx 

page_region_test_LDADD = \
	$(X11_LIBS) \
	$(XCB_LIBS) \
	$(CAIRO_LIBS) \
	$(PANGO_LIBS) \
	$(GLIB_LIBS) \
	$(RT_LIBS) 

