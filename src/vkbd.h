/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#include <stdint.h>
#include <memory>

struct vkbd;

std::shared_ptr<vkbd> vkbd_init(const char *name);

void vkbd_mouse_move(const struct vkbd *vkbd, int x, int y);
void vkbd_mouse_move_abs(const struct vkbd *vkbd, int x, int y);
void vkbd_mouse_scroll(const struct vkbd *vkbd, int x, int y);

void vkbd_send_key(const struct vkbd *vkbd, uint8_t code, int state);
#endif
