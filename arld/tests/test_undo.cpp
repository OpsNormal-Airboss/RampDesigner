#include <catch2/catch_test_macros.hpp>
#include <arld/core/ICommand.h>
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
