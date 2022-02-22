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

#include <QObject>

#include "UndoRedo.h"
#include "LineEdit.h"
#include "PlainTextEdit.h"

extern
uint Keyboard_Modifiers;

UndoRedo::UndoRedo ( QObject *parent ) : QObject(parent) {
    // This supports less aggressive undo/redo command compression.
    // Normally any adjacent insert operations are treated as a single ...
    // ... undoable/redoable operation. When inserts are being made in ...
    // ... application-logical chunks, they should be undoable/redoable ...
    // ... separately.

    //Undo/redo of operations performed on the document can be controlled using the setUndoRedoEnabled() function.
    // The undo/redo system can be controlled by an editor widget through the undo() and redo() slots;
    // the document also provides contentsChanged() , undoAvailable() , and redoAvailable() signals that
    // inform connected editor widgets about the state of the undo/redo system.
    // The following are the undo/redo operations of a QTextDocument :
    //   Insertion or removal of characters. A sequence of insertions or removals within the same text block are regarded
    //     as a single undo/redo operation.
    //   Insertion or removal of text blocks. Sequences of insertion or removals in a single operation (e.g., by selecting
    //     and then deleting text) are regarded as a single undo/redo operation.
    //Text character format changes.
    //Text block format changes.
    //Text block group format changes.

    // The goal is to catch every important change in the "text box" w/o ...
    // ... assuming that all adjacent character insertions are to be ...
    // ... treated as a single undo/redo unit as detailed above.
    // Generally assumes that insertion sequences are to be treated as ...
    // ... as series of identifiers or numbers, each such to be treated ...
    // ... as a separate undo/redo unit.
}

void
UndoRedo::Set_Focus_Widget ( QWidget *New_Focus_Widget ) {
    Focus_Widget = New_Focus_Widget;

    Focus_LineEdit = qobject_cast<LineEdit*>(Focus_Widget);
    Focus_PlainTextEdit = qobject_cast<PlainTextEdit*>(Focus_Widget);
}

// These must be "native" to this widget ...
UndoRedo::Text_State
UndoRedo::Save_Text_State ( ) {
    Text_State current_state;

    if (not (Focus_LineEdit == nullptr)) {
        current_state.Text = Focus_LineEdit->text();
        current_state.Select_Begin = Focus_LineEdit->selectionStart();
        current_state.Select_End = Focus_LineEdit->selectionEnd();
        current_state.Cursor_Position = Focus_LineEdit->cursorPosition();
    }
    else if (not (Focus_PlainTextEdit == nullptr)) {
        current_state.Text = Focus_PlainTextEdit->toPlainText_Clean();
        current_state.Select_Begin = Focus_PlainTextEdit->textCursor().selectionStart();
        current_state.Select_End = Focus_PlainTextEdit->textCursor().selectionEnd();
        current_state.Cursor_Position = Focus_PlainTextEdit->textCursor().position();
    }

    return current_state;
}

void
UndoRedo::Restore_Text_State ( Text_State New_Text_State ) {
    if (not (Focus_LineEdit == nullptr)) {
        Focus_LineEdit->setText(New_Text_State.Text);
        Focus_LineEdit->setCursorPosition(New_Text_State.Cursor_Position);
    }
    else if (not (Focus_PlainTextEdit == nullptr)) {
        Focus_PlainTextEdit->setPlainText(New_Text_State.Text);
        QTextCursor txt_cursor = Focus_PlainTextEdit->textCursor();
        txt_cursor.setPosition(New_Text_State.Cursor_Position);
        Focus_PlainTextEdit->setTextCursor(txt_cursor);
    }
}

int
UndoRedo::Selected_Count ( ) {
    if (not (Focus_LineEdit == nullptr)) return Focus_LineEdit->selectionLength();
    else if (not (Focus_PlainTextEdit == nullptr)) Focus_PlainTextEdit->textCursor().selectedText().length();
    return 0;
}

// ... These must be "native" to this widget


// Begin ...
void
UndoRedo::Push_State ( Stack_Selector Select_Stack ) {
    Text_State current_state = Save_Text_State();

    if (Select_Stack == Select_Undo)
        if (Undo_Stack.count() == 0)
            Undo_Stack.push(current_state);
        else {
            Text_State previous_state = Undo_Stack.last();
            // Prevent double pushing, push only if state is different ...
            // ... worry less about "trash" on stack
            if (not ((current_state.Text == previous_state.Text) and
                     (current_state.Cursor_Position == previous_state.Cursor_Position) and
                     (current_state.Select_Begin == previous_state.Select_Begin) and
                     (current_state.Select_End == previous_state.Select_End))) {
                Undo_Stack.push(current_state);
            }
        }
    else if (Select_Stack == Select_Redo)
        Redo_Stack.push(current_state);
}

void
UndoRedo::Pop_State ( Stack_Selector Select_Stack ) {
    Text_State current_state;
    if (Select_Stack == Select_Undo)
        current_state = Undo_Stack.pop();
    else if (Select_Stack == Select_Redo)
        current_state = Redo_Stack.pop();

    Restore_Text_State(current_state);
}

void
UndoRedo::Undo_Stack_Clear ( ) {
    Undo_Stack.clear();
}

void
UndoRedo::Redo_Stack_Clear ( ) {
    Redo_Stack.clear();
}

int
UndoRedo::Undo_Stack_Count ( ) {
    return Undo_Stack.count();
}

int
UndoRedo::Redo_Stack_Count ( ) {
    return Redo_Stack.count();
}


void
UndoRedo::Push_Undo ( ) {
    Deferred_Push_Undo = false;

    while (Undo_Stack.count() >= Maximum_Undo_Stack_Count) Undo_Stack.removeFirst();

    Redo_Stack_Clear();
    Push_State(Select_Undo);

    Do_State = "";
}

void
UndoRedo::Execute_Undo ( ) {
    if (Undo_Stack.count() > 0) {
        // Make sure we can get back to where we are
        Push_State(Select_Redo);
        Pop_State(Select_Undo);
        Do_State = "";
    }
}

void
UndoRedo::Execute_Redo ( ) {
    if (Redo_Stack.count() > 0) {
        // Make sure we can get back to where we are
        Push_State(Select_Undo);
        Pop_State(Select_Redo);
        Do_State = "";
    }
}

// This supports less aggressive undo/redo command compression.
// Normally any adjacent insert operations are treated as a single ...
// ... undoable/redoable operation. When inserts are being made in ...
// ... application-logical chunks, they should be undoable/redoable ...
// ... separately.

//Undo/redo of operations performed on the document can be controlled using the setUndoRedoEnabled() function.
// The undo/redo system can be controlled by an editor widget through the undo() and redo() slots;
// the document also provides contentsChanged() , undoAvailable() , and redoAvailable() signals that
// inform connected editor widgets about the state of the undo/redo system.
// The following are the undo/redo operations of a QTextDocument :
//   Insertion or removal of characters. A sequence of insertions or removals within the same text block are regarded
//     as a single undo/redo operation.
//   Insertion or removal of text blocks. Sequences of insertion or removals in a single operation (e.g., by selecting
//     and then deleting text) are regarded as a single undo/redo operation.
//Text character format changes.
//Text block format changes.
//Text block group format changes.

void
UndoRedo::Clear_No_Undo ( ) {
    Undo_Stack_Clear();
    Redo_Stack_Clear();
    if (not (Focus_LineEdit == nullptr)) Focus_LineEdit->clear();
    else if (not (Focus_PlainTextEdit == nullptr)) Focus_PlainTextEdit->clear();
}

void
UndoRedo::SetText_No_Undo ( QString New_Text ) {
    Clear_No_Undo();
    Push_Undo();
    if (not (Focus_LineEdit == nullptr)) Focus_LineEdit->insert(New_Text);
    else if (not (Focus_PlainTextEdit == nullptr)) Focus_PlainTextEdit->insertPlainText(New_Text);
}

bool
UndoRedo::Is_Identifier_Or_Number ( QChar Test_Ch ) {
    return (Test_Ch.isLetterOrNumber() or
            (Test_Ch == "_") or  (Test_Ch == ".") or
            (Test_Ch == Unicode_Thin_Space));
}

bool
UndoRedo::keyPressEvent_Handler ( QKeyEvent *event ) {
    bool already_handled_event = false; // Let superclass handle

    if (event->matches(QKeySequence::Undo)) {
        // Do not allow normal event handling
        already_handled_event = true;
        // Otherwise normal undo will trash text
    }
    else if (event->matches(QKeySequence::Redo)) {
        // Do not allow normal event handling
        already_handled_event = true;
        // Otherwise normal undo will trash text
    }
    else if ((event->matches(QKeySequence::Backspace)) or
             (event->matches(QKeySequence::Delete))) {
        this->Push_Undo();
    }
    else if ((event->matches(QKeySequence::Cut)) or
             (event->matches(QKeySequence::Paste))) {
        this->Push_Undo();
    }
    else if ((event->key() == Qt::Key_Up) or
             (event->key() == Qt::Key_Down) or
             (event->key() == Qt::Key_Left) or
             (event->key() == Qt::Key_Right)) {
        // The goal here is to avoid a series of cursor movment ...
        // ... pushes to the undo stack.
        Deferred_Push_Undo = true;
    }
    else {
#if defined(Q_OS_ANDROID)
        uint modifiers = Keyboard_Modifiers;
#else
        Qt::KeyboardModifiers modifiers = event->modifiers();
#endif

//        if (modifiers == Qt::NoModifier) {}
//        else if ((modifiers & Qt::ShiftModifier) == Qt::ShiftModifier) {}
//        else if ((modifiers & Qt::ControlModifier) == Qt::ControlModifier) {}
//        else if ((modifiers & Qt::AltModifier) == Qt::AltModifier) {}

        if (modifiers == Qt::NoModifier) {
            if (event->text().length() > 0) {
                Redo_Stack_Clear();
                if (Deferred_Push_Undo or (Undo_Stack.count() == 0) or
                    // Selection about to be replaced
                    (Selected_Count() > 0)) {
                    this->Push_Undo();
                }
                else if ((Do_State.length() > 0) and
                         (not Is_Identifier_Or_Number(Do_State.at(Do_State.length() - 1))) and
                         // Start of identifier or number
                         Is_Identifier_Or_Number(event->text().at(0))) {
                    this->Push_Undo();
                }

                Do_State += event->text();
            }
        }
    }

    return already_handled_event;
}

bool
UndoRedo::keyReleaseEvent_Handler ( QKeyEvent *event ) {
    bool already_handled_event = false; // Let superclass handle

    if (event->matches(QKeySequence::Undo)) {
        Execute_Undo();
        // Do not allow normal event handling
        already_handled_event = true;
        // Otherwise normal undo will trash text
    }
    else if (event->matches(QKeySequence::Redo)) {
        Execute_Redo();
        // Do not allow normal event handling
        already_handled_event = true;
        // Otherwise normal undo will trash text
    }

    return already_handled_event;
}
