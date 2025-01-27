// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WL_TYPE_H
#define WL_TYPE_H

#include "wayland-input-method-unstable-v2-client-protocol.h"
#include "wayland-virtual-keyboard-unstable-v1-client-protocol.h"
#include "wayland-text-input-unstable-v3-client-protocol.h"
#include "wayland-dim-text-input-unstable-v1-client-protocol.h"

#include <wayland-client-protocol.h>

#include <string>

#include <stdint.h>

namespace wl {

template<typename T>
class Type
{
public:
    using raw_type = T;
    static inline const std::string interface;
    static inline const struct wl_interface *wl_interface;
};

#define INIT_WL_TYPE(type)                                \
  template<>                                              \
  inline const std::string Type<type>::interface = #type; \
                                                          \
  template<>                                              \
  inline const struct wl_interface *Type<type>::wl_interface = &type##_interface;

INIT_WL_TYPE(wl_compositor);
INIT_WL_TYPE(wl_surface);
INIT_WL_TYPE(wl_seat);
INIT_WL_TYPE(wl_keyboard);
INIT_WL_TYPE(zwp_input_method_manager_v2);
INIT_WL_TYPE(zwp_input_method_v2);
INIT_WL_TYPE(zwp_input_popup_surface_v2);
INIT_WL_TYPE(zwp_virtual_keyboard_manager_v1);
INIT_WL_TYPE(zwp_virtual_keyboard_v1);
INIT_WL_TYPE(zwp_input_method_keyboard_grab_v2);
INIT_WL_TYPE(zwp_text_input_manager_v3);
INIT_WL_TYPE(zwp_text_input_v3);
INIT_WL_TYPE(zwp_dim_text_input_manager_v1);
INIT_WL_TYPE(zwp_dim_text_input_v1);

} // namespace wl

#endif // !WL_TYPE_H
