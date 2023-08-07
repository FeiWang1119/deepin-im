// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TextInputManagerV3.h"

#include "TextInputV3.h"
#include "qwayland-server-text-input-unstable-v3.h"

#include <QtWaylandCompositor/QWaylandSeat>

class TextInputManagerV3Private : public QtWaylandServer::zwp_text_input_manager_v3
{
public:
    TextInputManagerV3Private(TextInputManagerV3 *q)
        : q(q)
    {
    }

protected:
    void zwp_text_input_manager_v3_destroy(Resource *resource) override
    {
        wl_resource_destroy(resource->handle);
    }

    void zwp_text_input_manager_v3_get_text_input(Resource *resource,
                                                  uint32_t id,
                                                  struct ::wl_resource *seat) override
    {
        auto iter = q->textInputs_.find(seat);
        if (iter == q->textInputs_.end()) {
            auto *ti = new TextInputV3(q->core_, seat, q);
            auto [i, r] = q->textInputs_.emplace(seat, ti);
            iter = i;
        }

        iter->second->add(resource->client(), id);
    }

private:
    TextInputManagerV3 *q;
};

TextInputManagerV3::TextInputManagerV3(Core *core, QObject *parent)
    : QObject(parent)
    , core_(core)
    , d(new TextInputManagerV3Private(this))
{
}

TextInputManagerV3::~TextInputManagerV3() { }

INIT_FUNCS(TextInputManagerV3)

TextInputV3 *TextInputManagerV3::getTextInputV4BySeat(struct ::wl_resource *seat)
{
    return textInputs_.at(seat);
}