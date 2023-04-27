#include "InputMethodEntry.h"

class InputMethodEntryPrivate {
public:
    const QString &uniqueName;
    const QString &name;
    const QString &description;
    const QString &label;
    const QString &iconName;
};

InputMethodEntry::InputMethodEntry(const QString &uniqueName,
                                   const QString &name,
                                   const QString &description,
                                   const QString &label,
                                   const QString &iconName)
    : d(new InputMethodEntryPrivate{uniqueName, name, description, label, iconName}) {
}

const QString &InputMethodEntry::uniqueName() const {
    return d->uniqueName;
}

const QString &InputMethodEntry::name() const {
    return d->name;
}

const QString &InputMethodEntry::description() const {
    return d->description;
}

const QString &InputMethodEntry::label() const {
    return d->label;
}

const QString &InputMethodEntry::iconName() const {
    return d->iconName;
}