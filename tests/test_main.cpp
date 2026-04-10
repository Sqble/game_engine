#include "../src/aabb/aabb.h"
#include "../src/camera/camera.h"
#include "../src/file_manager/file_manager.h"
#include "../src/frustum/frustum.h"
#include "../src/game/inventory/inventory_container.h"
#include "../src/game/inventory/inventory_system.h"
#include "../src/game/inventory/item_definition.h"
#include "../src/pointlight/pointlight.h"
#include "../src/raycast/raycast.h"
#include "../src/sceneobject/sceneobject.h"
#include "../src/undo/undo.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void Shader::setVec3(const std::string&, const glm::vec3&) const {
}

void Shader::setFloat(const std::string&, float) const {
}

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class DummySceneObject : public SceneObject {
public:
    using SceneObject::SceneObject;

    void draw(Shader&) override {
    }
};

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> run;
};

std::string Vec3ToString(const glm::vec3& value) {
    std::ostringstream stream;
    stream << "(" << value.x << ", " << value.y << ", " << value.z << ")";
    return stream.str();
}

void ExpectTrue(bool condition, const std::string& message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

template <typename T, typename U>
void ExpectEqual(const T& actual, const U& expected, const std::string& message) {
    if (!(actual == expected)) {
        std::ostringstream stream;
        stream << message << " expected " << expected << " but got " << actual;
        throw TestFailure(stream.str());
    }
}

void ExpectNear(float actual, float expected, float epsilon, const std::string& message) {
    if (std::fabs(actual - expected) > epsilon) {
        std::ostringstream stream;
        stream << message << " expected " << expected << " but got " << actual;
        throw TestFailure(stream.str());
    }
}

void ExpectVec3Equal(const glm::vec3& actual,
                     const glm::vec3& expected,
                     const std::string& message,
                     float epsilon = 0.0001f) {
    if (std::fabs(actual.x - expected.x) > epsilon ||
        std::fabs(actual.y - expected.y) > epsilon ||
        std::fabs(actual.z - expected.z) > epsilon) {
        throw TestFailure(message + " expected " + Vec3ToString(expected) + " but got " + Vec3ToString(actual));
    }
}

void ExpectVec3Finite(const glm::vec3& value, const std::string& message) {
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) {
        throw TestFailure(message + " got non-finite vector " + Vec3ToString(value));
    }
}

void ExpectContains(const std::string& text, const std::string& needle, const std::string& message) {
    if (text.find(needle) == std::string::npos) {
        throw TestFailure(message + " missing substring '" + needle + "'");
    }
}

void AddTest(std::vector<TestCase>& tests,
             const std::string& suite,
             const std::string& name,
             std::function<void()> run) {
    tests.push_back(TestCase{suite, name, std::move(run)});
}

UndoManager::Action MakeAction(UndoManager::Action::ActionType type,
                               int objectId,
                               const json& before = json::object(),
                               const json& after = json::object()) {
    UndoManager::Action action{};
    action.type = type;
    action.objectId = objectId;
    action.objectStateBefore = before;
    action.objectStateAfter = after;
    return action;
}

void RegisterSceneObjectTests(std::vector<TestCase>& tests) {
    const std::string suite = "SceneObject suite";

    AddTest(tests, suite, "assigns sequential ids", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject first;
        DummySceneObject second;

        ExpectEqual(first.getId(), 0, "first object id");
        ExpectEqual(second.getId(), 1, "second object id");
    });

    AddTest(tests, suite, "nextId reflects next assignment", [] {
        SceneObject::setIDCounter(12);
        ExpectEqual(SceneObject::nextId(), 12, "nextId should match the next assigned id");

        DummySceneObject object;
        ExpectEqual(object.getId(), 12, "new object should receive next id");
        ExpectEqual(SceneObject::nextId(), 13, "nextId should advance after construction");
    });

    AddTest(tests, suite, "rejects null children", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent;
        ExpectTrue(!parent.addChild(nullptr), "adding a null child should fail");
    });

    AddTest(tests, suite, "rejects self parenting", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject object;
        ExpectTrue(!object.setParent(&object), "an object cannot parent itself");
    });

    AddTest(tests, suite, "keeps world position on addChild", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({10.0f, 5.0f, -2.0f}, {2.0f, 4.0f, 8.0f}, {1.0f, 1.0f, 1.0f}, {5.0f, 10.0f, 15.0f});
        DummySceneObject child({14.0f, 9.0f, 6.0f}, {6.0f, 12.0f, 16.0f}, {1.0f, 1.0f, 1.0f}, {20.0f, 35.0f, 50.0f});

        ExpectTrue(parent.addChild(&child, true), "parenting should succeed");
        ExpectVec3Equal(child.getPosition(), {14.0f, 9.0f, 6.0f}, "world position should stay the same");
    });

    AddTest(tests, suite, "keeps world scale on addChild", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {2.0f, 4.0f, 8.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {6.0f, 12.0f, 16.0f});

        ExpectTrue(parent.addChild(&child, true), "parenting should succeed");
        ExpectVec3Equal(child.getSize(), {6.0f, 12.0f, 16.0f}, "world scale should stay the same");
    });

    AddTest(tests, suite, "keeps world rotation on addChild", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {5.0f, 10.0f, 15.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {20.0f, 35.0f, 50.0f});

        ExpectTrue(parent.addChild(&child, true), "parenting should succeed");
        ExpectVec3Equal(child.getRotation(), {20.0f, 35.0f, 50.0f}, "world rotation should stay the same");
    });

    AddTest(tests, suite, "rebases local position when keeping world", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({10.0f, 5.0f, -2.0f});
        DummySceneObject child({14.0f, 9.0f, 6.0f});

        ExpectTrue(parent.addChild(&child, true), "parenting should succeed");
        ExpectVec3Equal(child.getLocalPosition(), {4.0f, 4.0f, 8.0f}, "local position should be rebased");
    });

    AddTest(tests, suite, "rebases local scale when keeping world", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {2.0f, 4.0f, 8.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {6.0f, 12.0f, 16.0f});

        ExpectTrue(parent.addChild(&child, true), "parenting should succeed");
        ExpectVec3Equal(child.getLocalSize(), {3.0f, 3.0f, 2.0f}, "local size should be normalized");
    });

    AddTest(tests, suite, "rebases local rotation when keeping world", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {5.0f, 10.0f, 15.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {20.0f, 35.0f, 50.0f});

        ExpectTrue(parent.addChild(&child, true), "parenting should succeed");
        ExpectVec3Equal(child.getLocalRotation(), {15.0f, 25.0f, 35.0f}, "local rotation should be rebased");
    });

    AddTest(tests, suite, "applies parent position without keep-world", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({3.0f, 4.0f, 5.0f});
        DummySceneObject child({1.0f, 2.0f, 3.0f});

        ExpectTrue(child.setParent(&parent, false), "setParent should succeed");
        ExpectVec3Equal(child.getPosition(), {4.0f, 6.0f, 8.0f}, "world position should include parent");
    });

    AddTest(tests, suite, "applies parent scale without keep-world", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {2.0f, 3.0f, 4.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {4.0f, 5.0f, 6.0f});

        ExpectTrue(child.setParent(&parent, false), "setParent should succeed");
        ExpectVec3Equal(child.getSize(), {8.0f, 15.0f, 24.0f}, "world scale should include parent scale");
    });

    AddTest(tests, suite, "applies parent rotation without keep-world", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {10.0f, 20.0f, 30.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {7.0f, 8.0f, 9.0f});

        ExpectTrue(child.setParent(&parent, false), "setParent should succeed");
        ExpectVec3Equal(child.getRotation(), {17.0f, 28.0f, 39.0f}, "world rotation should include parent rotation");
    });

    AddTest(tests, suite, "setPosition on child updates local space", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({10.0f, 10.0f, 10.0f});
        DummySceneObject child({12.0f, 13.0f, 14.0f});
        parent.addChild(&child, true);

        child.setPosition({20.0f, 25.0f, 30.0f});

        ExpectVec3Equal(child.getLocalPosition(), {10.0f, 15.0f, 20.0f}, "child local position should be relative to parent");
        ExpectVec3Equal(child.getPosition(), {20.0f, 25.0f, 30.0f}, "child world position should match requested position");
    });

    AddTest(tests, suite, "setSize on child updates local space", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {2.0f, 4.0f, 8.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {6.0f, 12.0f, 16.0f});
        parent.addChild(&child, true);

        child.setSize({10.0f, 20.0f, 40.0f});

        ExpectVec3Equal(child.getLocalSize(), {5.0f, 5.0f, 5.0f}, "child local size should be relative to parent");
        ExpectVec3Equal(child.getSize(), {10.0f, 20.0f, 40.0f}, "child world size should match requested size");
    });

    AddTest(tests, suite, "setRotation on child updates local space", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {5.0f, 10.0f, 15.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {20.0f, 35.0f, 50.0f});
        parent.addChild(&child, true);

        child.setRotation({30.0f, 45.0f, 60.0f});

        ExpectVec3Equal(child.getLocalRotation(), {25.0f, 35.0f, 45.0f}, "child local rotation should be relative to parent");
        ExpectVec3Equal(child.getRotation(), {30.0f, 45.0f, 60.0f}, "child world rotation should match requested rotation");
    });

    AddTest(tests, suite, "nested parents accumulate world position", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject root({1.0f, 2.0f, 3.0f});
        DummySceneObject parent({4.0f, 5.0f, 6.0f});
        DummySceneObject child({7.0f, 8.0f, 9.0f});

        root.addChild(&parent, false);
        parent.addChild(&child, false);

        ExpectVec3Equal(child.getPosition(), {12.0f, 15.0f, 18.0f}, "world position should accumulate through hierarchy");
    });

    AddTest(tests, suite, "nested parents accumulate world scale", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject root({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {3.0f, 3.0f, 3.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {4.0f, 4.0f, 4.0f});

        root.addChild(&parent, false);
        parent.addChild(&child, false);

        ExpectVec3Equal(child.getSize(), {24.0f, 24.0f, 24.0f}, "world scale should multiply through hierarchy");
    });

    AddTest(tests, suite, "nested parents accumulate world rotation", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject root({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 2.0f, 3.0f});
        DummySceneObject parent({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {4.0f, 5.0f, 6.0f});
        DummySceneObject child({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {7.0f, 8.0f, 9.0f});

        root.addChild(&parent, false);
        parent.addChild(&child, false);

        ExpectVec3Equal(child.getRotation(), {12.0f, 15.0f, 18.0f}, "world rotation should accumulate through hierarchy");
    });

    AddTest(tests, suite, "removeChild detaches matching child", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({10.0f, 0.0f, 0.0f});
        DummySceneObject child({15.0f, 0.0f, 0.0f});
        parent.addChild(&child, true);

        parent.removeChild(&child, true);

        ExpectTrue(child.getParent() == nullptr, "child should be detached");
        ExpectTrue(parent.getChildren().empty(), "parent child list should be empty");
        ExpectVec3Equal(child.getPosition(), {15.0f, 0.0f, 0.0f}, "world position should stay the same on detach");
    });

    AddTest(tests, suite, "removeChild ignores non-child", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent;
        DummySceneObject child;

        parent.removeChild(&child, true);

        ExpectTrue(child.getParent() == nullptr, "non-child should remain detached");
        ExpectTrue(parent.getChildren().empty(), "parent should remain empty");
    });

    AddTest(tests, suite, "detachChildren clears all children", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({10.0f, 0.0f, 0.0f});
        DummySceneObject childA({11.0f, 1.0f, 0.0f});
        DummySceneObject childB({12.0f, 2.0f, 0.0f});
        parent.addChild(&childA, true);
        parent.addChild(&childB, true);

        parent.detachChildren(true);

        ExpectTrue(parent.getChildren().empty(), "parent should have no children");
        ExpectTrue(childA.getParent() == nullptr, "first child should be detached");
        ExpectTrue(childB.getParent() == nullptr, "second child should be detached");
        ExpectVec3Equal(childA.getPosition(), {11.0f, 1.0f, 0.0f}, "first child world position should stay the same");
        ExpectVec3Equal(childB.getPosition(), {12.0f, 2.0f, 0.0f}, "second child world position should stay the same");
    });

    AddTest(tests, suite, "reparenting does not duplicate child", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent;
        DummySceneObject child;

        parent.addChild(&child, true);
        ExpectTrue(parent.addChild(&child, true), "re-adding same parent should succeed");

        ExpectEqual(parent.getChildren().size(), static_cast<size_t>(1), "child should only appear once");
    });

    AddTest(tests, suite, "reparenting preserves world transform", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject firstParent({0.0f, 0.0f, 0.0f});
        DummySceneObject secondParent({5.0f, 0.0f, 0.0f});
        DummySceneObject child({2.0f, 0.0f, 0.0f});
        firstParent.addChild(&child, true);

        ExpectTrue(secondParent.addChild(&child, true), "reparenting should succeed");
        ExpectTrue(firstParent.getChildren().empty(), "old parent should lose the child");
        ExpectEqual(secondParent.getChildren().size(), static_cast<size_t>(1), "new parent should have one child");
        ExpectVec3Equal(child.getPosition(), {2.0f, 0.0f, 0.0f}, "world position should remain unchanged");
    });

    AddTest(tests, suite, "rejects direct cycles", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject root;
        DummySceneObject child;
        root.addChild(&child, true);

        ExpectTrue(!root.setParent(&child, true), "setting a child as parent should fail");
    });

    AddTest(tests, suite, "rejects indirect cycles", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject root;
        DummySceneObject middle;
        DummySceneObject child;
        root.addChild(&middle, true);
        middle.addChild(&child, true);

        ExpectTrue(!root.setParent(&child, true), "setting a descendant as parent should fail");
    });

    AddTest(tests, suite, "effective active follows ancestors", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject root;
        DummySceneObject parent;
        DummySceneObject child;
        root.addChild(&parent, true);
        parent.addChild(&child, true);

        root.setActive(false);
        ExpectTrue(!child.isEffectivelyActive(), "inactive ancestors should disable descendants");

        root.setActive(true);
        parent.setActive(false);
        ExpectTrue(!child.isEffectivelyActive(), "inactive parent should disable descendants");
    });

    AddTest(tests, suite, "detach without keep-world keeps local values", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent({10.0f, 20.0f, 30.0f}, {2.0f, 3.0f, 4.0f}, {1.0f, 1.0f, 1.0f}, {5.0f, 6.0f, 7.0f});
        DummySceneObject child({1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {1.0f, 1.0f, 1.0f}, {8.0f, 9.0f, 10.0f});
        child.setParent(&parent, false);

        child.detach(false);

        ExpectTrue(child.getParent() == nullptr, "child should be detached");
        ExpectVec3Equal(child.getPosition(), {1.0f, 2.0f, 3.0f}, "world position should become previous local position");
        ExpectVec3Equal(child.getSize(), {4.0f, 5.0f, 6.0f}, "world size should become previous local size");
        ExpectVec3Equal(child.getRotation(), {8.0f, 9.0f, 10.0f}, "world rotation should become previous local rotation");
    });

    AddTest(tests, suite, "tag and manual id setters persist", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject object;

        object.setTag("player_spawn");
        object.setId(99);

        ExpectEqual(object.getTag(), std::string("player_spawn"), "tag should persist");
        ExpectEqual(object.getId(), 99, "manual id should persist");
    });

    AddTest(tests, suite, "parent id resets after detach", [] {
        SceneObject::setIDCounter(0);
        DummySceneObject parent;
        DummySceneObject child;
        parent.addChild(&child, true);

        child.detach(true);

        ExpectEqual(child.getParentId(), -1, "detached child should not report a parent id");
    });
}

void RegisterInventoryContainerTests(std::vector<TestCase>& tests) {
    const std::string suite = "InventoryContainer suite";

    AddTest(tests, suite, "stores configured slot count", [] {
        InventoryContainer container(3);
        ExpectEqual(container.getSlotCount(), static_cast<size_t>(3), "slot count should match constructor");
    });

    AddTest(tests, suite, "accepts valid slot zero", [] {
        InventoryContainer container(2);
        ExpectTrue(container.isValidSlot(0), "slot zero should be valid");
    });

    AddTest(tests, suite, "rejects negative slot indexes", [] {
        InventoryContainer container(2);
        ExpectTrue(!container.isValidSlot(-1), "negative slot should be invalid");
    });

    AddTest(tests, suite, "rejects indexes beyond slot count", [] {
        InventoryContainer container(2);
        ExpectTrue(!container.isValidSlot(2), "out-of-range slot should be invalid");
    });

    AddTest(tests, suite, "setSlot accepts valid indexes", [] {
        InventoryContainer container(2);
        ExpectTrue(container.setSlot(1, InventorySlot{"wrench", 1}), "valid slot assignment should succeed");
        ExpectEqual(container.getSlots()[1].itemId, std::string("wrench"), "assigned slot item id");
    });

    AddTest(tests, suite, "setSlot rejects negative indexes", [] {
        InventoryContainer container(2);
        ExpectTrue(!container.setSlot(-1, InventorySlot{"wrench", 1}), "negative slot assignment should fail");
    });

    AddTest(tests, suite, "setSlot rejects out-of-range indexes", [] {
        InventoryContainer container(2);
        ExpectTrue(!container.setSlot(2, InventorySlot{"wrench", 1}), "out-of-range slot assignment should fail");
    });

    AddTest(tests, suite, "slot is empty when item id is empty", [] {
        InventorySlot slot{"", 3};
        ExpectTrue(slot.isEmpty(), "slots without item ids should count as empty");
    });

    AddTest(tests, suite, "slot is empty when count is zero", [] {
        InventorySlot slot{"scrap", 0};
        ExpectTrue(slot.isEmpty(), "zero-count slots should count as empty");
    });

    AddTest(tests, suite, "clear resets slot contents", [] {
        InventorySlot slot{"scrap", 4};
        slot.clear();

        ExpectEqual(slot.itemId, std::string(""), "clear should reset item id");
        ExpectEqual(slot.count, 0, "clear should reset count");
        ExpectTrue(slot.isEmpty(), "cleared slot should be empty");
    });

    AddTest(tests, suite, "addItem rejects empty ids", [] {
        InventoryContainer container(1);
        ExpectTrue(!container.addItem("", 1), "empty item ids should be rejected");
    });

    AddTest(tests, suite, "addItem rejects zero counts", [] {
        InventoryContainer container(1);
        ExpectTrue(!container.addItem("wrench", 0), "zero counts should be rejected");
    });

    AddTest(tests, suite, "addItem rejects negative counts", [] {
        InventoryContainer container(1);
        ExpectTrue(!container.addItem("wrench", -1), "negative counts should be rejected");
    });

    AddTest(tests, suite, "adds unknown single-stack items across slots", [] {
        InventoryContainer container(2);
        ExpectTrue(container.addItem("mystery", 2), "unknown items should stack as singles");
        ExpectEqual(container.getSlots()[0].itemId, std::string("mystery"), "first slot item id");
        ExpectEqual(container.getSlots()[1].itemId, std::string("mystery"), "second slot item id");
    });

    AddTest(tests, suite, "unknown items fail atomically past capacity", [] {
        InventoryContainer container(1);
        ExpectTrue(!container.addItem("mystery", 2), "unknown items should fail when no capacity remains");
        ExpectTrue(container.getSlots()[0].isEmpty(), "failed add should not partially fill slots");
    });

    AddTest(tests, suite, "fills partial stack before using empty slots", [] {
        InventoryContainer container(2);
        container.setSlot(0, InventorySlot{"scrap", 4});

        ExpectTrue(container.addItem("scrap", 1), "scrap should top off partial stacks first");
        ExpectEqual(container.getSlots()[0].count, 5, "first slot should be filled first");
        ExpectTrue(container.getSlots()[1].isEmpty(), "second slot should remain unused");
    });

    AddTest(tests, suite, "splits stackable items across slots", [] {
        InventoryContainer container(3);
        ExpectTrue(container.addItem("scrap", 7), "scrap should fit across multiple slots");

        ExpectEqual(container.getSlots()[0].count, 5, "first slot should fill to max stack");
        ExpectEqual(container.getSlots()[1].count, 2, "second slot should contain the remainder");
        ExpectTrue(container.getSlots()[2].isEmpty(), "third slot should remain empty");
    });

    AddTest(tests, suite, "fills exact stack multiples cleanly", [] {
        InventoryContainer container(2);
        ExpectTrue(container.addItem("scrap", 10), "exact stack multiples should fit cleanly");
        ExpectEqual(container.getSlots()[0].count, 5, "first slot should be full");
        ExpectEqual(container.getSlots()[1].count, 5, "second slot should be full");
    });

    AddTest(tests, suite, "addItem is atomic on failure", [] {
        InventoryContainer container(2);
        container.setSlot(0, InventorySlot{"scrap", 4});

        ExpectTrue(!container.addItem("scrap", 7), "add should fail when capacity is insufficient");
        ExpectEqual(container.getSlots()[0].count, 4, "partial slot should remain unchanged after failure");
        ExpectTrue(container.getSlots()[1].isEmpty(), "empty slot should remain empty after failure");
    });

    AddTest(tests, suite, "addItem fails when there are no slots", [] {
        InventoryContainer container(0);
        ExpectTrue(!container.addItem("wrench", 1), "containers without slots cannot accept items");
    });

    AddTest(tests, suite, "moveSlotTo rejects invalid indexes", [] {
        InventoryContainer source(1);
        InventoryContainer target(1);
        ExpectTrue(!source.moveSlotTo(2, target), "invalid move source index should fail");
    });

    AddTest(tests, suite, "moveSlotTo rejects empty slots", [] {
        InventoryContainer source(1);
        InventoryContainer target(1);
        ExpectTrue(!source.moveSlotTo(0, target), "empty slots should not move");
    });

    AddTest(tests, suite, "moveSlotTo fails when target is full", [] {
        InventoryContainer source(1);
        InventoryContainer target(1);
        source.setSlot(0, InventorySlot{"wrench", 1});
        target.setSlot(0, InventorySlot{"wrench", 1});

        ExpectTrue(!source.moveSlotTo(0, target), "move should fail when target cannot accept item");
        ExpectEqual(source.getSlots()[0].itemId, std::string("wrench"), "source should keep its item on failure");
    });

    AddTest(tests, suite, "moveSlotTo merges into partial target stack", [] {
        InventoryContainer source(1);
        InventoryContainer target(2);
        source.setSlot(0, InventorySlot{"scrap", 1});
        target.setSlot(0, InventorySlot{"scrap", 4});

        ExpectTrue(source.moveSlotTo(0, target), "move should merge into partial stack");
        ExpectEqual(target.getSlots()[0].count, 5, "target partial stack should be filled");
        ExpectTrue(source.getSlots()[0].isEmpty(), "source slot should be cleared on success");
    });

    AddTest(tests, suite, "moveSlotTo can split across target slots", [] {
        InventoryContainer source(1);
        InventoryContainer target(2);
        source.setSlot(0, InventorySlot{"scrap", 5});
        target.setSlot(0, InventorySlot{"scrap", 4});

        ExpectTrue(source.moveSlotTo(0, target), "move should fill remaining capacity across slots");
        ExpectEqual(target.getSlots()[0].count, 5, "first target slot should top off");
        ExpectEqual(target.getSlots()[1].count, 4, "second target slot should receive the remainder");
    });

    AddTest(tests, suite, "moveSlotTo clears source after success", [] {
        InventoryContainer source(2);
        InventoryContainer target(2);
        source.setSlot(0, InventorySlot{"scrap", 3});

        ExpectTrue(source.moveSlotTo(0, target), "move should succeed");
        ExpectTrue(source.getSlots()[0].isEmpty(), "source slot should be cleared after move");
        ExpectEqual(target.getSlots()[0].count, 3, "target should receive the moved items");
    });

    AddTest(tests, suite, "mutable getSlots reflects edits", [] {
        InventoryContainer container(2);
        container.getSlots()[1] = InventorySlot{"wrench", 1};

        ExpectEqual(container.getSlots()[1].itemId, std::string("wrench"), "mutable slot edits should update the container");
    });
}

void RegisterInventorySystemTests(std::vector<TestCase>& tests) {
    const std::string suite = "InventorySystem suite";

    AddTest(tests, suite, "configures player slot count", [] {
        InventorySystem inventorySystem(4, 2);
        ExpectEqual(inventorySystem.getPlayerInventory().getSlotCount(), static_cast<size_t>(4), "player slot count should match constructor");
    });

    AddTest(tests, suite, "initializeLockers creates first locker", [] {
        InventorySystem inventorySystem(2, 3);
        inventorySystem.initializeLockers({10, 20});
        ExpectTrue(inventorySystem.getLockerInventory(10) != nullptr, "locker 10 should exist");
    });

    AddTest(tests, suite, "initializeLockers creates second locker", [] {
        InventorySystem inventorySystem(2, 3);
        inventorySystem.initializeLockers({10, 20});
        ExpectTrue(inventorySystem.getLockerInventory(20) != nullptr, "locker 20 should exist");
    });

    AddTest(tests, suite, "initializeLockers preserves existing contents", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.seedLockerWithItem(10, "wrench");
        inventorySystem.initializeLockers({10, 20});

        const InventoryContainer* locker = inventorySystem.getLockerInventory(10);
        ExpectTrue(locker != nullptr, "locker 10 should still exist");
        ExpectEqual(locker->getSlots()[0].itemId, std::string("wrench"), "existing locker contents should be preserved");
    });

    AddTest(tests, suite, "ensureLockerInventory returns stable storage", [] {
        InventorySystem inventorySystem(2, 2);
        InventoryContainer& first = inventorySystem.ensureLockerInventory(5);
        InventoryContainer& second = inventorySystem.ensureLockerInventory(5);
        ExpectTrue(&first == &second, "same locker id should return the same container");
    });

    AddTest(tests, suite, "missing locker inventory returns null", [] {
        InventorySystem inventorySystem(2, 2);
        ExpectTrue(inventorySystem.getLockerInventory(99) == nullptr, "missing lockers should return null");
    });

    AddTest(tests, suite, "openLocker creates missing locker", [] {
        InventorySystem inventorySystem(2, 2);
        ExpectTrue(inventorySystem.openLocker(44), "openLocker should succeed");
        ExpectTrue(inventorySystem.getLockerInventory(44) != nullptr, "opening should create locker storage");
    });

    AddTest(tests, suite, "openLocker updates active locker id", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.openLocker(44);
        ExpectEqual(inventorySystem.getOpenLockerId(), 44, "open locker id should match requested locker");
    });

    AddTest(tests, suite, "getOpenLockerInventory returns open locker", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.openLocker(44);
        ExpectTrue(inventorySystem.getOpenLockerInventory() != nullptr, "open locker inventory should be available");
    });

    AddTest(tests, suite, "closeLocker clears open flag", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.openLocker(44);
        inventorySystem.closeLocker();
        ExpectTrue(!inventorySystem.isLockerOpen(), "locker open state should reset on close");
    });

    AddTest(tests, suite, "closeLocker clears open locker id", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.openLocker(44);
        inventorySystem.closeLocker();
        ExpectEqual(inventorySystem.getOpenLockerId(), -1, "open locker id should reset on close");
    });

    AddTest(tests, suite, "closeLocker hides open inventory", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.openLocker(44);
        inventorySystem.closeLocker();
        ExpectTrue(inventorySystem.getOpenLockerInventory() == nullptr, "closed lockers should not expose inventory");
    });

    AddTest(tests, suite, "seedLockerWithItem adds items", [] {
        InventorySystem inventorySystem(2, 2);
        ExpectTrue(inventorySystem.seedLockerWithItem(7, "scrap", 2), "locker seeding should succeed");
        ExpectEqual(inventorySystem.getLockerInventory(7)->getSlots()[0].count, 2, "locker should contain seeded items");
    });

    AddTest(tests, suite, "move player item into open locker", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.getPlayerInventory().addItem("wrench");
        inventorySystem.openLocker(44);

        ExpectTrue(inventorySystem.movePlayerSlotToOpenLocker(0), "moving player item into open locker should succeed");
    });

    AddTest(tests, suite, "move player item clears player slot", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.getPlayerInventory().addItem("wrench");
        inventorySystem.openLocker(44);
        inventorySystem.movePlayerSlotToOpenLocker(0);

        ExpectTrue(inventorySystem.getPlayerInventory().getSlots()[0].isEmpty(), "player slot should be cleared after storing item");
    });

    AddTest(tests, suite, "move locker item to player", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.seedLockerWithItem(44, "wrench");
        inventorySystem.openLocker(44);

        ExpectTrue(inventorySystem.moveOpenLockerSlotToPlayer(0), "moving locker item to player should succeed");
    });

    AddTest(tests, suite, "move locker item clears locker slot", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.seedLockerWithItem(44, "wrench");
        inventorySystem.openLocker(44);
        inventorySystem.moveOpenLockerSlotToPlayer(0);

        ExpectTrue(inventorySystem.getLockerInventory(44)->getSlots()[0].isEmpty(), "locker slot should be cleared after transfer");
    });

    AddTest(tests, suite, "move player slot fails when locker is closed", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.getPlayerInventory().addItem("wrench");
        ExpectTrue(!inventorySystem.movePlayerSlotToOpenLocker(0), "cannot store items without an open locker");
    });

    AddTest(tests, suite, "move locker slot fails when locker is closed", [] {
        InventorySystem inventorySystem(2, 2);
        ExpectTrue(!inventorySystem.moveOpenLockerSlotToPlayer(0), "cannot take items without an open locker");
    });

    AddTest(tests, suite, "move player invalid slot fails", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.openLocker(44);
        ExpectTrue(!inventorySystem.movePlayerSlotToOpenLocker(9), "invalid player slot should fail");
    });

    AddTest(tests, suite, "move locker invalid slot fails", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.openLocker(44);
        ExpectTrue(!inventorySystem.moveOpenLockerSlotToPlayer(9), "invalid locker slot should fail");
    });

    AddTest(tests, suite, "selectPlayerSlot accepts valid selections", [] {
        InventorySystem inventorySystem(4, 2);
        inventorySystem.selectPlayerSlot(2);
        ExpectEqual(inventorySystem.getSelectedPlayerSlot(), 2, "valid slot selections should stick");
    });

    AddTest(tests, suite, "selectPlayerSlot rejects negative selections", [] {
        InventorySystem inventorySystem(4, 2);
        inventorySystem.selectPlayerSlot(2);
        inventorySystem.selectPlayerSlot(-1);
        ExpectEqual(inventorySystem.getSelectedPlayerSlot(), 2, "negative selections should be ignored");
    });

    AddTest(tests, suite, "selectPlayerSlot rejects out-of-range selections", [] {
        InventorySystem inventorySystem(4, 2);
        inventorySystem.selectPlayerSlot(2);
        inventorySystem.selectPlayerSlot(99);
        ExpectEqual(inventorySystem.getSelectedPlayerSlot(), 2, "out-of-range selections should be ignored");
    });

    AddTest(tests, suite, "switching lockers changes transfer destination", [] {
        InventorySystem inventorySystem(2, 2);
        inventorySystem.getPlayerInventory().addItem("wrench");
        inventorySystem.openLocker(10);
        inventorySystem.openLocker(20);
        inventorySystem.movePlayerSlotToOpenLocker(0);

        const InventoryContainer* lockerTen = inventorySystem.getLockerInventory(10);
        const InventoryContainer* lockerTwenty = inventorySystem.getLockerInventory(20);
        ExpectTrue(lockerTen->getSlots()[0].isEmpty(), "previously open locker should remain unchanged");
        ExpectEqual(lockerTwenty->getSlots()[0].itemId, std::string("wrench"), "current open locker should receive transferred item");
    });
}

void RegisterUndoManagerTests(std::vector<TestCase>& tests) {
    const std::string suite = "UndoManager suite";

    AddTest(tests, suite, "starts without undo history", [] {
        UndoManager manager;
        ExpectTrue(!manager.canUndo(), "new manager should not be undoable");
    });

    AddTest(tests, suite, "starts without redo history", [] {
        UndoManager manager;
        ExpectTrue(!manager.canRedo(), "new manager should not be redoable");
    });

    AddTest(tests, suite, "pushAction enables undo", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Transform, 3));
        ExpectTrue(manager.canUndo(), "pushing an action should enable undo");
    });

    AddTest(tests, suite, "popUndo returns pushed object id", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Transform, 7));
        ExpectEqual(manager.popUndo().objectId, 7, "popUndo should return the pushed action");
    });

    AddTest(tests, suite, "popUndo moves action to redo", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Transform, 7));
        manager.popUndo();
        ExpectTrue(manager.canRedo(), "undoing should make the action redoable");
    });

    AddTest(tests, suite, "popRedo returns last undone object id", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Transform, 7));
        manager.popUndo();
        ExpectEqual(manager.popRedo().objectId, 7, "redo should return the most recently undone action");
    });

    AddTest(tests, suite, "popRedo restores undo history", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Transform, 7));
        manager.popUndo();
        manager.popRedo();
        ExpectTrue(manager.canUndo(), "redoing should put the action back on the undo stack");
    });

    AddTest(tests, suite, "empty popUndo returns default object id", [] {
        UndoManager manager;
        ExpectEqual(manager.popUndo().objectId, 0, "empty undo should return a default-initialized action");
    });

    AddTest(tests, suite, "empty popUndo returns null before state", [] {
        UndoManager manager;
        ExpectTrue(manager.popUndo().objectStateBefore.is_null(), "empty undo should return null json state");
    });

    AddTest(tests, suite, "empty popRedo returns default object id", [] {
        UndoManager manager;
        ExpectEqual(manager.popRedo().objectId, 0, "empty redo should return a default-initialized action");
    });

    AddTest(tests, suite, "multiple undo uses LIFO order for latest action", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Create, 1));
        manager.pushAction(MakeAction(UndoManager::Action::Delete, 2));
        ExpectEqual(manager.popUndo().objectId, 2, "latest action should be undone first");
    });

    AddTest(tests, suite, "multiple undo exposes earlier action next", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Create, 1));
        manager.pushAction(MakeAction(UndoManager::Action::Delete, 2));
        manager.popUndo();
        ExpectEqual(manager.popUndo().objectId, 1, "earlier action should be undone second");
    });

    AddTest(tests, suite, "multiple redo uses reverse undo order", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Create, 1));
        manager.pushAction(MakeAction(UndoManager::Action::Delete, 2));
        manager.popUndo();
        manager.popUndo();
        ExpectEqual(manager.popRedo().objectId, 1, "first redo should restore the oldest undone action");
    });

    AddTest(tests, suite, "pushAction clears redo history", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Create, 1));
        manager.popUndo();
        manager.pushAction(MakeAction(UndoManager::Action::Delete, 2));
        ExpectTrue(!manager.canRedo(), "pushing a new action should clear redo history");
    });

    AddTest(tests, suite, "preserves objectStateBefore payload", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Transform, 5, json{{"x", 1}}, json{{"x", 2}}));
        ExpectEqual(manager.popUndo().objectStateBefore["x"].get<int>(), 1, "before state should be preserved");
    });

    AddTest(tests, suite, "preserves objectStateAfter payload", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Transform, 5, json{{"x", 1}}, json{{"x", 2}}));
        ExpectEqual(manager.popUndo().objectStateAfter["x"].get<int>(), 2, "after state should be preserved");
    });

    AddTest(tests, suite, "preserves action type", [] {
        UndoManager manager;
        manager.pushAction(MakeAction(UndoManager::Action::Duplicate, 5));
        ExpectEqual(static_cast<int>(manager.popUndo().type), static_cast<int>(UndoManager::Action::Duplicate), "action type should be preserved");
    });
}

void RegisterCameraAndRaycastTests(std::vector<TestCase>& tests) {
    const std::string suite = "CameraAndRaycast suite";

    AddTest(tests, suite, "default forward direction faces negative z", [] {
        Camera camera(800, 600);
        ExpectVec3Equal(camera.getForwardDirection(), {0.0f, 0.0f, -1.0f}, "default forward direction");
    });

    AddTest(tests, suite, "setLookDirection ignores zero vectors", [] {
        Camera camera(800, 600);
        const glm::vec3 before = camera.getForwardDirection();
        camera.setLookDirection({0.0f, 0.0f, 0.0f});
        ExpectVec3Equal(camera.getForwardDirection(), before, "zero look directions should be ignored");
    });

    AddTest(tests, suite, "setLookDirection supports positive x", [] {
        Camera camera(800, 600);
        camera.setLookDirection({1.0f, 0.0f, 0.0f});
        ExpectVec3Equal(camera.getForwardDirection(), {1.0f, 0.0f, 0.0f}, "camera should face positive x");
    });

    AddTest(tests, suite, "setLookDirection supports positive y", [] {
        Camera camera(800, 600);
        camera.setLookDirection({0.0f, 1.0f, 0.0f});
        ExpectVec3Equal(camera.getForwardDirection(), {0.0f, 1.0f, 0.0f}, "camera should face positive y");
    });

    AddTest(tests, suite, "lookAt targets requested point", [] {
        Camera camera(800, 600, {0.0f, 0.0f, 0.0f});
        camera.lookAt({5.0f, 0.0f, 0.0f});
        ExpectVec3Equal(camera.getForwardDirection(), {1.0f, 0.0f, 0.0f}, "lookAt should orient the camera toward the target");
    });

    AddTest(tests, suite, "rotateViewDirection updates yaw", [] {
        Camera camera(800, 600);
        camera.rotateViewDirection({0.0f, 0.5f});
        ExpectNear(camera.getRotation().y, -0.5f, 0.0001f, "yaw should update from mouse delta");
    });

    AddTest(tests, suite, "rotateViewDirection updates pitch", [] {
        Camera camera(800, 600);
        camera.rotateViewDirection({0.25f, 0.0f});
        ExpectNear(camera.getRotation().x, 0.25f, 0.0001f, "pitch should update from mouse delta");
    });

    AddTest(tests, suite, "rotateViewDirection clamps positive pitch", [] {
        Camera camera(800, 600);
        camera.rotateViewDirection({10.0f, 0.0f});
        ExpectTrue(camera.getRotation().x < static_cast<float>(M_PI_2), "pitch should stay below ninety degrees");
    });

    AddTest(tests, suite, "rotateViewDirection clamps negative pitch", [] {
        Camera camera(800, 600);
        camera.rotateViewDirection({-10.0f, 0.0f});
        ExpectTrue(camera.getRotation().x > -static_cast<float>(M_PI_2), "pitch should stay above negative ninety degrees");
    });

    AddTest(tests, suite, "move vec3 translates position directly", [] {
        Camera camera(800, 600);
        camera.move(glm::vec3(1.0f, 2.0f, 3.0f));
        ExpectVec3Equal(camera.getPosition(), {1.0f, 2.0f, 3.0f}, "vec3 movement should translate position directly");
    });

    AddTest(tests, suite, "moveVertical only changes y", [] {
        Camera camera(800, 600, {1.0f, 2.0f, 3.0f});
        camera.moveVertical(4.0f);
        ExpectVec3Equal(camera.getPosition(), {1.0f, 6.0f, 3.0f}, "vertical movement should only affect y");
    });

    AddTest(tests, suite, "move vec2 moves forward in view direction", [] {
        Camera camera(800, 600);
        camera.move(glm::vec2(2.0f, 0.0f));
        ExpectVec3Equal(camera.getPosition(), {0.0f, 0.0f, -2.0f}, "forward movement should follow camera forward");
    });

    AddTest(tests, suite, "move vec2 strafes along camera right", [] {
        Camera camera(800, 600);
        camera.move(glm::vec2(0.0f, 1.0f));
        ExpectVec3Equal(camera.getPosition(), {1.0f, 0.0f, 0.0f}, "strafe movement should follow camera right");
    });

    AddTest(tests, suite, "move2D ignores pitch when moving", [] {
        Camera camera(800, 600);
        camera.setRotation({1.0f, 0.0f, 0.0f});
        camera.move2D({1.0f, 0.0f});
        ExpectNear(camera.getPosition().y, 0.0f, 0.0001f, "2D movement should stay on the horizontal plane");
    });

    AddTest(tests, suite, "move2D stays finite when looking straight up", [] {
        Camera camera(800, 600);
        camera.setLookDirection({0.0f, 1.0f, 0.0f});
        camera.move2D({1.0f, 1.0f});
        ExpectVec3Finite(camera.getPosition(), "2D movement should not produce NaN positions");
    });

    AddTest(tests, suite, "move vec2 stays finite when looking straight up", [] {
        Camera camera(800, 600);
        camera.setLookDirection({0.0f, 1.0f, 0.0f});
        camera.move(glm::vec2(1.0f, 1.0f));
        ExpectVec3Finite(camera.getPosition(), "3D movement should not produce NaN positions");
    });

    AddTest(tests, suite, "updateProjection changes matrix values", [] {
        Camera camera(800, 600);
        const glm::mat4 before = camera.getProjectionMatrix();
        camera.updateProjection(1600, 600);
        const glm::mat4 after = camera.getProjectionMatrix();
        ExpectTrue(std::fabs(before[0][0] - after[0][0]) > 0.0001f, "projection matrix should change with aspect ratio");
    });

    AddTest(tests, suite, "raycast center follows forward direction", [] {
        Camera camera(800, 600);
        const glm::vec3 ray = Raycast::getRayFromScreen(400.0f, 300.0f, 800, 600, &camera);
        ExpectVec3Equal(ray, camera.getForwardDirection(), "center-screen ray should match the forward direction", 0.0002f);
    });

    AddTest(tests, suite, "raycast results stay normalized", [] {
        Camera camera(800, 600);
        const glm::vec3 ray = Raycast::getRayFromScreen(0.0f, 0.0f, 800, 600, &camera);
        ExpectNear(glm::length(ray), 1.0f, 0.0002f, "screen rays should be normalized");
    });

    AddTest(tests, suite, "raycast honors camera rotation", [] {
        Camera camera(800, 600);
        camera.setLookDirection({1.0f, 0.0f, 0.0f});
        const glm::vec3 ray = Raycast::getRayFromScreen(400.0f, 300.0f, 800, 600, &camera);
        ExpectVec3Equal(ray, {1.0f, 0.0f, 0.0f}, "center ray should rotate with the camera", 0.0002f);
    });
}

void RegisterAabbAndFrustumTests(std::vector<TestCase>& tests) {
    const std::string suite = "AABBAndFrustum suite";

    AddTest(tests, suite, "identity transform preserves min", [] {
        AABB box({-1.0f, -2.0f, -3.0f}, {4.0f, 5.0f, 6.0f});
        AABB transformed = box.transformed(glm::mat4(1.0f));
        ExpectVec3Equal(transformed.min, box.min, "identity transform should preserve min");
    });

    AddTest(tests, suite, "identity transform preserves max", [] {
        AABB box({-1.0f, -2.0f, -3.0f}, {4.0f, 5.0f, 6.0f});
        AABB transformed = box.transformed(glm::mat4(1.0f));
        ExpectVec3Equal(transformed.max, box.max, "identity transform should preserve max");
    });

    AddTest(tests, suite, "translation transform shifts min", [] {
        AABB box({-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
        AABB transformed = box.transformed(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f)));
        ExpectVec3Equal(transformed.min, {1.0f, 2.0f, 3.0f}, "translation should shift min");
    });

    AddTest(tests, suite, "translation transform shifts max", [] {
        AABB box({-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
        AABB transformed = box.transformed(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f)));
        ExpectVec3Equal(transformed.max, {3.0f, 4.0f, 5.0f}, "translation should shift max");
    });

    AddTest(tests, suite, "scale transform updates min", [] {
        AABB box({-1.0f, -2.0f, -3.0f}, {1.0f, 2.0f, 3.0f});
        AABB transformed = box.transformed(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f)));
        ExpectVec3Equal(transformed.min, {-2.0f, -6.0f, -12.0f}, "scale should update min");
    });

    AddTest(tests, suite, "scale transform updates max", [] {
        AABB box({-1.0f, -2.0f, -3.0f}, {1.0f, 2.0f, 3.0f});
        AABB transformed = box.transformed(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f)));
        ExpectVec3Equal(transformed.max, {2.0f, 6.0f, 12.0f}, "scale should update max");
    });

    AddTest(tests, suite, "rotation transform expands swapped extents", [] {
        AABB box({-1.0f, -1.0f, -2.0f}, {1.0f, 1.0f, 2.0f});
        AABB transformed = box.transformed(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        ExpectVec3Equal(transformed.min, {-2.0f, -1.0f, -1.0f}, "rotation should swap horizontal extents", 0.0002f);
        ExpectVec3Equal(transformed.max, {2.0f, 1.0f, 1.0f}, "rotation should swap horizontal extents", 0.0002f);
    });

    AddTest(tests, suite, "outsidePlane detects separated boxes", [] {
        AABB box({0.0f, 0.0f, 0.0f}, {3.0f, 1.0f, 1.0f});
        ExpectTrue(box.isOutsidePlane(glm::vec4(1.0f, 0.0f, 0.0f, -5.0f)), "box should be outside when even its positive vertex fails the plane");
    });

    AddTest(tests, suite, "outsidePlane keeps intersecting boxes visible", [] {
        AABB box({0.0f, 0.0f, 0.0f}, {3.0f, 1.0f, 1.0f});
        ExpectTrue(!box.isOutsidePlane(glm::vec4(1.0f, 0.0f, 0.0f, -1.0f)), "box should remain visible when it intersects the plane");
    });

    AddTest(tests, suite, "frustum sees centered boxes", [] {
        Frustum frustum;
        frustum.extract(glm::mat4(1.0f));
        ExpectTrue(frustum.isBoxVisible(AABB({-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f})), "identity frustum should see centered boxes");
    });

    AddTest(tests, suite, "frustum rejects boxes outside clip space", [] {
        Frustum frustum;
        frustum.extract(glm::mat4(1.0f));
        ExpectTrue(!frustum.isBoxVisible(AABB({2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f})), "identity frustum should reject distant boxes");
    });

    AddTest(tests, suite, "frustum keeps intersecting boxes visible", [] {
        Frustum frustum;
        frustum.extract(glm::mat4(1.0f));
        ExpectTrue(frustum.isBoxVisible(AABB({0.9f, 0.9f, 0.9f}, {1.1f, 1.1f, 1.1f})), "boxes intersecting the frustum edge should stay visible");
    });

    AddTest(tests, suite, "frustum planes are normalized", [] {
        Frustum frustum;
        frustum.extract(glm::mat4(1.0f));
        for (const glm::vec4& plane : frustum.planes) {
            ExpectNear(glm::length(glm::vec3(plane)), 1.0f, 0.0002f, "plane normals should be normalized");
        }
    });

    AddTest(tests, suite, "expand grows min and max", [] {
        AABB box({1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f});
        box.expand({-2.0f, 3.0f, 0.0f});
        ExpectVec3Equal(box.min, {-2.0f, 1.0f, 0.0f}, "expand should grow min");
        ExpectVec3Equal(box.max, {1.0f, 3.0f, 1.0f}, "expand should grow max");
    });

    AddTest(tests, suite, "transformed boxes preserve min max ordering", [] {
        AABB box({-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
        AABB transformed = box.transformed(glm::scale(glm::mat4(1.0f), glm::vec3(-2.0f, 3.0f, 4.0f)));
        ExpectTrue(transformed.min.x <= transformed.max.x, "transformed x extents should stay ordered");
        ExpectTrue(transformed.min.y <= transformed.max.y, "transformed y extents should stay ordered");
        ExpectTrue(transformed.min.z <= transformed.max.z, "transformed z extents should stay ordered");
    });
}

void RegisterFileItemAndLightTests(std::vector<TestCase>& tests) {
    const std::string suite = "FileItemAndLight suite";

    AddTest(tests, suite, "wrench definition exists", [] {
        ExpectTrue(GetItemDefinitionById("wrench") != nullptr, "wrench definition should exist");
    });

    AddTest(tests, suite, "wrench max stack is one", [] {
        ExpectEqual(GetItemDefinitionById("wrench")->maxStack, 1, "wrench should be non-stackable");
    });

    AddTest(tests, suite, "scrap definition exists", [] {
        ExpectTrue(GetItemDefinitionById("scrap") != nullptr, "scrap definition should exist");
    });

    AddTest(tests, suite, "scrap max stack is five", [] {
        ExpectEqual(GetItemDefinitionById("scrap")->maxStack, 5, "scrap should stack to five");
    });

    AddTest(tests, suite, "unknown definitions return null", [] {
        ExpectTrue(GetItemDefinitionById("unknown_item") == nullptr, "unknown item ids should return null");
    });

    AddTest(tests, suite, "FileManager reads file contents", [] {
        const std::string path = "test_file_manager_read.txt";
        {
            std::ofstream output(path);
            output << "hello from tests";
        }

        const std::string contents = FileManager::read(path);
        std::remove(path.c_str());
        ExpectEqual(contents, std::string("hello from tests"), "FileManager should read file contents");
    });

    AddTest(tests, suite, "FileManager missing files return empty strings", [] {
        const std::string contents = FileManager::read("definitely_missing_test_file.txt");
        ExpectEqual(contents, std::string(""), "missing files should return empty strings");
    });

    AddTest(tests, suite, "FileManager reads scene json", [] {
        const std::string contents = FileManager::read("scene.json");
        ExpectTrue(!contents.empty(), "scene.json should be readable during tests");
        ExpectContains(contents, "objects", "scene.json should contain serialized objects");
    });

    AddTest(tests, suite, "scene json parses into object array", [] {
        const json sceneJson = json::parse(FileManager::read("scene.json"));
        ExpectTrue(sceneJson.contains("objects"), "scene json should contain an objects key");
        ExpectTrue(sceneJson["objects"].is_array(), "scene json objects should deserialize as an array");
    });

    AddTest(tests, suite, "point lights report light type", [] {
        PointLight light({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, 2.0f);
        ExpectTrue(light.isLight(), "point lights should report themselves as lights");
    });

    AddTest(tests, suite, "point lights report point light subtype", [] {
        PointLight light({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, 2.0f);
        ExpectTrue(light.isPointLight(), "point lights should report point-light subtype");
    });

    AddTest(tests, suite, "point lights are not spotlights", [] {
        PointLight light({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, 2.0f);
        ExpectTrue(!light.isSpotLight(), "point lights should not report spotlight subtype");
    });

    AddTest(tests, suite, "point lights preserve intensity", [] {
        PointLight light({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, 2.0f);
        light.setIntensity(4.5f);
        ExpectNear(light.getIntensity(), 4.5f, 0.0001f, "point light intensity should round-trip");
    });

    AddTest(tests, suite, "point lights preserve color", [] {
        PointLight light({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, 2.0f);
        ExpectVec3Equal(light.getColor(), {0.5f, 0.6f, 0.7f}, "point light color should round-trip");
    });

    AddTest(tests, suite, "point lights preserve active flag", [] {
        PointLight light({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, 2.0f, false);
        ExpectTrue(!light.isActive(), "point light active flag should round-trip");
    });

    AddTest(tests, suite, "spotlights report light type", [] {
        SpotLight light({1.0f, 2.0f, 3.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        ExpectTrue(light.isLight(), "spotlights should report themselves as lights");
    });

    AddTest(tests, suite, "spotlights report spotlight subtype", [] {
        SpotLight light({1.0f, 2.0f, 3.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        ExpectTrue(light.isSpotLight(), "spotlights should report spotlight subtype");
    });

    AddTest(tests, suite, "spotlights are not point lights", [] {
        SpotLight light({1.0f, 2.0f, 3.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        ExpectTrue(!light.isPointLight(), "spotlights should not report point-light subtype");
    });

    AddTest(tests, suite, "spotlights normalize constructor direction", [] {
        SpotLight light({1.0f, 2.0f, 3.0f}, {0.0f, -2.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        ExpectVec3Equal(light.getDirection(), {0.0f, -1.0f, 0.0f}, "spotlight direction should be normalized on construction");
    });

    AddTest(tests, suite, "spotlights default zero directions downward", [] {
        SpotLight light({1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        ExpectVec3Equal(light.getDirection(), {0.0f, -1.0f, 0.0f}, "zero spotlight direction should fall back to down");
    });

    AddTest(tests, suite, "spotlights renormalize direction updates", [] {
        SpotLight light({1.0f, 2.0f, 3.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        light.setDirection({10.0f, 0.0f, 0.0f});
        ExpectVec3Equal(light.getDirection(), {1.0f, 0.0f, 0.0f}, "spotlight direction updates should be normalized");
    });

    AddTest(tests, suite, "spotlights preserve cutoff", [] {
        SpotLight light({1.0f, 2.0f, 3.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        light.setCutoff(25.0f);
        ExpectNear(light.getCutoff(), 25.0f, 0.0001f, "spotlight cutoff should round-trip");
    });

    AddTest(tests, suite, "spotlights preserve active flag", [] {
        SpotLight light({1.0f, 2.0f, 3.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 1.0f, 12.5f, false);
        ExpectTrue(!light.isActive(), "spotlight active flag should round-trip");
    });
}

std::vector<TestCase> BuildTests() {
    std::vector<TestCase> tests;
    tests.reserve(140);

    RegisterSceneObjectTests(tests);
    RegisterInventoryContainerTests(tests);
    RegisterInventorySystemTests(tests);
    RegisterUndoManagerTests(tests);
    RegisterCameraAndRaycastTests(tests);
    RegisterAabbAndFrustumTests(tests);
    RegisterFileItemAndLightTests(tests);

    return tests;
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<TestCase> tests = BuildTests();
    const std::string requestedSuite = argc > 1 ? argv[1] : "";

    int failedCount = 0;
    int executedCount = 0;

    for (const TestCase& test : tests) {
        if (!requestedSuite.empty() && test.suite != requestedSuite) {
            continue;
        }

        executedCount++;
        try {
            test.run();
            std::cout << "[PASS] " << test.suite << " :: " << test.name << '\n';
        } catch (const std::exception& ex) {
            failedCount++;
            std::cerr << "[FAIL] " << test.suite << " :: " << test.name << ": " << ex.what() << '\n';
        }
    }

    if (executedCount == 0) {
        std::cerr << "Unknown test suite: " << requestedSuite << '\n';
        return 1;
    }

    if (failedCount > 0) {
        std::cerr << failedCount << " test(s) failed out of " << executedCount << ".\n";
        return 1;
    }

    std::cout << executedCount << " test(s) passed.\n";
    return 0;
}
