#include <poll.h>
#include <string.h>
#include <sysexits.h>

#include "calc.h"
#include "cfg.h"
#include "info.h"
#include "laptop.h"
#include "listeners.h"
#include "layout.h"
#include "types.h"
#include "wl_wrappers.h"


// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
void listen(struct Displ *displ) {

	displ->display = checked_wl_display_connect(NULL, __FILE__, __LINE__);

	struct wl_registry *registry = wl_display_get_registry(displ->display);
	wl_registry_add_listener(registry, registry_listener(), displ);

	struct pollfd readfds[1] = {0};
	readfds[0].fd = wl_display_get_fd(displ->display);
	readfds[0].events = POLLIN;

	int num_pending = 0;
	int loops = 0;
	int nloops = 10;
	for (;;) {
		fprintf(stderr, "\n\nlisten %d\n", loops);

		while (checked_wl_display_prepare_read(displ->display, __FILE__, __LINE__) != 0) {
			num_pending = checked_wl_display_dispatch_pending(displ->display, __FILE__, __LINE__);
			fprintf(stderr, "listen 1 dispatched %d pending\n", num_pending);
		}

		checked_wl_display_flush(displ->display, __FILE__, __LINE__);

		fprintf(stderr, "listen polling\n");
		if (poll(readfds, 1, -1) > 0) {
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


		print_heads(ARRIVED, displ->output_manager->heads_arrived);
		slist_free(&displ->output_manager->heads_arrived);

		print_heads(DEPARTED, displ->output_manager->heads_departed);
		output_manager_free_heads_departed(displ->output_manager);

		if (is_dirty(displ->output_manager) && !is_pending_output_manager(displ->output_manager)) {
			fprintf(stderr, "listen dirty, arranging\n");

			reset_dirty(displ->output_manager);

			desire_ltr(displ);

			pend_desired(displ);

			if (is_pending_output_manager(displ->output_manager)) {

				print_heads(DELTA, displ->output_manager->heads);

				apply_desired(displ);
			} else {

				// TODO could print out current state, requires breaking up print_desired
				printf("\nNo changes needed\n");
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

	listen(displ);

	free_displ(displ);

	return EXIT_SUCCESS;
}

