#pragma once
#include <arld/core/ICommand.h>
#include <memory>
#include <string>
#include <vector>

namespace arld::core {

/// Composite command: executes N sub-commands as a single undoable step.
/// Sub-commands are executed forward in insertion order and undone in reverse.
class MacroCommand : public ICommand {
public:
    explicit MacroCommand(std::string description)
        : m_description(std::move(description)) {}

    void add(std::unique_ptr<ICommand> cmd) {
        m_cmds.push_back(std::move(cmd));
    }

    bool empty() const { return m_cmds.empty(); }

    void execute() override {
        for (auto& c : m_cmds) c->execute();
    }

    void undo() override {
        for (auto it = m_cmds.rbegin(); it != m_cmds.rend(); ++it)
            (*it)->undo();
    }

    std::string describe() const override { return m_description; }

private:
    std::string m_description;
    std::vector<std::unique_ptr<ICommand>> m_cmds;
};

} // namespace arld::core
