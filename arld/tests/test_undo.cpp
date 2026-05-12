#include <catch2/catch_test_macros.hpp>
#include <arld/core/ICommand.h>
#include <arld/core/MacroCommand.h>
#include <arld/core/UndoStack.h>

namespace {

// Minimal command that increments/decrements a counter.
struct CounterCmd : arld::core::ICommand {
    int& counter;
    explicit CounterCmd(int& c) : counter(c) {}
    void execute() override { ++counter; }
    void undo()    override { --counter; }
    std::string describe() const override { return "Increment"; }
};

} // anonymous namespace

TEST_CASE("UndoStack: push executes command and canUndo becomes true", "[undo]") {
    int n = 0;
    arld::core::UndoStack stack;
    stack.push(std::make_unique<CounterCmd>(n));
    REQUIRE(n == 1);
    REQUIRE(stack.canUndo());
    REQUIRE_FALSE(stack.canRedo());
}

TEST_CASE("UndoStack: undo reverts and enables redo", "[undo]") {
    int n = 0;
    arld::core::UndoStack stack;
    stack.push(std::make_unique<CounterCmd>(n));
    stack.undo();
    REQUIRE(n == 0);
    REQUIRE_FALSE(stack.canUndo());
    REQUIRE(stack.canRedo());
}

TEST_CASE("UndoStack: redo re-applies command", "[undo]") {
    int n = 0;
    arld::core::UndoStack stack;
    stack.push(std::make_unique<CounterCmd>(n));
    stack.undo();
    stack.redo();
    REQUIRE(n == 1);
    REQUIRE(stack.canUndo());
    REQUIRE_FALSE(stack.canRedo());
}

TEST_CASE("UndoStack: new push after undo clears redo history", "[undo]") {
    int n = 0;
    arld::core::UndoStack stack;
    stack.push(std::make_unique<CounterCmd>(n));
    stack.push(std::make_unique<CounterCmd>(n));
    stack.undo();
    REQUIRE(stack.canRedo());

    stack.push(std::make_unique<CounterCmd>(n));
    REQUIRE_FALSE(stack.canRedo());
    REQUIRE(n == 2);
}

TEST_CASE("UndoStack: respects 100-level depth limit", "[undo]") {
    int n = 0;
    arld::core::UndoStack stack(100);

    for (int i = 0; i < 110; ++i)
        stack.push(std::make_unique<CounterCmd>(n));

    REQUIRE(n == 110);

    // Should be able to undo exactly 100 times, not 110.
    int undoCount = 0;
    while (stack.canUndo()) { stack.undo(); ++undoCount; }
    REQUIRE(undoCount == 100);
    REQUIRE(n == 10);  // oldest 10 commands were dropped
}

TEST_CASE("UndoStack: undoText and redoText return command descriptions", "[undo]") {
    int n = 0;
    arld::core::UndoStack stack;

    REQUIRE(stack.undoText().empty());
    REQUIRE(stack.redoText().empty());

    stack.push(std::make_unique<CounterCmd>(n));
    REQUIRE(stack.undoText() == "Increment");
    stack.undo();
    REQUIRE(stack.redoText() == "Increment");
}

TEST_CASE("UndoStack: clear resets all state", "[undo]") {
    int n = 0;
    arld::core::UndoStack stack;
    stack.push(std::make_unique<CounterCmd>(n));
    stack.push(std::make_unique<CounterCmd>(n));
    stack.clear();
    REQUIRE_FALSE(stack.canUndo());
    REQUIRE_FALSE(stack.canRedo());
}

TEST_CASE("UndoStack: onChanged callback fires on push, undo, redo, clear", "[undo]") {
    int n = 0, callbackCount = 0;
    arld::core::UndoStack stack;
    stack.onChanged = [&] { ++callbackCount; };

    stack.push(std::make_unique<CounterCmd>(n)); REQUIRE(callbackCount == 1);
    stack.undo();                                REQUIRE(callbackCount == 2);
    stack.redo();                                REQUIRE(callbackCount == 3);
    stack.clear();                               REQUIRE(callbackCount == 4);
}

TEST_CASE("MacroCommand: execute runs sub-commands in order", "[undo][macro]") {
    std::vector<int> log;
    struct LogCmd : arld::core::ICommand {
        std::vector<int>& log;
        int value;
        LogCmd(std::vector<int>& l, int v) : log(l), value(v) {}
        void execute() override { log.push_back(value); }
        void undo()    override { log.push_back(-value); }
        std::string describe() const override { return "Log"; }
    };

    arld::core::MacroCommand macro("Test Macro");
    macro.add(std::make_unique<LogCmd>(log, 1));
    macro.add(std::make_unique<LogCmd>(log, 2));
    macro.add(std::make_unique<LogCmd>(log, 3));
    macro.execute();

    REQUIRE(log == std::vector<int>{1, 2, 3});
}

TEST_CASE("MacroCommand: undo reverses sub-commands in reverse order", "[undo][macro]") {
    std::vector<int> log;
    struct LogCmd : arld::core::ICommand {
        std::vector<int>& log;
        int value;
        LogCmd(std::vector<int>& l, int v) : log(l), value(v) {}
        void execute() override { log.push_back(value); }
        void undo()    override { log.push_back(-value); }
        std::string describe() const override { return "Log"; }
    };

    arld::core::MacroCommand macro("Test Macro");
    macro.add(std::make_unique<LogCmd>(log, 1));
    macro.add(std::make_unique<LogCmd>(log, 2));
    macro.execute();
    log.clear();
    macro.undo();

    REQUIRE(log == std::vector<int>{-2, -1});
}

TEST_CASE("MacroCommand: describe returns the given label", "[undo][macro]") {
    arld::core::MacroCommand macro("Move Group");
    REQUIRE(macro.describe() == "Move Group");
}

TEST_CASE("UndoStack: beginMacro/endMacro groups pushes into one undo step", "[undo][macro]") {
    int n = 0;
    arld::core::UndoStack stack;
    stack.beginMacro("Group");
    stack.push(std::make_unique<CounterCmd>(n));
    stack.push(std::make_unique<CounterCmd>(n));
    stack.push(std::make_unique<CounterCmd>(n));
    stack.endMacro();

    REQUIRE(n == 3);
    REQUIRE(stack.canUndo());
    REQUIRE(stack.undoText() == "Group");

    stack.undo();
    REQUIRE(n == 0);
    REQUIRE_FALSE(stack.canUndo());
}

TEST_CASE("UndoStack: beginMacro/endMacro redo re-applies all sub-commands", "[undo][macro]") {
    int n = 0;
    arld::core::UndoStack stack;
    stack.beginMacro("Group");
    stack.push(std::make_unique<CounterCmd>(n));
    stack.push(std::make_unique<CounterCmd>(n));
    stack.endMacro();

    stack.undo();
    REQUIRE(n == 0);
    stack.redo();
    REQUIRE(n == 2);
}
