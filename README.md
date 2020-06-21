# About

*page* is mouse friendly tiling window manager for linux. *page* organizes windows in tiles and groups them in tabs. It allow to reconfigure dynamically tile and tabs with your mouse pointer. *page* can be used with desktop environment that allow to replace the window manager, for instance MATE or XFCE.

*page* is under development and implement ICCCM and EWMH. It is implemented in C++11 and depend on Xlib, xcb, glib, pango and cairo. The code is distributed under GPLv3.

# Features

Supported features:
* Tiling Window manager,
* Mouse friendly,
* Drag&Drop tabs,
* Floating windows,
* Internal compositor,
* Multiple monitor support,
* Multiple desktop.

# Install on gentoo

An ebuild is available in the official gentoo repository, thus just do:

```bash
 $ emerge x11-wm/page
```

*page* package is marked as unstable on amd64 and x86, you may have to add the corresponding keywords as emerge may suggest.

Also, note that, the *page* executable is renamed to /usr/bin/pagewm because /usr/bin/page is used by another package.

That's it, now you may want use page with Mate desktop environment, folow instruction of the next section.

*page* is a standalone window manager, but it doesn't include a full desktop environment. In order to use *page* you need to integrate it into an existing desktop environment. While *page* can be used in any desktop environment that allow to replace the window manager, I recommend to use Mate desktop environment, mainly because I use *page* in this environment. To install Mate in several linux distribution you can follow instruction [here](https://wiki.mate-desktop.org/#!pages/download.md).

# Setup Mate session to use *page*

You can permanently replace the default window manager of Mate, which is *marco*, by *page* with the following command lines in terminal while your are loged in Mate desktop environment:

```bash
 $ dbus-launch --exit-with-session gsettings set org.mate.session.required-components windowmanager page
 $ dbus-launch --exit-with-session gsettings set org.mate.session required-components-list \
       [\'windowmanager\',\ \'panel\']
```

you can now logout then login within mate-desktop with page as windows manager.

## Revert Mate session to default

If you want revert back to the legacy window manager of Mate, login in a console and run :

```bash
 $ gsettings set org.mate.session.required-components windowmanager marco
 $ gsettings set org.mate.session required-components-list \
     [\'windowmanager\',\ \'panel\',\ \'filemanager\']
```

and log-in again.


# Configuration

*page* has a system default configuration file: <code>/usr/share/page/page.conf</code>. You can override default setting with the <code>$HOME/.page.conf</code> file. The easiest way to override the configuration is to copy <code>/usr/share/page/page.conf</code> to <code>$HOME/.page.conf</code> with:

```bash
 $ cp /usr/share/page/page.conf $HOME/.page.conf
```

then edit the <code>$HOME/.page.conf</code> file with your favourite text editor.

# Defaults Shortcut

Default shortcut use *ctrl*, *alt* and *mod4* meta key. *mod4* is Windows key on window or the apple key on MacOS. Many shortcut can be changed in the configuration file.

General shortcuts:

* *mod4* + *q* : terminate page,
* *ctrl* + *alt* + *Left* : go to the left workspace,
* *ctrl* + *alt* + *Right* : go to the right workspace
* *mod4* + *v* : turn the selected window to fullscreen
* *mod4* + *b* : bind the selected window
* *mod4* + *c* : unbind the selected window

Shortcuts for floating windows:

* *alt* + *right mouse button* : resize window
* *alt* + *left mouse button* : move window

Shortcuts bound windows:
* *alt* + *left mouse button* : drag&drop to move window

Shortcut fullscreen windows:
* *alt* + *left mouse button* : drag&drop to change window screen

