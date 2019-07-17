#include <sys/types.h>
#include <sys/shm.h>

#include <errno.h>
#include <execinfo.h>
#include <getopt.h>
#include <libgen.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include "camera.h"
#include "renderer.h"
#include "scene.h"
#include "tensor.h"
#include "world.h"

static void
sighandler(int signum __attribute__((unused)), siginfo_t *siginfo, void *context __attribute__((unused)))
{
	psiginfo(siginfo, NULL);

	void *buffer[255] = {};
	
	int calls = 0;
	calls = backtrace(buffer, sizeof(buffer) / sizeof(void *));
	backtrace_symbols_fd(buffer, calls, STDERR_FILENO);

	_exit(EXIT_FAILURE);
}

static void
loop(Display *display, Window window, ObscuraRenderer *renderer)
{
	XGCValues gc_values = {
		.graphics_exposures = False,
	};

	GC context = 0;
	context = XCreateGC(display, window, GCGraphicsExposures, &gc_values);

	uint64_t frame_count = 0;

	bool running = true;
	while (running) {
		struct timespec t0 = {};
		clock_gettime(CLOCK_MONOTONIC, &t0);

		while (XPending(display)) {
			XEvent event = {};
			XNextEvent(display, &event);

			switch (event.type) {
			case KeyPress:
				if (XLookupKeysym(&event.xkey, 0) == XK_Escape) {
					running = false;
				} else if (XLookupKeysym(&event.xkey, 0) == XK_c) {
					ObscuraNode *view = renderer->world->scene->view;
					ObscuraComponent *component = ObscuraFindAnyComponent(view, OBSCURA_COMPONENT_FAMILY_CAMERA);
					ObscuraCamera *camera = component->component;
					camera->filter = OBSCURA_CAMERA_FILTER_TYPE_COLOR;
				} else if (XLookupKeysym(&event.xkey, 0) == XK_d) {
					ObscuraNode *view = renderer->world->scene->view;
					ObscuraComponent *component = ObscuraFindAnyComponent(view, OBSCURA_COMPONENT_FAMILY_CAMERA);
					ObscuraCamera *camera = component->component;
					camera->filter = OBSCURA_CAMERA_FILTER_TYPE_DEPTH;
				} else if (XLookupKeysym(&event.xkey, 0) == XK_n) {
					ObscuraNode *view = renderer->world->scene->view;
					ObscuraComponent *component = ObscuraFindAnyComponent(view, OBSCURA_COMPONENT_FAMILY_CAMERA);
					ObscuraCamera *camera = component->component;
					camera->filter = OBSCURA_CAMERA_FILTER_TYPE_NORMAL;
				}
				break;
			case KeyRelease:
				break;
			case MotionNotify:
				break;
			case ButtonPress:
				break;
			case ButtonRelease:
				break;
			}
		}

		ObscuraDraw(renderer);

		ObscuraFramebuffer *framebuffer = renderer->framebuffer;
		XPutImage(display, window, context, framebuffer->image, 0, 0, 0, 0, framebuffer->width, framebuffer->height);

		struct timespec t1 = {};
		clock_gettime(CLOCK_MONOTONIC, &t1);
		uint64_t frame_delta = ((t1.tv_sec - t0.tv_sec) * 1000000000 + (t1.tv_nsec - t0.tv_nsec)) / 1000000;

		char *str = NULL;
		if (asprintf(&str, "frame:%ld|time:%ldms", frame_count, frame_delta) != -1) {
			XGCValues gc_values = {
				.foreground = 0x22ff00,
			};

			GC gc = XCreateGC(display, window, GCForeground, &gc_values);
			XDrawString(display, window, gc, 10, 20, str, strlen(str));

			free(str);
		}
		if (asprintf(&str, "camera:%ld|reflect:%ld|refract:%ld|shadow:%ld",
				renderer->cast_count[OBSCURA_RENDERER_RAY_TYPE_CAMERA],
				renderer->cast_count[OBSCURA_RENDERER_RAY_TYPE_REFLECTION],
				renderer->cast_count[OBSCURA_RENDERER_RAY_TYPE_REFRACTION],
				renderer->cast_count[OBSCURA_RENDERER_RAY_TYPE_SHADOW]) != -1) {
			XGCValues gc_values = {
				.foreground = 0x22ff00,
			};

			GC gc = XCreateGC(display, window, GCForeground, &gc_values);
			XDrawString(display, window, gc, 10, 40, str, strlen(str));

			free(str);
		}

		frame_count++;

		XSync(display, False);
	}
}

static void
putpixel(void *image, int x, int y, uint32_t color)
{
    XPutPixel((XImage *) image, x, y, color);
}

int main(int argc, char **argv) {
	struct sigaction signal_act = {
		.sa_sigaction = &sighandler,
		.sa_flags     = SA_SIGINFO,
	};

	if (sigaction(SIGUSR1, &signal_act, NULL) == -1) {
		fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}

	uint16_t width  = 1280;
	uint16_t height = 720;

	int opt = 0;
	while ((opt = getopt(argc, argv, "h:w:")) != -1) {
		switch (opt) {
		case 'h':
			height = atoi(optarg);
			break;
		case 'w':
			width = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s [-h height] [-w width]\n", basename(argv[0]));
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	Display *display = NULL;
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, "unable to connect to X server");
		exit(EXIT_FAILURE);
	}

	XVisualInfo visual_tpl = {
		.visualid = XVisualIDFromVisual(XDefaultVisual(display, XDefaultScreen(display))),
	};
	
	int visual_count = 0;

	XVisualInfo *visual_info = NULL;
	visual_info = XGetVisualInfo(display, VisualIDMask, &visual_tpl, &visual_count);

	Window window_root = 0;
	window_root = XRootWindow(display, visual_info->screen);

	int window_attrs_mask = 0;
	window_attrs_mask = CWEventMask | CWColormap | CWBorderPixel;
	
	XSetWindowAttributes window_attrs = {
		.event_mask   = KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		.colormap     = XCreateColormap(display, window_root, visual_info->visual, AllocNone),
		.border_pixel = 0,
	};

	Window window = 0;
	window = XCreateWindow(display, window_root, 0, 0, width, height, 0, visual_info->depth, InputOutput,
			visual_info->visual, window_attrs_mask, &window_attrs);

	XSizeHints window_hints = {
		.flags	    = PMinSize | PMaxSize,
		.min_width  = width,
		.max_width  = width,
		.min_height = height,
		.max_height = height,
	};
	XSetWMNormalHints(display, window, &window_hints);
	
	XStoreName(display, window, "Obscura");
	XMapWindow(display, window);

	XShmSegmentInfo shm_info = {};

	XImage *image = NULL;
	image = XShmCreateImage(display, visual_info->visual, visual_info->depth, ZPixmap, NULL, &shm_info, width, height);

	size_t image_size = image->bytes_per_line * image->height;
	shm_info.shmid = shmget((key_t) 0, image_size, IPC_CREAT | 0777);

	shm_info.shmaddr = (char *) shmat(shm_info.shmid, 0, 0);
	image->data = shm_info.shmaddr;

	XShmAttach(display, &shm_info);
	XSync(display, False);
	shmctl(shm_info.shmid, IPC_RMID, 0);

	ObscuraWorld *world = ObscuraCreateWorld();
	ObscuraLoadWorld(world, argv[0]);

	ObscuraFramebuffer framebuffer = {
		.width  = width,
		.height = height,
		.image  = image,
		.paint  = &putpixel,
	};
	ObscuraRenderer renderer = {
		.world       = world,
		.framebuffer = &framebuffer,
	};

	loop(display, window, &renderer);

	ObscuraUnloadWorld(world);
	ObscuraDestroyWorld(&world);

	XShmDetach(display, &shm_info);
	XFree(framebuffer.image);
	shmdt(shm_info.shmaddr);

	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return 0;
}
