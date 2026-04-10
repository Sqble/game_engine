#include "../src/game/inventory/inventory_container.h"
#include "../src/game/inventory/inventory_system.h"
#include "../src/sceneobject/sceneobject.h"

#include <glm/glm.hpp>

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

template <typename T>
void ExpectEqual(const T& actual, const T& expected, const std::string& message) {
    if (!(actual == expected)) {
        std::ostringstream stream;
        stream << message << " expected " << expected << " but got " << actual;
        throw TestFailure(stream.str());
    }
}

void ExpectVec3Equal(const glm::vec3& actual, const glm::vec3& expected, const std::string& message) {
    const float epsilon = 0.0001f;
    if (std::fabs(actual.x - expected.x) > epsilon ||
        std::fabs(actual.y - expected.y) > epsilon ||
        std::fabs(actual.z - expected.z) > epsilon) {
        throw TestFailure(message + " expected " + Vec3ToString(expected) + " but got " + Vec3ToString(actual));
    }
}

void RunSubtest(const std::string& name, const std::function<void()>& test) {
    try {
        test();
    } catch (const std::exception& ex) {
        throw TestFailure(name + ": " + ex.what());
    }
}

void TestSceneObjectParentKeepsWorldTransform() {
    SceneObject::setIDCounter(0);
    DummySceneObject parent({10.0f, 5.0f, -2.0f}, {2.0f, 4.0f, 8.0f}, {1.0f, 1.0f, 1.0f}, {5.0f, 10.0f, 15.0f});
    DummySceneObject child({14.0f, 9.0f, 6.0f}, {6.0f, 12.0f, 16.0f}, {1.0f, 1.0f, 1.0f}, {20.0f, 35.0f, 50.0f});

    ExpectTrue(parent.addChild(&child, true), "addChild should succeed");
    ExpectTrue(child.getParent() == &parent, "child parent should be set");
    ExpectEqual(child.getParentId(), parent.getId(), "child parent id should match");
    ExpectVec3Equal(child.getPosition(), {14.0f, 9.0f, 6.0f}, "child world position should stay the same");
    ExpectVec3Equal(child.getLocalPosition(), {4.0f, 4.0f, 8.0f}, "child local position should be rebased");
    ExpectVec3Equal(child.getSize(), {6.0f, 12.0f, 16.0f}, "child world scale should stay the same");
    ExpectVec3Equal(child.getLocalSize(), {3.0f, 3.0f, 2.0f}, "child local scale should be normalized by parent");
    ExpectVec3Equal(child.getRotation(), {20.0f, 35.0f, 50.0f}, "child world rotation should stay the same");
    ExpectVec3Equal(child.getLocalRotation(), {15.0f, 25.0f, 35.0f}, "child local rotation should be rebased");
}

void TestSceneObjectParentWithoutKeepingWorldTransform() {
    SceneObject::setIDCounter(0);
    DummySceneObject parent({3.0f, 4.0f, 5.0f}, {2.0f, 3.0f, 4.0f}, {1.0f, 1.0f, 1.0f}, {10.0f, 20.0f, 30.0f});
    DummySceneObject child({1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {1.0f, 1.0f, 1.0f}, {7.0f, 8.0f, 9.0f});

    ExpectTrue(child.setParent(&parent, false), "setParent should succeed without keeping world transform");
    ExpectVec3Equal(child.getLocalPosition(), {1.0f, 2.0f, 3.0f}, "local position should remain unchanged");
    ExpectVec3Equal(child.getPosition(), {4.0f, 6.0f, 8.0f}, "world position should be parent plus local");
    ExpectVec3Equal(child.getSize(), {8.0f, 15.0f, 24.0f}, "world scale should be parent multiplied by local");
    ExpectVec3Equal(child.getRotation(), {17.0f, 28.0f, 39.0f}, "world rotation should be parent plus local");
}

void TestSceneObjectPreventsCyclesAndDetachKeepsWorldTransform() {
    SceneObject::setIDCounter(0);
    DummySceneObject root({0.0f, 0.0f, 0.0f});
    DummySceneObject parent({10.0f, 0.0f, 0.0f});
    DummySceneObject child({15.0f, 1.0f, 0.0f});

    ExpectTrue(root.addChild(&parent, true), "root should adopt parent");
    ExpectTrue(parent.addChild(&child, true), "parent should adopt child");
    ExpectTrue(!root.setParent(&child, true), "setting a descendant as parent should fail");

    child.detach(true);
    ExpectTrue(child.getParent() == nullptr, "detached child should not have a parent");
    ExpectVec3Equal(child.getPosition(), {15.0f, 1.0f, 0.0f}, "detached child world position should be preserved");
}

void TestSceneObjectEffectiveActiveFollowsParent() {
    SceneObject::setIDCounter(0);
    DummySceneObject parent({0.0f, 0.0f, 0.0f});
    DummySceneObject child({1.0f, 0.0f, 0.0f});

    ExpectTrue(parent.addChild(&child, true), "parenting should succeed");
    ExpectTrue(child.isEffectivelyActive(), "child should start active");

    parent.setActive(false);
    ExpectTrue(!child.isEffectivelyActive(), "child should inherit inactive state");

    parent.setActive(true);
    child.setActive(false);
    ExpectTrue(!child.isEffectivelyActive(), "child local inactive state should win");
}

void TestSceneObjectDetachChildrenKeepsWorldTransform() {
    SceneObject::setIDCounter(0);
    DummySceneObject parent({10.0f, 0.0f, 0.0f});
    DummySceneObject childA({11.0f, 1.0f, 0.0f});
    DummySceneObject childB({12.0f, 2.0f, 0.0f});

    ExpectTrue(parent.addChild(&childA, true), "first child should be parented");
    ExpectTrue(parent.addChild(&childB, true), "second child should be parented");

    parent.detachChildren(true);

    ExpectTrue(parent.getChildren().empty(), "parent should have no children after detachChildren");
    ExpectTrue(childA.getParent() == nullptr, "first child should be detached");
    ExpectTrue(childB.getParent() == nullptr, "second child should be detached");
    ExpectVec3Equal(childA.getPosition(), {11.0f, 1.0f, 0.0f}, "first child world position should stay the same");
    ExpectVec3Equal(childB.getPosition(), {12.0f, 2.0f, 0.0f}, "second child world position should stay the same");
}

void TestSceneObjectReparentingMovesChildBetweenParents() {
    SceneObject::setIDCounter(0);
    DummySceneObject firstParent({0.0f, 0.0f, 0.0f});
    DummySceneObject secondParent({5.0f, 0.0f, 0.0f});
    DummySceneObject child({2.0f, 0.0f, 0.0f});

    ExpectTrue(firstParent.addChild(&child, true), "initial parenting should succeed");
    ExpectTrue(secondParent.addChild(&child, true), "reparenting should succeed");
    ExpectTrue(firstParent.getChildren().empty(), "old parent should lose the child");
    ExpectEqual(secondParent.getChildren().size(), static_cast<size_t>(1), "new parent should have one child");
    ExpectTrue(secondParent.getChildren().front() == &child, "new parent should contain the child");
    ExpectVec3Equal(child.getPosition(), {2.0f, 0.0f, 0.0f}, "child world position should remain unchanged when reparented");
}

void TestInventoryContainerSplitsStacksAcrossSlots() {
    InventoryContainer container(3);

    ExpectTrue(container.addItem("scrap", 7), "scrap should fit across multiple slots");
    const std::vector<InventorySlot>& slots = container.getSlots();
    ExpectEqual(slots[0].itemId, std::string("scrap"), "first slot item id");
    ExpectEqual(slots[0].count, 5, "first slot should fill to max stack");
    ExpectEqual(slots[1].itemId, std::string("scrap"), "second slot item id");
    ExpectEqual(slots[1].count, 2, "second slot should contain remainder");
    ExpectTrue(slots[2].isEmpty(), "third slot should remain empty");
}

void TestInventoryContainerAddItemIsAtomicOnFailure() {
    InventoryContainer container(1);
    ExpectTrue(container.setSlot(0, InventorySlot{"scrap", 4}), "should seed slot");

    ExpectTrue(!container.addItem("scrap", 2), "adding beyond capacity should fail");
    const InventorySlot& slot = container.getSlots()[0];
    ExpectEqual(slot.itemId, std::string("scrap"), "slot item id should be unchanged after failure");
    ExpectEqual(slot.count, 4, "slot count should be unchanged after failure");
}

void TestInventoryContainerMoveSlotTransfersAndClearsSource() {
    InventoryContainer source(2);
    InventoryContainer target(2);
    ExpectTrue(source.setSlot(0, InventorySlot{"scrap", 3}), "source should be seeded");

    ExpectTrue(source.moveSlotTo(0, target), "moveSlotTo should succeed with available capacity");
    ExpectTrue(source.getSlots()[0].isEmpty(), "source slot should be cleared after move");
    ExpectEqual(target.getSlots()[0].itemId, std::string("scrap"), "target should receive the item");
    ExpectEqual(target.getSlots()[0].count, 3, "target should receive the full stack count");
}

void TestInventoryContainerMoveSlotFailsWhenTargetIsFull() {
    InventoryContainer source(1);
    InventoryContainer target(1);
    ExpectTrue(source.setSlot(0, InventorySlot{"wrench", 1}), "source should be seeded");
    ExpectTrue(target.setSlot(0, InventorySlot{"wrench", 1}), "target should be seeded");

    ExpectTrue(!source.moveSlotTo(0, target), "moveSlotTo should fail when target cannot accept item");
    ExpectEqual(source.getSlots()[0].itemId, std::string("wrench"), "source should retain item on failed move");
    ExpectEqual(source.getSlots()[0].count, 1, "source should retain count on failed move");
}

void TestInventorySystemTracksLockerContentsByObjectId() {
    InventorySystem inventorySystem(3, 2);
    inventorySystem.initializeLockers({10, 20});
    ExpectTrue(inventorySystem.seedLockerWithItem(20, "wrench"), "should seed locker item");

    const InventoryContainer* lockerTen = inventorySystem.getLockerInventory(10);
    const InventoryContainer* lockerTwenty = inventorySystem.getLockerInventory(20);
    ExpectTrue(lockerTen != nullptr, "locker 10 should exist");
    ExpectTrue(lockerTwenty != nullptr, "locker 20 should exist");
    ExpectTrue(lockerTen->getSlots()[0].isEmpty(), "locker 10 should remain empty");
    ExpectEqual(lockerTwenty->getSlots()[0].itemId, std::string("wrench"), "locker 20 should contain wrench");

    ExpectTrue(inventorySystem.openLocker(20), "opening locker 20 should succeed");
    ExpectTrue(inventorySystem.moveOpenLockerSlotToPlayer(0), "moving from locker to player should succeed");
    ExpectEqual(inventorySystem.getPlayerInventory().getSlots()[0].itemId, std::string("wrench"), "player should receive wrench");
    ExpectTrue(inventorySystem.getLockerInventory(20)->getSlots()[0].isEmpty(), "locker 20 slot should now be empty");
}

void TestInventorySystemRejectsInvalidSelectedSlot() {
    InventorySystem inventorySystem(4, 2);
    inventorySystem.selectPlayerSlot(2);
    ExpectEqual(inventorySystem.getSelectedPlayerSlot(), 2, "valid slot selection should stick");

    inventorySystem.selectPlayerSlot(99);
    ExpectEqual(inventorySystem.getSelectedPlayerSlot(), 2, "invalid slot selection should be ignored");
}

void TestInventorySystemMovesPlayerItemIntoOpenLocker() {
    InventorySystem inventorySystem(3, 2);
    ExpectTrue(inventorySystem.getPlayerInventory().addItem("wrench"), "player inventory should accept wrench");
    ExpectTrue(inventorySystem.openLocker(44), "locker should open");

    ExpectTrue(inventorySystem.movePlayerSlotToOpenLocker(0), "moving player slot into open locker should succeed");
    ExpectTrue(inventorySystem.getPlayerInventory().getSlots()[0].isEmpty(), "player slot should be cleared after storing");

    const InventoryContainer* locker = inventorySystem.getLockerInventory(44);
    ExpectTrue(locker != nullptr, "locker should exist after opening");
    ExpectEqual(locker->getSlots()[0].itemId, std::string("wrench"), "open locker should receive stored item");
}

void TestInventorySystemFailsTransfersWithoutOpenLocker() {
    InventorySystem inventorySystem(2, 1);
    ExpectTrue(inventorySystem.getPlayerInventory().addItem("wrench"), "player should get an item");

    ExpectTrue(!inventorySystem.movePlayerSlotToOpenLocker(0), "storing without an open locker should fail");
    ExpectTrue(!inventorySystem.moveOpenLockerSlotToPlayer(0), "taking without an open locker should fail");
    ExpectEqual(inventorySystem.getPlayerInventory().getSlots()[0].itemId, std::string("wrench"), "player item should remain in place");
    ExpectTrue(inventorySystem.getOpenLockerInventory() == nullptr, "open locker inventory should be null while closed");
}

void TestInventorySystemCloseLockerClearsOpenState() {
    InventorySystem inventorySystem(2, 1);
    ExpectTrue(inventorySystem.openLocker(7), "locker should open");
    ExpectTrue(inventorySystem.isLockerOpen(), "locker open state should be true");

    inventorySystem.closeLocker();
    ExpectTrue(!inventorySystem.isLockerOpen(), "locker open state should be false after close");
    ExpectEqual(inventorySystem.getOpenLockerId(), -1, "open locker id should reset on close");
    ExpectTrue(inventorySystem.getOpenLockerInventory() == nullptr, "open locker inventory should be null after close");
}

void RunSceneObjectSuite() {
    RunSubtest("keep world transform when parented", TestSceneObjectParentKeepsWorldTransform);
    RunSubtest("apply parent transform without keep-world", TestSceneObjectParentWithoutKeepingWorldTransform);
    RunSubtest("prevent cycles and detach safely", TestSceneObjectPreventsCyclesAndDetachKeepsWorldTransform);
    RunSubtest("effective active state follows hierarchy", TestSceneObjectEffectiveActiveFollowsParent);
    RunSubtest("detachChildren preserves world transforms", TestSceneObjectDetachChildrenKeepsWorldTransform);
    RunSubtest("reparenting moves child between parents", TestSceneObjectReparentingMovesChildBetweenParents);
}

void RunInventoryContainerSuite() {
    RunSubtest("split stackable items", TestInventoryContainerSplitsStacksAcrossSlots);
    RunSubtest("addItem is atomic on failure", TestInventoryContainerAddItemIsAtomicOnFailure);
    RunSubtest("moveSlotTo transfers and clears source", TestInventoryContainerMoveSlotTransfersAndClearsSource);
    RunSubtest("moveSlotTo fails when target is full", TestInventoryContainerMoveSlotFailsWhenTargetIsFull);
}

void RunInventorySystemSuite() {
    RunSubtest("track locker contents by object id", TestInventorySystemTracksLockerContentsByObjectId);
    RunSubtest("ignore invalid selected slot", TestInventorySystemRejectsInvalidSelectedSlot);
    RunSubtest("move player item into open locker", TestInventorySystemMovesPlayerItemIntoOpenLocker);
    RunSubtest("fail transfers without open locker", TestInventorySystemFailsTransfersWithoutOpenLocker);
    RunSubtest("close locker clears open state", TestInventorySystemCloseLockerClearsOpenState);
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> suites = {
        {"SceneObject suite", RunSceneObjectSuite},
        {"InventoryContainer suite", RunInventoryContainerSuite},
        {"InventorySystem suite", RunInventorySystemSuite},
    };

    if (argc > 1) {
        const std::string requestedTest = argv[1];
        for (const auto& [name, test] : suites) {
            if (name != requestedTest) {
                continue;
            }

            try {
                test();
                std::cout << "[PASS] " << name << '\n';
                return 0;
            } catch (const std::exception& ex) {
                std::cerr << "[FAIL] " << name << ": " << ex.what() << '\n';
                return 1;
            }
        }

        std::cerr << "Unknown test: " << requestedTest << '\n';
        return 1;
    }

    int failedCount = 0;
    for (const auto& [name, test] : suites) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& ex) {
            failedCount++;
            std::cerr << "[FAIL] " << name << ": " << ex.what() << '\n';
        }
    }

    if (failedCount > 0) {
        std::cerr << failedCount << " test(s) failed.\n";
        return 1;
    }

    std::cout << suites.size() << " test suite(s) passed.\n";
    return 0;
}
