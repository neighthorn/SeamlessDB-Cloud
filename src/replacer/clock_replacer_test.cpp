
#include "replacer/clock_replacer.h"

#include <cstdio>
#include <thread>  // NOLINT
#include <vector>

#include "gtest/gtest.h"

TEST(ClockReplacerTest, SampleTest) {
    ClockReplacer clock_replacer(7);

    // Scenario: unpin six elements, i.e. add them to the replacer.
    clock_replacer.unpin(1);
    clock_replacer.unpin(2);
    clock_replacer.unpin(3);
    clock_replacer.unpin(4);
    clock_replacer.unpin(5);
    clock_replacer.unpin(6);
    clock_replacer.unpin(1);
    EXPECT_EQ(6, clock_replacer.Size());

    // Scenario: get three victims from the clock.
    int value;
    clock_replacer.victim(&value);
    EXPECT_EQ(1, value);
    clock_replacer.victim(&value);
    EXPECT_EQ(2, value);
    clock_replacer.victim(&value);
    EXPECT_EQ(3, value);

    // Scenario: pin elements in the replacer.
    // Note that 3 has already been victimized, so pinning 3 should have no effect.
    clock_replacer.pin(3);
    clock_replacer.pin(4);
    EXPECT_EQ(2, clock_replacer.Size());

    // Scenario: unpin 4. We expect that the reference bit of 4 will be set to 1.
    clock_replacer.unpin(4);

    // Scenario: continue looking for victims. We expect these victims.
    clock_replacer.victim(&value);
    EXPECT_EQ(5, value);
    clock_replacer.victim(&value);
    EXPECT_EQ(6, value);
    clock_replacer.victim(&value);
    EXPECT_EQ(4, value);
}

TEST(ClockReplacerTest, CornerCaseTest) {
    ClockReplacer clock_replacer(4);
    int value;
    bool result = clock_replacer.victim(&value);
    EXPECT_FALSE(result);

    clock_replacer.unpin(3);
    clock_replacer.unpin(2);
    EXPECT_EQ(2, clock_replacer.Size());
    clock_replacer.victim(&value);
    EXPECT_EQ(2, value);
    clock_replacer.unpin(1);
    EXPECT_EQ(2, clock_replacer.Size());
    clock_replacer.victim(&value);
    EXPECT_EQ(3, value);
    clock_replacer.victim(&value);
    EXPECT_EQ(1, value);
    EXPECT_FALSE(clock_replacer.victim(&value));
    EXPECT_EQ(0, clock_replacer.Size());
}
