// SPDX-FileCopyrightText: 2021 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROXYADDON_H
#define PROXYADDON_H

#include "Addon.h"
#include "InputMethodEntry.h"

#include <QList>

namespace org {
namespace deepin {
namespace dim {

class ProxyAddon : public Addon
{
    Q_OBJECT

public:
    explicit ProxyAddon(Dim *dim, const QString &key);
    virtual ~ProxyAddon();

    virtual QList<InputMethodEntry> getInputMethods() = 0;
    virtual void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) = 0;
    virtual void focusIn(uint32_t id) = 0;
    virtual void focusOut(uint32_t id) = 0;
    virtual void destroyed(uint32_t id) = 0;
    virtual void createInputContext(uint32_t id, const QString &appName) = 0;
};

} // namespace dim
} // namespace deepin
} // namespace org

#endif // PROXYADDON_H
