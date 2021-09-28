// #include <libinput.h>
#include <poll.h>
#include <string.h>
#include <sysexits.h>

#include "calc.h"
#include "cfg.h"
#include "info.h"
#include "lid.h"
#include "listeners.h"
#include "layout.h"
#include "types.h"
#include "wl_wrappers.h"

enum {
	PFD_WL = 0,
	PFD_LI,
	PFD_MAX,
};

void create_pfds(struct Displ *displ) {
	displ->npfds = displ->lid ? PFD_MAX : PFD_MAX - 1;
	displ->pfds = calloc(displ->npfds, sizeof(struct pollfd));
	displ->pfds[PFD_WL].fd = wl_display_get_fd(displ->display);
	displ->pfds[PFD_WL].events = POLLIN;
	if (displ->lid) {
		displ->pfds[PFD_LI].fd = displ->lid->libinput_fd;
		displ->pfds[PFD_LI].events = POLLIN;
	}
}

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
void listen(struct Displ *displ) {

	displ->display = checked_wl_display_connect(NULL, __FILE__, __LINE__);

	struct wl_registry *registry = wl_display_get_registry(displ->display);
	wl_registry_add_listener(registry, registry_listener(), displ);

	create_pfds(displ);

	int num_pending = 0;
	int loops = 0;
	int nloops = 20;
	for (;;) {
		fprintf(stderr, "\n\nlisten 0 loops=%d\n", loops);

		while (checked_wl_display_prepare_read(displ->display, __FILE__, __LINE__) != 0) {
			num_pending = checked_wl_display_dispatch_pending(displ->display, __FILE__, __LINE__);
			fprintf(stderr, "listen 1 dispatched %d pending\n", num_pending);
		}

		checked_wl_display_flush(displ->display, __FILE__, __LINE__);

		fprintf(stderr, "listen polling\n");
		// TODO check poll for -1 error and exit
		if (poll(displ->pfds, displ->npfds, -1) > 0) {
			checked_wl_display_read_events(displ->display, __FILE__, __LINE__);
		} else {
			checked_wl_display_cancel_read(displ->display, __FILE__, __LINE__);
		}


		num_pending = checked_wl_display_dispatch_pending(displ->display, __FILE__, __LINE__);
		fprintf(stderr, "listen 2 dispatched %d pending\n", num_pending);


		if (!displ->output_manager) {
			fprintf(stderr, "ERROR: output manager has been destroyed, exiting\n");
			exit(EX_SOFTWARE);
		}


		if (displ->npfds > PFD_LI && displ->pfds[PFD_LI].revents & displ->pfds[PFD_LI].events) {
			update_lid(displ);
		}
		update_heads_lid_closed(displ);


		print_heads(ARRIVED, displ->output_manager->heads_arrived);
		output_manager_release_heads_arrived(displ->output_manager);

		print_heads(DEPARTED, displ->output_manager->heads_departed);
		output_manager_free_heads_departed(displ->output_manager);

		for (struct SList *i = displ->output_manager->heads; i; i = i->nex) {
			struct Head *head = (struct Head*)i->val;

			fprintf(stderr, " listen enabled=%d lid_closed=%d dirty=%d\n", head->enabled, head->lid_closed, head->dirty);
		}
		fprintf(stderr, " listen displ dirty %d pending %d\n", is_dirty(displ), is_pending_output_manager(displ->output_manager));

		if (is_dirty(displ) && !is_pending_output_manager(displ->output_manager)) {
			fprintf(stderr, "listen dirty, arranging\n");

			reset_dirty(displ);

			desire_ltr(displ);

			pend_desired(displ);

			if (is_pending_output_manager(displ->output_manager)) {

				print_heads(DELTA, displ->output_manager->heads);

				apply_desired(displ);
			}
		} else {
			fprintf(stderr, "listen nothingtodohere\n");
		}


		loops++;
		if (loops == (nloops - 2)) {
			fprintf(stderr, "listen disconnecting WLR\n");
			zwlr_output_manager_v1_stop(displ->output_manager->zwlr_output_manager);
			fprintf(stderr, "listen disconnected WLR\n");
			continue;
		}

		if (loops >= nloops) {
			break;
		}


		fprintf(stderr, "listen end\n");
	}

	fprintf(stderr, "listen exited after %d loops\n", loops);
}

int
main(int argc, const char **argv) {

	struct Displ *displ = calloc(1, sizeof(struct Displ));

	displ->cfg = read_cfg("./cfg.yaml");

	print_cfg(displ->cfg);

	displ->lid = create_lid();

	// update once to discover initial switch state
	update_lid(displ);

	listen(displ);

	free_displ(displ);

	return EXIT_SUCCESS;
}

