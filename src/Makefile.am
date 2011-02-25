bin_PROGRAMS = page query_tree log_events page_next
page_SOURCES = main.c page.c tree.c gtk_wm.c client.c 
page_LDADD = $(X11_LIBS) $(PANGO_LIBS) $(GDK_LIBS) $(GTK_LIBS)
page_CFLAGS = $(X11_CFLAGS) $(PANGO_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS) -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED

query_tree_SOURCES = query_tree.c
query_tree_LDADD = $(X11_LIBS) $(PANGO_LIBS) $(GDK_LIBS) $(GTK_LIBS)
query_tree_CFLAGS = $(X11_CFLAGS) $(PANGO_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS)

log_events_SOURCES = log_events.c
log_events_LDADD = $(X11_LIBS) $(PANGO_LIBS) $(GDK_LIBS) $(GTK_LIBS)
log_events_CFLAGS = $(X11_CFLAGS) $(PANGO_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS)

page_next_SOURCES = page-next.cc
page_next_LDADD = $(X11_LIBS) $(PANGO_LIBS) $(GDK_LIBS) $(GTK_LIBS) $(CAIRO_LIBS)
page_next_CXXFLAGS = $(X11_CFLAGS) $(PANGO_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS) $(CAIRO_CFLAGS)