#pragma once
#include <arld/core/ICommand.h>
#include <arld/core/MacroCommand.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace arld::core {

class UndoStack {
public:
    explicit UndoStack(int maxDepth = 100);

    // Calls cmd->execute(), pushes onto the stack, clears redo history.
    // While a macro is open (beginMacro called), the command is added to the
    // pending macro instead of being pushed directly.
    void push(std::unique_ptr<ICommand> cmd);

    /// Start collecting subsequent push() calls into a single MacroCommand.
    /// Nested calls are ignored — only one level of macro is supported.
    void beginMacro(const std::string& description);

    /// Finish the macro and push it as a single undoable step.
    /// No-op if no macro is open. Empty macros are discarded.
    void endMacro();

    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;

    std::string undoText() const;
    std::string redoText() const;

    void clear();

    /// Returns descriptions of all commands oldest→newest.
    std::vector<std::string> history() const;

    /// Current top-of-stack index (0 = nothing executed, i = i commands have been executed).
    int currentIndex() const;

    /// Undo/redo to reach @p targetIndex. Calls undo() or redo() as needed.
    void goToIndex(int targetIndex);

    // Invoked after every mutation so the UI can refresh action enable states.
    std::function<void()> onChanged;

private:
    int m_maxDepth;
    std::vector<std::unique_ptr<ICommand>> m_stack;
    int m_currentIndex = -1;

    std::unique_ptr<MacroCommand> m_pendingMacro; // non-null while inside beginMacro
};

} // namespace arld::core
