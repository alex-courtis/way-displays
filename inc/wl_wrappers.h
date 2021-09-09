#ifndef WL_WRAPPERS
#define WL_WRAPPERS

#include <wayland-client-core.h>

struct wl_display *checked_wl_display_connect(const char *name, char *file, int line);

int checked_wl_display_dispatch_pending(struct wl_display *display, char *file, int line);

int checked_wl_display_prepare_read(struct wl_display *display, char *file, int line);

int checked_wl_display_flush(struct wl_display *display, char *file, int line);

int checked_wl_display_read_events(struct wl_display *display, char *file, int line);

int checked_wl_display_cancel_read(struct wl_display *display, char *file, int line);

#endif // WL_WRAPPERS

