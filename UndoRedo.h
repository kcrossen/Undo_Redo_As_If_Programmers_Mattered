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

#ifndef UNDOREDO_H
#define UNDOREDO_H

#include <QObject>
#include <QStack>
#include <QKeyEvent>

class LineEdit;
class PlainTextEdit;

class UndoRedo : public QObject {
    Q_OBJECT
public:
    explicit UndoRedo ( QObject *parent = nullptr );

    void Set_Focus_Widget ( QWidget *New_Focus_Widget );

    // If true, this event has been handled, ...
    // ... if false let superclass handle
    bool keyPressEvent_Handler ( QKeyEvent *event );
    bool keyReleaseEvent_Handler ( QKeyEvent *event );

private:
    QWidget *Focus_Widget;
    LineEdit *Focus_LineEdit;
    PlainTextEdit *Focus_PlainTextEdit;

    struct Text_State {
        QString Text;
        int Select_Begin;
        int Select_End;
        int Cursor_Position;
    };

    Text_State Save_Text_State ( );
    void Restore_Text_State ( Text_State New_Text_State );

    int Selected_Count ( );

#define Maximum_Undo_Stack_Count 100

    // This supports less aggressive undo/redo command compression.
    // Normally any adjacent insert operations are treated as a single ...
    // ... undoable/redoable operation. When inserts are being made in ...
    // ... application-logical chunks, they should be undoable/redoable ...
    // ... separately.
    enum Stack_Selector { Select_Undo, Select_Redo };

    QStack<Text_State> Undo_Stack;
    // The current state is here, between undo states and redo states
    QStack<Text_State> Redo_Stack;

    // If an Undo state is pushed (independent of undo execution) ...
    // ... Redo_Stack is cleared.
    // Otherwise, for undo or redo execution, push the current state onto ...
    // ... the opposite stack, pop the new state off same-named stack, and ...
    // ... enforce that popped state.
    void Push_State ( Stack_Selector Select_Stack );
    void Pop_State ( Stack_Selector Select_Stack );

    bool Record_Move_Cursor_Undo = false;

public:
    void Undo_Stack_Clear ( );
    void Redo_Stack_Clear ( );

    int Undo_Stack_Count ( );
    int Redo_Stack_Count ( );

    void Push_Undo ( );
    bool Deferred_Push_Undo = false;

    QString Do_State = "";

    // The strategy is to break the undo/redo "atoms" between identifiers ...
    // ... (e.g function/variable names), numbers, and keywords.
    bool Is_Identifier_Or_Number ( QChar Test_Ch );

public:
    // This supports less aggressive undo/redo command compression.
    // Normally any adjacent insert operations are treated as a single ...
    // ... undoable/redoable operation. When inserts are being made in ...
    // ... application-logical chunks, they should be undoable/redoable ...
    // ... separately.
    void Execute_Undo ( );
    void Execute_Redo ( );

    void Clear_No_Undo ( );
    void SetText_No_Undo ( QString New_Text );
};


#endif // UNDOREDO_H
