#include <DeletionQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>
#include <vk_utils.h>

TEST(UtilTests, AlignUp) {
    // Basic alignment tests
    EXPECT_EQ(align_up(1, 4), 4);
    EXPECT_EQ(align_up(4, 4), 4);
    EXPECT_EQ(align_up(5, 8), 8);
    EXPECT_EQ(align_up(7, 8), 8);
    EXPECT_EQ(align_up(8, 8), 8);

    // Edge case: zero
    EXPECT_EQ(align_up(0, 4), 0);
    EXPECT_EQ(align_up(0, 16), 0);

    // Power of 2 alignments
    EXPECT_EQ(align_up(1, 1), 1);
    EXPECT_EQ(align_up(1, 2), 2);
    EXPECT_EQ(align_up(3, 2), 4);
    EXPECT_EQ(align_up(15, 16), 16);
    EXPECT_EQ(align_up(16, 16), 16);
    EXPECT_EQ(align_up(17, 16), 32);
    EXPECT_EQ(align_up(31, 32), 32);
    EXPECT_EQ(align_up(33, 32), 64);

    // Large values
    EXPECT_EQ(align_up(1000, 256), 1024);
    EXPECT_EQ(align_up(1024, 256), 1024);
    EXPECT_EQ(align_up(1025, 256), 1280);

    // Different integral types
    EXPECT_EQ(align_up(static_cast<uint32_t>(5), 4), 8u);
    EXPECT_EQ(align_up(static_cast<uint64_t>(9), 8), 16ull);
    EXPECT_EQ(align_up(static_cast<size_t>(3), 4), static_cast<size_t>(4));
}

class DeletionQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue = DeletionQueue{};
        execution_order.clear();
    }

    DeletionQueue queue;
    std::vector<int> execution_order;
};

TEST_F(DeletionQueueTest, BasicFunctionality) {
    // Add functions in order 1, 2, 3
    queue.push_function([&]() { execution_order.push_back(1); });
    queue.push_function([&]() { execution_order.push_back(2); });
    queue.push_function([&]() { execution_order.push_back(3); });

    // Should execute in reverse order: 3, 2, 1
    queue.flush();

    std::vector<int> expected = {3, 2, 1};
    EXPECT_EQ(execution_order, expected);
}

TEST_F(DeletionQueueTest, EmptyFlush) {
    // Flushing empty queue should not crash
    EXPECT_NO_THROW(queue.flush());

    EXPECT_EQ(queue.deletors.size(), 0u);
}

TEST_F(DeletionQueueTest, MultipleFlushes) {
    // First batch
    queue.push_function([&]() { execution_order.push_back(1); });
    queue.push_function([&]() { execution_order.push_back(2); });
    queue.flush();

    // Second batch
    queue.push_function([&]() { execution_order.push_back(3); });
    queue.push_function([&]() { execution_order.push_back(4); });
    queue.flush();

    std::vector<int> expected = {2, 1, 4, 3};
    EXPECT_EQ(execution_order, expected);
}

TEST_F(DeletionQueueTest, ClearsAfterFlush) {
    queue.push_function([]() {});
    queue.push_function([]() {});
    queue.push_function([]() {});

    EXPECT_EQ(queue.deletors.size(), 3u);

    queue.flush();

    // Queue should be empty after flush
    EXPECT_EQ(queue.deletors.size(), 0u);
}

TEST_F(DeletionQueueTest, ExceptionSafety) {
    // Add functions
    queue.push_function([this]() { execution_order.push_back(1); });
    queue.push_function([this]() {
        execution_order.push_back(2);
        throw std::runtime_error("test exception");
    });

    EXPECT_THROW(queue.flush(), std::runtime_error);

    // Only the throwing function should have executed
    EXPECT_EQ(execution_order.size(), 1u);
    EXPECT_EQ(execution_order[0], 2);
}

TEST_F(DeletionQueueTest, SingleFunction) {
    bool executed = false;

    queue.push_function([&]() { executed = true; });
    queue.flush();

    EXPECT_TRUE(executed);
    EXPECT_EQ(queue.deletors.size(), 0u);
}

TEST_F(DeletionQueueTest, MoveSemantics) {
    // Test that move semantics work correctly
    auto lambda = [this]() { execution_order.push_back(42); };
    queue.push_function(std::move(lambda));

    queue.flush();

    EXPECT_EQ(execution_order.size(), 1u);
    EXPECT_EQ(execution_order[0], 42);
}