#pragma once

#include <stack>
#include <string>
#include "../includes/json.hpp"

using json = nlohmann::json;

// we are going to use a stack to manage undo states in the editor
// types of actions we want to support:
// - object transformations (move, rotate, scale, color, active, intensity)
// - object creation/deletion/duplication
// - material changes (cant change materials yet but in future)

class UndoManager {
public:
    struct Action {
        enum ActionType {
            Transform,
            Create,
            Delete,
            Duplicate,
            MaterialChange
        } type;

        json objectStateBefore;
        json objectStateAfter;
        int objectId; 
    };

    void pushAction(const Action& action) {
        undoStack.push(action);
        while (!redoStack.empty()) redoStack.pop();
    }

    bool canUndo() const {
        return !undoStack.empty();
    }

    bool canRedo() const {
        return !redoStack.empty();
    }

    Action popUndo() {
        if (canUndo()) {
            Action action = undoStack.top();
            undoStack.pop();
            redoStack.push(action);
            return action;
        }
        return Action{};
    }

    Action popRedo() {
        if (canRedo()) {
            Action action = redoStack.top();
            redoStack.pop();
            undoStack.push(action);
            return action;
        }
        return Action{};
    }
private:
    std::stack<Action> undoStack;
    std::stack<Action> redoStack;
};
