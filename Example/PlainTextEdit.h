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

#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QApplication>
#include <QClipboard>
#include <QPlainTextEdit>
#include <QMenu>
#include <QDateTime>
#include <QKeyEvent>
#include <QScrollBar>
#include <QStack>

#include "UI_Defines.h"
#include "UndoRedo.h"

class PlainTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit PlainTextEdit ( QWidget *parent = nullptr );

    QString toPlainText_Clean ( );
    QString toPlainText_Clean_No_ThinSpace ( );

    QChar TextCursorIndicator ( );

    void
    insertTextCursorIndicator ( );
    void
    removeTextCursorIndicator ( );

    void
    Set_Numeric_Thin_Spaces ( bool New_Numeric_Thin_Spaces );

    void
    Set_Support_Long_Press ( bool New_Support_Long_Press );

    void
    Set_Long_Press_Milliseconds_Threshold ( qint64 New_Long_Press_Milliseconds_Threshold );

private:
    bool Numeric_Thin_Spaces = false;

    bool Suppress_PlainTextChanged = false;

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

    UndoRedo *Undo_Redo;

public slots:
    void insertPlainText ( const QString &Text );
    void Delete_Previous_Character ( );

    void Move_Cursor ( QTextCursor::MoveOperation Move_Operation );

    void undo ( );
    void redo ( );

    void copy ( );
    void cut ( );
    void paste ( );

signals:
    void keyPressed ( int Key );
    void focusIn ( );
    void focusOut ( );
    void PlainTextChanged ( );

private slots:
    void Private_textChanged ( );
    // void Private_cursorPositionChanged ( );

protected:
    void keyPressEvent ( QKeyEvent *event );
    void keyReleaseEvent ( QKeyEvent *event );

    void focusInEvent ( QFocusEvent *event );
    void focusOutEvent ( QFocusEvent *event );

private:
    bool Support_Long_Press = false;

    qint64 Long_Press_Milliseconds_Threshold;

    QTextCursor Mouse_Press_Select_Cursor;

    qint64 Mouse_Pressed_Milliseconds;
    int Mouse_Press_Position;

    qint64 Mouse_Released_Milliseconds;
    int Mouse_Release_Position;

    QPoint Mouse_Press_Cursor_Position;

    int Mouse_Press_Cursor_Y;
    int Mouse_Press_Vertical_ScrollBar_Value;

protected:
    void
    mousePressEvent ( QMouseEvent *event );

    void
    mouseMoveEvent ( QMouseEvent *event );

    void
    mouseReleaseEvent ( QMouseEvent *event );

private:
    bool Has_Focus = false;

signals:
    void
    changedFocus ( bool has_focus );

    void
    longPressed ( int position );

public:
    void
    Custom_ContextMenu ( const QPoint &Mouse_Cursor_Poition );

private slots:
    void onContextPaste ( );

    void onContextCopySelection ( );
    void onContextCutSelection ( );
    void onContextDeleteSelection ( );

    void onContextCopyLine ( );
    void onContextCopyWord ( );
    void onContextCopyAll ( );

};

#endif // PLAINTEXTEDIT_H
