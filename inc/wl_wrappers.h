#ifndef WL_WRAPPERS
#define WL_WRAPPERS

#include <wayland-client-core.h>

#define FL __FILE__, __LINE__

int _wl_display_dispatch_pending(struct wl_display *display, char *file, int line);

int _wl_display_prepare_read(struct wl_display *display, char *file, int line);

int _wl_display_flush(struct wl_display *display, char *file, int line);

int _wl_display_read_events(struct wl_display *display, char *file, int line);

#endif // WL_WRAPPERS

