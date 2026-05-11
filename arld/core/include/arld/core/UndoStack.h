#pragma once
#include <arld/core/ICommand.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace arld::core {

class UndoStack {
public:
    explicit UndoStack(int maxDepth = 100);

    // Calls cmd->execute(), pushes onto the stack, clears redo history.
    void push(std::unique_ptr<ICommand> cmd);

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
};

} // namespace arld::core
