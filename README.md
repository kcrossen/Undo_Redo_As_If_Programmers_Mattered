# Undo_Redo_As_If_Programmers_Mattered
QUndoStack command compression is absurdly/needlessly aggressive

QUndoStack and QUndoCommand, when used for text widgets (e.g. QLineEdit and QPlainTextEdit), implicitly assume that adjacent text characters inserted in sequence are to be treated as a single unit for undo/redo purposes. To see how wrong this is, you have typed:

"define sine_loop(loop_count) { local begin_time = time(); local count = 0; local angle; local sin4; while"

And you realize that you mistyped "sin4" instead of "sine". If you Ctrl-Z to undo, the entire string is erased. What should happen is that "while" is erased, Ctrl-Z again to erase "sin4; ", and finally retype "sine; while". Put differently, the undo/redo machinery is your enemy, not your friend.

The goal should be to catch every important change in the "text box" w/o assuming that all adjacent character insertions are to be treated as a single undo/redo unit as QUndoStack does. Undo/redo should assume that insertion sequences are to be treated as a series of identifiers or words or numbers, each such to be treated as a separate undo/redo unit.
