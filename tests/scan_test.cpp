#include <gtest/gtest.h>
#include <print>

#include "scan.hpp"

TEST(ScanTest, SimpleTest) {
    auto result = stdx::scan<std::string>("number", "{}");
    ASSERT_FALSE(result);
}