bin_PROGRAMS = page query_tree log_events
page_SOURCES = main.c gtk_xwindow_handler.c page.c tree.c gtk_wm.c
page_LDADD = $(X11_LIBS) $(PANGO_LIBS) $(GDK_LIBS) $(GTK_LIBS)
page_CFLAGS = $(X11_CFLAGS) $(PANGO_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS)

query_tree_SOURCES = query_tree.c
query_tree_LDADD = $(X11_LIBS) $(PANGO_LIBS) $(GDK_LIBS) $(GTK_LIBS)
query_tree_CFLAGS = $(X11_CFLAGS) $(PANGO_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS)

log_events_SOURCES = log_events.c
log_events_LDADD = $(X11_LIBS) $(PANGO_LIBS) $(GDK_LIBS) $(GTK_LIBS)
log_events_CFLAGS = $(X11_CFLAGS) $(PANGO_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS)