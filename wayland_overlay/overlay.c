#define _GNU_SOURCE
#include <wayland-client-protocol.h>
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

static struct wl_seat *seat = NULL;
static struct wl_keyboard *keyboard = NULL;

static void keyboard_keymap(void* data,
                            struct wl_keyboard *wl_keyboard,
                            uint format,
                            int fd, uint size) {
    close(fd);
    (void)data;
    (void)wl_keyboard;
    (void)format;
    (void)size;
}

static void keyboard_enter(void* data,
                           struct wl_keyboard* wl_keyboard,
                           uint serial,
                           struct wl_surface* surface,
                           struct wl_array* keys) {

    (void)data;
    (void)wl_keyboard;
    (void)serial;
    (void)surface;
    (void)keys;
}

static void keyboard_leave(void* data,
                           struct wl_keyboard* wl_keyboard, 
                           uint serial,
                           struct wl_surface* surface) {
    (void)data;
    (void)wl_keyboard;
    (void)serial;
    (void)surface;
}

static void keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
                        uint32_t serial, uint32_t time, uint32_t key,
                        uint32_t state) {
    (void)wl_keyboard; (void)serial; (void)time;
    
    // fprintf(stderr, "Key pressed %u\n", key);

    // kun når key er pressed (ikke released)
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        // fundet med key pressed ovenfor
        uint32_t h = 35;
        uint32_t j = 36;
        uint32_t k = 37;
        uint32_t l = 38;
        uint32_t up = 103;
        uint32_t down = 108;
        uint32_t left = 105;
        uint32_t right = 106;
        if (key == h || key == j || key == k || key == l ||
            key == up || key == down || key == left || key == right) {
            return;
        }
        
        // alle andre keys lukker vinduet
        wl_display_disconnect(display);
        exit(0);
    }
    (void)data;
}

static void keyboard_modifiers(void* data,
                               struct wl_keyboard* wl_keyboard, 
                               uint serial, 
                               uint mods_depressed, 
                               uint mods_latched, 
                               uint mods_locked, 
                               uint group) {

    (void)data;
    (void)wl_keyboard;
    (void)serial;
    (void)mods_depressed;
    (void)mods_latched;
    (void)mods_locked;
    (void)group;
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
};

static void layer_closed(void *data,
                         struct zwlr_layer_surface_v1 *lsurf) {
    // compositor siger luk
    wl_display_disconnect(display);
    exit(0);
    (void)data;
    (void)lsurf;
}

static struct wl_buffer *create_buffer(int w, int h) {
    // shm format er ARGB8888, 
    // så 8 bits per kanal aka 1byte per A, R, G og B
    int stride = w * 4;

    size_t size = (size_t)stride * (size_t)h;
    int fd = memfd_create("wayland_shm", 0);
    if (fd < 0) {
        fprintf(stderr, "memfd_create failed: %s\n", strerror(errno));
        return NULL;
    }

    if (ftruncate(fd, (off_t)size)<0) {
        fprintf(stderr, "ftruncate failed: %s\n", strerror(errno)); 
        close(fd);
        return NULL;   
    }

    shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        close(fd);
        shm_data = NULL;
        return NULL;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, (int)size);
    struct wl_buffer *buf = wl_shm_pool_create_buffer(
        pool, 0, w, h, stride, WL_SHM_FORMAT_ARGB8888
    );
    wl_shm_pool_destroy(pool);
    close(fd);

    shm_size = size;
    return buf;
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
    // fprintf(stderr, "configure: w=%u h=%u\n", width, height);

    if (!buffer) {
        buffer = create_buffer((int)w, (int)h);
        if (!buffer || !shm_data) {
            fprintf(stderr, "create_buffer failed\n");
            return;
        }
    }
    
    uint32_t *px = (uint32_t *)shm_data;
    for (int y = 0; y<height; y++) {
        for (int x = 0; x<width ; x++) {
            px[y*w +x] = 0xAA222222;
        }
    }

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, w, h);
    wl_surface_commit(surface);
     
    (void)data;
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
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
    }

    (void)version;
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
    // fprintf(stderr, " seat        = %p\n", (void*)seat);

    if (seat) {
        keyboard = wl_seat_get_keyboard(seat);
        // fprintf(stderr, "keyboard = %p\n", (void*)keyboard);
        if (keyboard) {
            wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
        }
    }

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

    zwlr_layer_surface_v1_set_keyboard_interactivity(
        layer_surface, 
        ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE
    );

    zwlr_layer_surface_v1_set_size(layer_surface, 600, 400);

    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, 0);
    
    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_listener, NULL);

    if (seat) {
        keyboard = wl_seat_get_keyboard(seat);
        if (keyboard) {
            wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
        }
    }

    wl_surface_commit(surface);

    while (wl_display_dispatch(display) != -1) {

    }

    wl_display_disconnect(display);
    return 0;
}
