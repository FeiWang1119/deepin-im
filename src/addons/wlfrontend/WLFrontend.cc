#include "WLFrontend.h"

#include "WaylandConnection.h"
#include "utils.h"

#include <dimcore/Dim.h>
#include <wayland-input-method-unstable-v1-client-protocol.h>

#include <QDBusConnection>
#include <QDebug>

using namespace org::deepin::dim;

static const wl_registry_listener registry_listener = {
    CallbackWrapper<&WLFrontend::registryGlobal>::func,
    []([[maybe_unused]] void *data, [[maybe_unused]] struct wl_registry *registry, uint32_t name) {
        qWarning() << "global_remove" << name;
    },
};

static const zwp_input_method_v1_listener imListener = {
    CallbackWrapper<&WLFrontend::inputMethodActivate>::func,
    CallbackWrapper<&WLFrontend::inputMethodDeactivate>::func,
};

WLFrontend::WLFrontend()
{
    std::string waylandDisplay = getenv("WAYLAND_DISPLAY");
    if (waylandDisplay.empty()) {
        qDebug("WAYLAND_DISPLAY is not set");
        // todo: fake wayland server
    }

    wl_ = new WaylandConnection(waylandDisplay, this);

    auto *registry = wl_display_get_registry(wl_->display());
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_->roundTrip();
    wl_display_flush(wl_->display());

    zwp_input_method_v1_add_listener(input_method_v1_, &imListener, this);
}

WLFrontend::~WLFrontend() { }

void WLFrontend::registryGlobal(struct wl_registry *registry,
                                uint32_t name,
                                const char *interface,
                                uint32_t version)
{
    qWarning() << "global" << name << interface << version;

#define BIND(member, interface_type)                                             \
  if (strcmp(interface, #interface_type) == 0) {                                 \
    member = static_cast<interface_type *>(                                      \
        wl_registry_bind(registry, name, &interface_type##_interface, version)); \
    return;                                                                      \
  }

    BIND(input_method_v1_, zwp_input_method_v1);
    BIND(input_method_context_v1_, zwp_input_method_context_v1);
}

void WLFrontend::inputMethodActivate(
    [[maybe_unused]] struct zwp_input_method_v1 *zwp_input_method_v1,
    [[maybe_unused]] struct zwp_input_method_context_v1 *id)
{
    // icid_ = id;
}

void WLFrontend::inputMethodDeactivate(
    [[maybe_unused]] struct zwp_input_method_v1 *zwp_input_method_v1,
    [[maybe_unused]] struct zwp_input_method_context_v1 *id)
{
    // icid_ = nullptr;
}