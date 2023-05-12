#ifndef FCITX5PROXY_H
#define FCITX5PROXY_H

#include <dimcore/InputMethodAddon.h>

class DBusProvider;

class Fcitx5Proxy : public InputMethodAddon {
public:
    Fcitx5Proxy(Dim *dim);
    virtual ~Fcitx5Proxy();

    QList<InputMethodEntry> getInputMethods() override;

private:
    DBusProvider *dbusProvider_;
    bool available_;

    QList<InputMethodEntry> inputMethods_;

    void updateInputMethods();
};

#endif // !FCITX5PROXY_H