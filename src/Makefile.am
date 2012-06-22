bin_PROGRAMS = page query_tree listen_root_event simple_window_test


page_SOURCES = main.cxx page.cxx tree.cxx notebook.cxx split.cxx root.cxx client.cxx popup.cxx xconnection.cxx ftrace_function.cxx
page_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS) -lbfd
page_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing

query_tree_SOURCES = query_tree.cxx
query_tree_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS)
query_tree_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing

listen_root_event_SOURCES = listen_root_event.cxx
listen_root_event_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS)
listen_root_event_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing

simple_window_test_SOURCES = simple_window_test.cxx
simple_window_test_LDADD = $(X11_LIBS) $(CAIRO_LIBS) $(XCOMPOSITE_LIBS) $(XDAMAGE_LIBS) $(XRENDER_LIBS) $(XINERAMA_LIBS)
simple_window_test_CXXFLAGS = $(X11_CXXFLAGS) $(CAIRO_CFLAGS) $(XCOMPOSITE_CFLAGS) $(XDAMAGE_CFLAGS) $(XRENDER_CFLAGS) $(XINERAMA_CFLAGS) -fno-strict-aliasing
