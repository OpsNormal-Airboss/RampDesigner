#include <arld/core/UndoStack.h>
#include <cassert>

namespace arld::core {

UndoStack::UndoStack(int maxDepth) : m_maxDepth(maxDepth) {
    assert(maxDepth > 0);
}

void UndoStack::push(std::unique_ptr<ICommand> cmd) {
    // Truncate redo history.
    if (m_currentIndex < static_cast<int>(m_stack.size()) - 1)
        m_stack.erase(m_stack.begin() + m_currentIndex + 1, m_stack.end());

    cmd->execute();
    m_stack.push_back(std::move(cmd));
    m_currentIndex = static_cast<int>(m_stack.size()) - 1;

    // Enforce depth limit by dropping the oldest command.
    if (static_cast<int>(m_stack.size()) > m_maxDepth) {
        m_stack.erase(m_stack.begin());
        --m_currentIndex;
    }

    if (onChanged) onChanged();
}

void UndoStack::undo() {
    if (!canUndo()) return;
    m_stack[m_currentIndex]->undo();
    --m_currentIndex;
    if (onChanged) onChanged();
}

void UndoStack::redo() {
    if (!canRedo()) return;
    ++m_currentIndex;
    m_stack[m_currentIndex]->execute();
    if (onChanged) onChanged();
}

bool UndoStack::canUndo() const { return m_currentIndex >= 0; }

bool UndoStack::canRedo() const {
    return m_currentIndex < static_cast<int>(m_stack.size()) - 1;
}

std::string UndoStack::undoText() const {
    return canUndo() ? m_stack[m_currentIndex]->describe() : "";
}

std::string UndoStack::redoText() const {
    return canRedo() ? m_stack[m_currentIndex + 1]->describe() : "";
}

void UndoStack::clear() {
    m_stack.clear();
    m_currentIndex = -1;
    if (onChanged) onChanged();
}

std::vector<std::string> UndoStack::history() const {
    std::vector<std::string> result;
    result.reserve(m_stack.size());
    for (const auto& cmd : m_stack)
        result.push_back(cmd->describe());
    return result;
}

int UndoStack::currentIndex() const {
    return m_currentIndex;
}

void UndoStack::goToIndex(int targetIndex) {
    // Clamp to valid range: -1 (nothing done) to stack.size()-1
    const int maxIdx = static_cast<int>(m_stack.size()) - 1;
    if (targetIndex < -1) targetIndex = -1;
    if (targetIndex > maxIdx) targetIndex = maxIdx;

    while (m_currentIndex > targetIndex)
        undo();
    while (m_currentIndex < targetIndex)
        redo();
}

} // namespace arld::core
