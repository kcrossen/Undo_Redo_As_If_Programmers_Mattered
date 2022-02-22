/**************************************************************************
**
** Copyright (C) 2022 Ken Crossen, example expanded into useful application
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Redistributions in source code or binary form may not be sold.
**
**************************************************************************/

#include "PlainTextEdit.h"

extern
uint Keyboard_Modifiers;

PlainTextEdit::PlainTextEdit ( QWidget *parent ) : QPlainTextEdit(parent) {
    Long_Press_Milliseconds_Threshold = 500;

    Mouse_Press_Position = -1;
    Mouse_Pressed_Milliseconds = 0;

    Mouse_Release_Position = -1;
    Mouse_Released_Milliseconds = 0;

    Undo_Redo = new UndoRedo(this);
    Undo_Redo->Set_Focus_Widget(this);

    connect(this, SIGNAL(textChanged()), this, SLOT(Private_textChanged()));
    // connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(Private_cursorPositionChanged()));
}

void
PlainTextEdit::Set_Support_Long_Press ( bool New_Support_Long_Press ) {
    Support_Long_Press = New_Support_Long_Press;
}

void
PlainTextEdit::Set_Long_Press_Milliseconds_Threshold ( qint64 New_Long_Press_Milliseconds ) {
    Long_Press_Milliseconds_Threshold = New_Long_Press_Milliseconds;
}

void
PlainTextEdit::Set_Numeric_Thin_Spaces ( bool New_Numeric_Thin_Spaces ) {
    Numeric_Thin_Spaces = New_Numeric_Thin_Spaces;
}

#define QChar_TextCursorIndicator QChar(0x25b2)

QString
PlainTextEdit::toPlainText_Clean ( ) {
    return QPlainTextEdit::toPlainText().remove(QChar_TextCursorIndicator);
}

QString
PlainTextEdit::toPlainText_Clean_No_ThinSpace ( ) {
    return QPlainTextEdit::toPlainText().remove(QChar_TextCursorIndicator).remove(Unicode_Thin_Space);
}

//void
//PlainTextEdit::Private_cursorPositionChanged ( ) {

//}

// Must ambush/subvert these ...
void
PlainTextEdit::undo ( ) {
    // QPlainTextEdit::undo();
    Suppress_PlainTextChanged = true;
    Undo_Redo->Execute_Undo();
    Suppress_PlainTextChanged = false;
}

void
PlainTextEdit::redo ( ) {
    // QPlainTextEdit::redo();
    Suppress_PlainTextChanged = true;
    Undo_Redo->Execute_Redo();
    Suppress_PlainTextChanged = false;
}

void
PlainTextEdit::copy ( ) {
    QPlainTextEdit::copy();
}

void
PlainTextEdit::cut ( ) {
    Undo_Redo->Push_Undo();
    QPlainTextEdit::cut();
}

void
PlainTextEdit::paste ( ) {
    Undo_Redo->Push_Undo();
    QPlainTextEdit::paste();
}
// ... Must ambush/subvert these

void
PlainTextEdit::insertPlainText(const QString &Text) {
    Undo_Redo->Redo_Stack_Clear();

    if (Undo_Redo->Deferred_Push_Undo or (Undo_Redo->Undo_Stack_Count() == 0)) {
        Undo_Redo->Push_Undo();
    }
    else if (Undo_Redo->Do_State.length() > 0) {
        if (Text.length() > 1) {
            Undo_Redo->Push_Undo();
        }
        else {
            if ((not Undo_Redo->Is_Identifier_Or_Number(Undo_Redo->Do_State.at(Undo_Redo->Do_State.length() - 1))) and
                Undo_Redo->Is_Identifier_Or_Number(Text.at(0))) {
                Undo_Redo->Push_Undo();
            }
        }
    }

    QPlainTextEdit::insertPlainText(Text);
    Undo_Redo->Do_State += Text;
}

void
PlainTextEdit::Delete_Previous_Character ( ) {
    Undo_Redo->Push_Undo();

    QTextCursor txt_cursor = this->textCursor();
    txt_cursor.deletePreviousChar();
    this->setTextCursor(txt_cursor);
}

void
PlainTextEdit::keyPressEvent ( QKeyEvent *event ) {
    emit keyPressed(event->key());

    bool event_already_handled = Undo_Redo->keyPressEvent_Handler(event);
    if (event_already_handled) {
        // Do not allow normal event handling
        event->accept();
    }
    else {
        // Normal event handling
        QPlainTextEdit::keyPressEvent(event);
    }
}

void
PlainTextEdit::keyReleaseEvent ( QKeyEvent *event ) {
    // Can't use Undo_Redo->keyReleaseEvent_Handler ...
    // ... need to wrap w/ suppression of text change
    if (event->matches(QKeySequence::Undo)) {
        this->undo();
        // Do not allow normal event handling
        event->accept();
        // Otherwise normal undo will trash text
    }
    else if (event->matches(QKeySequence::Redo)) {
        this->redo();
        // Do not allow normal event handling
        event->accept();
        // Otherwise normal undo will trash text
    }
    else {
        QPlainTextEdit::keyReleaseEvent(event);
    }
}

void
PlainTextEdit::Move_Cursor ( QTextCursor::MoveOperation Move_Operation ) {
    // The goal here is to avoid a series of cursor movment ...
    // ... pushes to the undo stack.
    Undo_Redo->Deferred_Push_Undo = true;

    QTextCursor txt_cursor = this->textCursor();
    txt_cursor.movePosition(Move_Operation);
    this->setTextCursor(txt_cursor);
}

// Dynamically manages digit grouping ...
void
PlainTextEdit::Private_textChanged ( ) {
    if (Suppress_PlainTextChanged) return;

    if (Numeric_Thin_Spaces) {
        Suppress_PlainTextChanged = true;

        // What about hexadecimal & binary numbers?
        QString plain_txt = this->toPlainText();
        QTextCursor txt_cursor = this->textCursor();
        int cursor_position = txt_cursor.position();

        // Number must start with a digit
        QRegExp decimal_number_rx = QRegExp("[0-9\\x2009]+[.]?[0-9\\x2009]*");

        // Locate integer part
        int begin_number_position = cursor_position;
        int end_number_position = cursor_position;
        QString test_number = "";
        QString test_txt;
        while (begin_number_position > 0) {
            test_txt = plain_txt.at(begin_number_position - 1) + test_number;
            if ((test_txt == ".") or
                decimal_number_rx.exactMatch(test_txt)) {
                test_number = test_txt;
                begin_number_position -= 1;
            }
            else break;
        }

        // Locate fractional part, if any
        while (end_number_position < plain_txt.length()) {
            test_txt.append(plain_txt.at(end_number_position));
            if (decimal_number_rx.exactMatch(test_txt)) {
                test_number = test_txt;
                end_number_position += 1;
            }
            else break;
        }

        if (((begin_number_position == 0) or
             (not (plain_txt.at(begin_number_position - 1).isLetterOrNumber() or
                  (plain_txt.at(begin_number_position - 1) == "_")))) and
            ((end_number_position == plain_txt.length()) or
             (not (plain_txt.at(end_number_position + 1).isLetterOrNumber() or
                  (plain_txt.at(end_number_position + 1) == "_"))))) {
            // Syntactically "isolated" now
            QString number = test_number.remove(Unicode_Thin_Space);
            if (decimal_number_rx.exactMatch(number)) {
                // Insert integer part digit grouping separator
                int decimal_point_position = number.indexOf(".");
                if (decimal_point_position < 0) decimal_point_position = number.length();
                int ch_idx = decimal_point_position - 1;
                while (ch_idx >= 3) {
                    ch_idx = ch_idx - 2;
                    number.insert(ch_idx, Unicode_Thin_Space);
                    ch_idx = ch_idx - 1;
                }
                // Insert fractional part digit grouping separator
                decimal_point_position = number.indexOf(".");
                if (decimal_point_position < 0) decimal_point_position = number.length();
                ch_idx = decimal_point_position + 1;
                while (ch_idx <= (number.length() - 3)) {
                    ch_idx = ch_idx + 3;
                    number.insert(ch_idx, Unicode_Thin_Space);
                    // Pass the thin space
                    ch_idx = ch_idx + 1;
                }

                txt_cursor.setPosition(begin_number_position, QTextCursor::MoveAnchor);
                txt_cursor.setPosition(end_number_position, QTextCursor::KeepAnchor);
                txt_cursor.removeSelectedText();
                txt_cursor.setPosition(begin_number_position, QTextCursor::MoveAnchor);
                this->setTextCursor(txt_cursor);
                this->insertPlainText(number);
            }
        }
        else {
            // Number must start with "0x or "0b", ...
            // ... but cut some slack searching for start of number
            QRegExp find_hexadecimal_number_rx = QRegExp("x?[0-9A-F\\x2009]+[.]?[0-9A-F\\x2009]*", Qt::CaseInsensitive);
            QRegExp find_binary_number_rx = QRegExp("b?[01\\x2009]+[.]?[01\\x2009]*", Qt::CaseInsensitive);

            QRegExp hexadecimal_number_rx = QRegExp("0x[0-9A-F\\x2009]+[.]?[0-9A-F\\x2009]*", Qt::CaseInsensitive);
            QRegExp binary_number_rx = QRegExp("0b[01\\x2009]+[.]?[01\\x2009]*", Qt::CaseInsensitive);

            // Locate integer part
            int begin_number_position = cursor_position;
            int end_number_position = cursor_position;
            QString test_number = "";
            QString test_txt;
            while (begin_number_position > 0) {
                test_txt = plain_txt.at(begin_number_position - 1) + test_number;
                if ((test_txt == ".") or
                    find_hexadecimal_number_rx.exactMatch(test_txt) or
                    hexadecimal_number_rx.exactMatch(test_txt) or
                    find_binary_number_rx.exactMatch(test_txt) or
                    binary_number_rx.exactMatch(test_txt)) {
                    test_number = test_txt;
                    begin_number_position -= 1;
                }
                else break;
            }

            // Locate fractional part, if any
            while (end_number_position < plain_txt.length()) {
                test_txt = test_number + plain_txt.at(end_number_position);
                if (hexadecimal_number_rx.exactMatch(test_txt) or
                    binary_number_rx.exactMatch(test_txt)) {
                    test_number = test_txt;
                    end_number_position += 1;
                }
                else break;
            }

            if (((begin_number_position == 0) or
                 (not (plain_txt.at(begin_number_position - 1).isLetterOrNumber() or
                      (plain_txt.at(begin_number_position - 1) == "_")))) and
                ((end_number_position == plain_txt.length()) or
                 (not (plain_txt.at(end_number_position + 1).isLetterOrNumber() or
                      (plain_txt.at(end_number_position + 1) == "_"))))) {
                // Syntactically "isolated" now
                QString number = test_number.remove(Unicode_Thin_Space);
                if ((hexadecimal_number_rx.exactMatch(number)) or
                    (binary_number_rx.exactMatch(number))) {
                    // Insert integer part digit grouping separator
                    int decimal_point_position = number.indexOf(".");
                    if (decimal_point_position < 0) decimal_point_position = number.length();
                    int ch_idx = decimal_point_position - 1;
                    // Take "0b" or "0x" into account
                    while (ch_idx >= (2 /* "0b" or "0x" */ + 4)) {
                        ch_idx = ch_idx - 3;
                        number.insert(ch_idx, Unicode_Thin_Space);
                        ch_idx = ch_idx - 1;
                    }
                    // Insert fractional part digit grouping separator
                    decimal_point_position = number.indexOf(".");
                    if (decimal_point_position < 0) decimal_point_position = number.length();
                    ch_idx = decimal_point_position + 1;
                    while (ch_idx <= (number.length() - 4)) {
                        ch_idx = ch_idx + 4;
                        number.insert(ch_idx, Unicode_Thin_Space);
                        // Pass the thin space
                        ch_idx = ch_idx + 1;
                    }

                    txt_cursor.setPosition(begin_number_position, QTextCursor::MoveAnchor);
                    txt_cursor.setPosition(end_number_position, QTextCursor::KeepAnchor);
                    txt_cursor.removeSelectedText();
                    txt_cursor.setPosition(begin_number_position, QTextCursor::MoveAnchor);
                    this->setTextCursor(txt_cursor);
                    this->insertPlainText(number);
                }
            }
        }

        Suppress_PlainTextChanged = false;
    }

    emit PlainTextChanged();
}
// ... Dynamically manages digit grouping

void
PlainTextEdit::focusInEvent ( QFocusEvent *event ) {
    if (event->reason() == Qt::MouseFocusReason) emit focusIn();
    QPlainTextEdit::focusInEvent(event);
    // if (not Has_Focus) {
    //     Has_Focus = true;
    //     emit changedFocus(Has_Focus);
    // }
    // else Has_Focus = true;
}

void
PlainTextEdit::focusOutEvent ( QFocusEvent *event ) {
    if (event->reason() == Qt::MouseFocusReason) emit focusOut();
    QPlainTextEdit::focusOutEvent(event);
    // if (not Has_Focus) {
    //     Has_Focus = false;
    //     emit changedFocus(Has_Focus);
    // }
    // else Has_Focus = false;
}

QChar
PlainTextEdit::TextCursorIndicator ( ) {
    return QChar_TextCursorIndicator;
}

void
PlainTextEdit::insertTextCursorIndicator ( ) {
    Suppress_PlainTextChanged = true;
    if (QPlainTextEdit::toPlainText().length() > 0) {
        QPlainTextEdit::insertPlainText(QChar_TextCursorIndicator);
        QPlainTextEdit::moveCursor(QTextCursor::Left, QTextCursor::KeepAnchor);
    }
    Suppress_PlainTextChanged = false;
}

void
PlainTextEdit::removeTextCursorIndicator ( ) {
    Suppress_PlainTextChanged = true;
    QTextCursor txtcur = QPlainTextEdit::textCursor();
    QPlainTextEdit::moveCursor(QTextCursor::End);
    if (QPlainTextEdit::find(QChar_TextCursorIndicator, QTextDocument::FindBackward))
        QPlainTextEdit::insertPlainText("");
    else QPlainTextEdit::setTextCursor(txtcur);
    Suppress_PlainTextChanged = false;
}

void
PlainTextEdit::mousePressEvent ( QMouseEvent* event ) {
    Mouse_Pressed_Milliseconds = QDateTime::currentMSecsSinceEpoch();
    Mouse_Press_Cursor_Position = event->pos();

    // Mouse_Press_Select_Cursor = this->textCursor();

    QTextCursor press_cursor = cursorForPosition(event->pos());
    Mouse_Press_Position = press_cursor.position();

    Mouse_Press_Cursor_Y = event->y();
    QScrollBar* vertical_scrollbar = this->verticalScrollBar();
    Mouse_Press_Vertical_ScrollBar_Value = vertical_scrollbar->value();

    // The goal here is to avoid a series of cursor movment ...
    // ... pushes to the undo stack.
    Undo_Redo->Deferred_Push_Undo = true;

    // Allow super class (normal) handling of event
    QPlainTextEdit::mousePressEvent(event);
}

void
PlainTextEdit::mouseMoveEvent ( QMouseEvent* event ) {
#if defined(Q_OS_ANDROID)
    int cursor_delta_position = event->y() - Mouse_Press_Cursor_Y;
    QScrollBar* vertical_scrollbar = this->verticalScrollBar();
    int scrollbar_height = vertical_scrollbar->height();
    int scrollbar_min_value = vertical_scrollbar->minimum();
    int scrollbar_max_value = vertical_scrollbar->maximum();
    int scrollbar_page_step = vertical_scrollbar->pageStep();

    int delta_value =
          int((0.999 * cursor_delta_position *
               (scrollbar_max_value - scrollbar_min_value + scrollbar_page_step)) /
              scrollbar_height);
    if (Grab_Scroll_Document) delta_value = -delta_value;

    // qDebug() << cursor_delta_position
    //          << scrollbar_max_value << scrollbar_min_value
    //          << scrollbar_height << scrollbar_page_step << delta_value;

    int new_value = Mouse_Press_Vertical_ScrollBar_Value + delta_value;
    vertical_scrollbar->setValue(new_value);

    // Do not allow normal event handling
    event->accept();
    // Because otherwise we'll also be selecting
#else
    // Allow super class (normal) handling of event
    QPlainTextEdit::mouseMoveEvent(event);
#endif
}

void
PlainTextEdit::mouseReleaseEvent ( QMouseEvent* event ) {
    if (not Support_Long_Press) {
        // Allow super class (normal) handling of event
        QPlainTextEdit::mouseReleaseEvent(event);
    }
    else {
        Mouse_Released_Milliseconds = QDateTime::currentMSecsSinceEpoch();

        QTextCursor release_cursor = cursorForPosition(event->pos());
        Mouse_Release_Position = release_cursor.position();

        qint64 pressed_to_released_delta_milliseconds =
                 Mouse_Released_Milliseconds - Mouse_Pressed_Milliseconds;

        if (pressed_to_released_delta_milliseconds > Long_Press_Milliseconds_Threshold) {
            // This is a long press ...
            if (Mouse_Release_Position == Mouse_Press_Position) {
                // Long press (for mouseless UI) ...
                // ... but forst restore original selection.
                // this->setTextCursor(Mouse_Press_Select_Cursor);

                emit longPressed(Mouse_Press_Position);

                // // Do not allow normal event handling
                // event->accept();
                // Allow super class (normal) handling of event
                QPlainTextEdit::mouseReleaseEvent(event);
            }
            else {
                // // Do not allow normal event handling
                // event->accept();
                // Allow super class (normal) handling of event
                QPlainTextEdit::mouseReleaseEvent(event);
            }
        }
        else {
            // Allow super class (normal) handling of event
            QPlainTextEdit::mouseReleaseEvent(event);

            if (Mouse_Release_Position == Mouse_Press_Position) {
                QTextCursor txt_cursor = this->textCursor();
                txt_cursor.setPosition(Mouse_Press_Position);
                this->setTextCursor(txt_cursor);
            }
            Has_Focus = true;
            emit changedFocus(Has_Focus);
        }
    }
}

void
PlainTextEdit::Custom_ContextMenu ( const QPoint &Mouse_Cursor_Poition ) {
    QMenu* contextMenu = new QMenu ( this );
    Q_CHECK_PTR ( contextMenu );

    if (this->isReadOnly()) {
        contextMenu->addAction("Copy selection", this, SLOT(onContextCopySelection()));
        contextMenu->addAction("Copy line", this, SLOT(onContextCopyLine()));
        contextMenu->addAction("Copy word", this, SLOT(onContextCopyWord()));
        contextMenu->addAction("Copy all", this, SLOT(onContextCopyAll()));

    }
    else {
        contextMenu->addAction("Paste", this, SLOT(onContextPaste()));

        contextMenu->addSeparator();

        contextMenu->addAction("Copy selection", this, SLOT(onContextCopySelection()));
        contextMenu->addAction("Cut selection", this, SLOT(onContextCutSelection()));
        contextMenu->addAction("Delete selection", this, SLOT(onContextDeleteSelection()));

        contextMenu->addSeparator();

        contextMenu->addAction("Copy line", this, SLOT(onContextCopyLine()));
        contextMenu->addAction("Copy word", this, SLOT(onContextCopyWord()));
        contextMenu->addAction("Copy all", this, SLOT(onContextCopyAll()));
    }

    contextMenu->exec(Mouse_Cursor_Poition);
    delete contextMenu;
    contextMenu = nullptr;
}

void
PlainTextEdit::onContextCopySelection ( ) {
    QTextCursor txt_cursor = this->textCursor();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(txt_cursor.selectedText());
}

void
PlainTextEdit::onContextCutSelection ( ) {
    QTextCursor txt_cursor = this->textCursor();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(txt_cursor.selectedText());

    Undo_Redo->Push_Undo();
    txt_cursor.removeSelectedText();
}

void
PlainTextEdit::onContextDeleteSelection ( ) {
    Undo_Redo->Push_Undo();
    this->textCursor().removeSelectedText();
}

void
PlainTextEdit::onContextCopyLine ( ) {
    QTextCursor txt_cursor = this->textCursor();
    txt_cursor.select(QTextCursor::LineUnderCursor);
    this->setTextCursor(txt_cursor);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(txt_cursor.selectedText());
}

void
PlainTextEdit::onContextCopyWord ( ) {
    QTextCursor txt_cursor = this->textCursor();
    txt_cursor.select(QTextCursor::WordUnderCursor);
    this->setTextCursor(txt_cursor);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(txt_cursor.selectedText());
}

void
PlainTextEdit::onContextCopyAll ( ) {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(this->toPlainText());
}

void
PlainTextEdit::onContextPaste ( ) {
    QClipboard *clipboard = QApplication::clipboard();
    QString clipboard_text = clipboard->text(QClipboard::Clipboard);
    // If clipboard text has the form "12345.6789 { blah, blah, blah }" ...
    // ... paste "{ blah, blah, blah }" into Algebraic_PlainTextEdit
    // ... else paste clipboard text into Algebraic_PlainTextEdit
    QRegExp brace_enclosed_expression_rx("^[^\\{]+\\{(.+)\\}$", Qt::CaseInsensitive, QRegExp::RegExp);
    int match_position = brace_enclosed_expression_rx.indexIn(clipboard_text);
    QStringList brace_enclosed_expression_list = brace_enclosed_expression_rx.capturedTexts();
    if ((match_position >= 0) and (brace_enclosed_expression_list.count() > 1)) {
        this->insertPlainText(brace_enclosed_expression_list[1]);
    }
    else {
        this->insertPlainText(clipboard_text);
    }
}
