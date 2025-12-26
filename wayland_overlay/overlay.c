#include <wayland-client.h>
#include "wlr-layer-shell.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



// her gemmer vi de “services” vi finder
static struct wl_display *display;

static struct wl_compositor *compositor = NULL;
static struct wl_shm *shm = NULL;
static struct zwlr_layer_shell_v1 *layer_shell = NULL;

static struct wl_surface *surface = NULL;
static struct zwlr_layer_surface_v1 *layer_surface = NULL;

static struct wl_buffer *buffer = NULL;
static void *shm_data = NULL;
static size_t shm_size = 0;

static int width = 600;
static int height = 400;


static void layer_closed(void *data,
                         struct zwlr_layer_surface_v1 *lsurf) {
    // compositor siger luk
    wl_display_disconnect(display);
    exit(0);
}

static void layer_configure(void *data,
                            struct zwlr_layer_surface_v1 *lsurf,
                            uint32_t serial,
                            uint32_t w,
                            uint32_t h) {

    zwlr_layer_surface_v1_ack_configure(lsurf, serial);
    if (w == 0) w = 600;
    if (h == 0) h = 400;
    
    width = w;
    height = h;
    fprintf(stderr, "configure: w=%u h=%u\n", width, height);

}


static const struct zwlr_layer_surface_v1_listener layer_listener = {
    .configure = layer_configure,
    .closed = layer_closed,
};


// Denne callback bliver kaldt én gang per “global” som compositoren annoncerer.
// interface = navnet (fx "wl_compositor")
// id = serverens handle for den service
// version = max version compositoren kan

static void registry_global(void *data,
                            struct wl_registry *registry,
                            uint32_t id,
                            const char *interface,
                            uint32_t version)
{
    (void)data;

    // // debug: bare for at se hvad der bliver annonceret
    // fprintf(stderr, "global: interface=%s id=%u version=%u\n", interface, id, version);

    // hvis det er en service vi vil bruge, så binder vi til den
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);

    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);

    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        layer_shell = wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1);
    }
}

static void registry_remove(void *data,
                            struct wl_registry *registry,
                            uint32_t id)
{
    (void)data; (void)registry; (void)id;
}

static const struct wl_registry_listener reg_listener = {
    .global = registry_global,
    .global_remove = registry_remove,
};

int main(void)
{
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland display.\n");
        return 1;
    }

    // Få registry-objektet (annonce-kanalen)
    struct wl_registry *registry = wl_display_get_registry(display);

    // Sig: “kald mine callbacks når der kommer globals”
    wl_registry_add_listener(registry, &reg_listener, NULL);

    // Pump events en gang, så compositoren når at annoncere globals
    wl_display_roundtrip(display);

    // fprintf(stderr, "resultat:\n");
    // fprintf(stderr, " compositor  = %p\n", (void*)compositor);
    // fprintf(stderr, " shm         = %p\n", (void*)shm);
    // fprintf(stderr, " layer_shell = %p\n", (void*)layer_shell);

    if (!compositor || !shm || !layer_shell) {
        fprintf(stderr, "Missing globals: compositor=%p shm=%p layer_shell=%p\n",
                (void*)compositor, (void*)shm, (void*)layer_shell);
        return 1;
    }
    
    surface = wl_compositor_create_surface(compositor);

    layer_surface = zwlr_layer_shell_v1_get_layer_surface (
        layer_shell,
        surface,
        NULL,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
        "layer surface keybinds"
    );
    
    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_listener, NULL);

    wl_surface_commit(surface);

    while (wl_display_dispatch(display) != -1) {

    }

    wl_display_disconnect(display);
    return 0;
}
