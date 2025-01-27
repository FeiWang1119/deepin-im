// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "DimTextInputV1.h"

#include "Key.h"

#include <private/qxkbcommon_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QTextCharFormat>
#include <QtCore/qloggingcategory.h>
#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qwindow.h>

Q_LOGGING_CATEGORY(qLcQpaWaylandTextInput, "qt.qpa.wayland.textinput")

DimTextInputV1::DimTextInputV1(struct ::zwp_dim_text_input_v1 *text_input)
    : wl::client::ZwpDimTextInputV1(text_input)
{
}

DimTextInputV1::~DimTextInputV1() { }

namespace {
const Qt::InputMethodQueries supportedQueries4 = Qt::ImEnabled | Qt::ImSurroundingText
    | Qt::ImCursorPosition | Qt::ImAnchorPosition | Qt::ImHints | Qt::ImCursorRectangle;
}

void DimTextInputV1::zwp_dim_text_input_v1_enter()
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;

    // m_surface = surface;

    m_pendingPreeditString.clear();
    m_pendingCommitString.clear();
    m_pendingDeleteBeforeText = 0;
    m_pendingDeleteAfterText = 0;
    m_entered = true;

    enable();
    updateState(supportedQueries4, update_state_enter);
}

void DimTextInputV1::zwp_dim_text_input_v1_leave()
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;

    // if (m_surface != surface) {
    //     qCWarning(qLcQpaWaylandTextInput()) << Q_FUNC_INFO << "Got leave event for surface"
    //                                         << surface << "focused surface" << m_surface;
    //     return;
    // }

    // QTBUG-97248: check commit_mode
    // Currently text-input-unstable-v4-wip is implemented with preedit_commit_mode
    // 'commit'

    m_currentPreeditString.clear();

    // m_surface = nullptr;
    m_entered = false;
    m_currentSerial = 0U;

    disable();
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "Done";
}

void DimTextInputV1::zwp_dim_text_input_v1_modifiers_map(struct wl_array *map)
{
    const QList<QByteArray> modifiersMap =
        QByteArray::fromRawData(static_cast<const char *>(map->data), map->size).split('\0');

    m_modifiersMap.clear();

    for (const QByteArray &modifier : modifiersMap) {
        if (modifier == "Shift")
            m_modifiersMap.append(Qt::ShiftModifier);
        else if (modifier == "Control")
            m_modifiersMap.append(Qt::ControlModifier);
        else if (modifier == "Alt")
            m_modifiersMap.append(Qt::AltModifier);
        else if (modifier == "Mod1")
            m_modifiersMap.append(Qt::AltModifier);
        else if (modifier == "Mod4")
            m_modifiersMap.append(Qt::MetaModifier);
        else
            m_modifiersMap.append(Qt::NoModifier);
    }
}

void DimTextInputV1::zwp_dim_text_input_v1_preedit_string(const char *text,
                                                          int32_t cursorBegin,
                                                          int32_t cursorEnd)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << text << cursorBegin << cursorEnd;

    if (!QGuiApplication::focusObject())
        return;

    m_pendingPreeditString.text = text;
    m_pendingPreeditString.cursorBegin = cursorBegin;
    m_pendingPreeditString.cursorEnd = cursorEnd;
}

void DimTextInputV1::zwp_dim_text_input_v1_commit_string(const char *text)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << text;

    if (!QGuiApplication::focusObject())
        return;

    m_pendingCommitString = text;
}

void DimTextInputV1::zwp_dim_text_input_v1_delete_surrounding_text(uint32_t beforeText,
                                                                   uint32_t afterText)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << beforeText << afterText;

    if (!QGuiApplication::focusObject())
        return;

    m_pendingDeleteBeforeText =
        QWaylandInputMethodEventBuilder::indexFromWayland(m_surroundingText, beforeText);
    m_pendingDeleteAfterText =
        QWaylandInputMethodEventBuilder::indexFromWayland(m_surroundingText, afterText);
}

void DimTextInputV1::zwp_dim_text_input_v1_done(uint32_t serial)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "with serial" << serial << m_currentSerial;

    // This is a case of double click.
    // text_input_v4 will ignore this done signal and just keep the selection of the clicked word.
    if (m_cursorPos != m_anchorPos
        && (m_pendingDeleteBeforeText != 0 || m_pendingDeleteAfterText != 0)) {
        qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "Ignore done";
        m_pendingDeleteBeforeText = 0;
        m_pendingDeleteAfterText = 0;
        m_pendingPreeditString.clear();
        m_pendingCommitString.clear();
        return;
    }

    QObject *focusObject = QGuiApplication::focusObject();
    if (!focusObject)
        return;

    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "PREEDIT" << m_pendingPreeditString.text
                                    << m_pendingPreeditString.cursorBegin;

    QList<QInputMethodEvent::Attribute> attributes;
    {
        if (m_pendingPreeditString.cursorBegin != -1 || m_pendingPreeditString.cursorEnd != -1) {
            // Current supported cursor shape is just line.
            // It means, cursorEnd and cursorBegin are the same.
            QInputMethodEvent::Attribute attribute1(QInputMethodEvent::Cursor,
                                                    m_pendingPreeditString.text.length(),
                                                    1);
            attributes.append(attribute1);
        }

        // only use single underline style for now
        QTextCharFormat format;
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        QInputMethodEvent::Attribute attribute2(QInputMethodEvent::TextFormat,
                                                0,
                                                m_pendingPreeditString.text.length(),
                                                format);
        attributes.append(attribute2);
    }
    QInputMethodEvent event(m_pendingPreeditString.text, attributes);

    qCDebug(qLcQpaWaylandTextInput)
        << Q_FUNC_INFO << "DELETE" << m_pendingDeleteBeforeText << m_pendingDeleteAfterText;
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "COMMIT" << m_pendingCommitString;

    // A workaround for reselection
    // It will disable redundant commit after reselection
    if (m_pendingDeleteBeforeText != 0 || m_pendingDeleteAfterText != 0)
        m_condReselection = true;

    event.setCommitString(m_pendingCommitString,
                          -m_pendingDeleteBeforeText,
                          m_pendingDeleteBeforeText + m_pendingDeleteAfterText);
    m_currentPreeditString = m_pendingPreeditString;
    m_pendingPreeditString.clear();
    m_pendingCommitString.clear();
    m_pendingDeleteBeforeText = 0;
    m_pendingDeleteAfterText = 0;
    QCoreApplication::sendEvent(focusObject, &event);

    if (serial == m_currentSerial) {
        updateState(supportedQueries4, update_state_full);
    }
}

void DimTextInputV1::zwp_dim_text_input_v1_keysym(
    uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers)
{
    // m_serial = serial;

    // if (m_resetCallback) {
    //     qCDebug(qLcQpaInputMethods()) << "discard keysym: reset not confirmed";
    //     return;
    // }

    if (!QGuiApplication::focusWindow()) {
        return;
    }

    Qt::KeyboardModifiers qtModifiers = modifiersToQtModifiers(modifiers);

    QEvent::Type type =
        state == WL_KEYBOARD_KEY_STATE_PRESSED ? QEvent::KeyPress : QEvent::KeyRelease;
    QString text = QXkbCommon::lookupStringNoKeysymTransformations(sym);
    int qtkey = keySymToQtKey(sym, text);

    QWindowSystemInterface::handleKeyEvent(QGuiApplication::focusWindow(),
                                           time,
                                           type,
                                           qtkey,
                                           qtModifiers,
                                           text);
}

Qt::KeyboardModifiers DimTextInputV1::modifiersToQtModifiers(uint32_t modifiers)
{
    Qt::KeyboardModifiers ret = Qt::NoModifier;
    for (int i = 0; i < m_modifiersMap.size(); ++i) {
        if (modifiers & (1 << i)) {
            ret |= m_modifiersMap[i];
        }
    }
    return ret;
}

void DimTextInputV1::reset()
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;

    m_pendingPreeditString.clear();
}

void DimTextInputV1::enable()
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;

    wl::client::ZwpDimTextInputV1::enable();
}

void DimTextInputV1::disable()
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;

    wl::client::ZwpDimTextInputV1::disable();
}

void DimTextInputV1::commit()
{
    m_currentSerial = (m_currentSerial < UINT_MAX) ? m_currentSerial + 1U : 0U;

    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "with serial" << m_currentSerial;
    ZwpDimTextInputV1::commit();
}

void DimTextInputV1::updateState(Qt::InputMethodQueries queries, uint32_t flags)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << queries << flags;

    if (flags == update_state_enter && !m_entered) {
        return;
    }

    if (!QGuiApplication::focusObject()) {
        return;
    }

    if (!QGuiApplication::focusWindow() || !QGuiApplication::focusWindow()->handle()) {
        return;
    }

    auto *window = QGuiApplication::focusWindow()->handle();

    queries &= supportedQueries4;
    bool needsCommit = false;

    QInputMethodQueryEvent event(queries);
    QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);

    // For some reason, a query for Qt::ImSurroundingText gives an empty string even though it is
    // not.
    if (!(queries & Qt::ImSurroundingText)
        && event.value(Qt::ImSurroundingText).toString().isEmpty()) {
        return;
    }

    if (queries & Qt::ImCursorRectangle) {
        const QRect &cRect = event.value(Qt::ImCursorRectangle).toRect();
        const QRect &windowRect =
            QGuiApplication::inputMethod()->inputItemTransform().mapRect(cRect);
        const QMargins margins = window->frameMargins();
        const QRect &surfaceRect = windowRect.translated(margins.left(), margins.top());
        if (surfaceRect != m_cursorRect) {
            set_cursor_rectangle(surfaceRect.x(),
                                 surfaceRect.y(),
                                 surfaceRect.width(),
                                 surfaceRect.height());
            m_cursorRect = surfaceRect;
            needsCommit = true;
        }
    }

    if ((queries & Qt::ImSurroundingText) || (queries & Qt::ImCursorPosition)
        || (queries & Qt::ImAnchorPosition)) {
        QString text = event.value(Qt::ImSurroundingText).toString();
        int cursor = event.value(Qt::ImCursorPosition).toInt();
        int anchor = event.value(Qt::ImAnchorPosition).toInt();

        qCDebug(qLcQpaWaylandTextInput)
            << "Orginal surrounding_text from InputMethodQuery: " << text << cursor << anchor;

        // Make sure text is not too big
        // surround_text cannot exceed 4000byte in wayland protocol
        // The worst case will be supposed here.
        const int MAX_MESSAGE_SIZE = 4000;

        if (text.toUtf8().size() > MAX_MESSAGE_SIZE) {
            const int selectionStart =
                QWaylandInputMethodEventBuilder::indexToWayland(text, qMin(cursor, anchor));
            const int selectionEnd =
                QWaylandInputMethodEventBuilder::indexToWayland(text, qMax(cursor, anchor));
            const int selectionLength = selectionEnd - selectionStart;
            // If selection is bigger than 4000 byte, it is fixed to 4000 byte.
            // anchor will be moved in the 4000 byte boundary.
            if (selectionLength > MAX_MESSAGE_SIZE) {
                if (anchor > cursor) {
                    const int length = MAX_MESSAGE_SIZE;
                    anchor = QWaylandInputMethodEventBuilder::trimmedIndexFromWayland(text,
                                                                                      length,
                                                                                      cursor);
                    anchor -= cursor;
                    text = text.mid(cursor, anchor);
                    cursor = 0;
                } else {
                    const int length = -MAX_MESSAGE_SIZE;
                    anchor = QWaylandInputMethodEventBuilder::trimmedIndexFromWayland(text,
                                                                                      length,
                                                                                      cursor);
                    cursor -= anchor;
                    text = text.mid(anchor, cursor);
                    anchor = 0;
                }
            } else {
                const int offset = (MAX_MESSAGE_SIZE - selectionLength) / 2;

                int textStart =
                    QWaylandInputMethodEventBuilder::trimmedIndexFromWayland(text,
                                                                             -offset,
                                                                             qMin(cursor, anchor));
                int textEnd =
                    QWaylandInputMethodEventBuilder::trimmedIndexFromWayland(text,
                                                                             MAX_MESSAGE_SIZE,
                                                                             textStart);

                anchor -= textStart;
                cursor -= textStart;
                text = text.mid(textStart, textEnd - textStart);
            }
        }
        qCDebug(qLcQpaWaylandTextInput)
            << "Modified surrounding_text: " << text << cursor << anchor;

        const int cursorPos = QWaylandInputMethodEventBuilder::indexToWayland(text, cursor);
        const int anchorPos = QWaylandInputMethodEventBuilder::indexToWayland(text, anchor);

        if (m_surroundingText != text || m_cursorPos != cursorPos || m_anchorPos != anchorPos) {
            qCDebug(qLcQpaWaylandTextInput)
                << "Current surrounding_text: " << m_surroundingText << m_cursorPos << m_anchorPos;
            qCDebug(qLcQpaWaylandTextInput)
                << "New surrounding_text: " << text << cursorPos << anchorPos;

            set_surrounding_text(text.toStdString().c_str(), cursorPos, anchorPos);

            // A workaround in the case of reselection
            // It will work when re-clicking a preedit text
            if (m_condReselection) {
                qCDebug(qLcQpaWaylandTextInput)
                    << "\"commit\" is disabled when Reselection by changing focus";
                m_condReselection = false;
                needsCommit = false;
            }

            m_surroundingText = text;
            m_cursorPos = cursorPos;
            m_anchorPos = anchorPos;
            m_cursor = cursor;
        }
    }

    if (queries & Qt::ImHints) {
        QWaylandInputMethodContentType contentType = QWaylandInputMethodContentType::convertV4(
            static_cast<Qt::InputMethodHints>(event.value(Qt::ImHints).toInt()));
        qCDebug(qLcQpaWaylandTextInput) << m_contentHint << contentType.hint;
        qCDebug(qLcQpaWaylandTextInput) << m_contentPurpose << contentType.purpose;

        if (m_contentHint != contentType.hint || m_contentPurpose != contentType.purpose) {
            qCDebug(qLcQpaWaylandTextInput)
                << "set_content_type: " << contentType.hint << contentType.purpose;
            set_content_type(contentType.hint, contentType.purpose);

            m_contentHint = contentType.hint;
            m_contentPurpose = contentType.purpose;
            needsCommit = true;
        }
    }

    if (needsCommit && (flags == update_state_change || flags == update_state_enter))
        commit();
}

QRectF DimTextInputV1::keyboardRect() const
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;
    return m_cursorRect;
}
