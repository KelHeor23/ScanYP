#include <gtest/gtest.h>
//#include <print>

#include "scan.hpp"

template <class Result>
decltype(auto) as_tuple(const Result& r) {
    return r.values();
}

TEST(Scan, ParsesSUFI) {
    using std::string;
    std::string_view input  = "name=alpha; id=42; temp=3.5; delta=-7";
    std::string_view format = "name={%s}; id={%u}; temp={%f}; delta={%d}";

    auto res = stdx::scan<string, unsigned, double, int>(input, format);
    ASSERT_TRUE(res.has_value());

    const auto& [name, id, temp, delta] = as_tuple(res.value());
    EXPECT_EQ(name, "alpha");
    EXPECT_EQ(id, 42u);
    EXPECT_DOUBLE_EQ(temp, 3.5);
    EXPECT_EQ(delta, -7);
}

TEST(Scan, ParsesSvI) {
    std::string_view input  = "key=xyz value=101";
    std::string_view format = "key={%s} value={%d}";

    auto res = stdx::scan<std::string_view, int>(input, format);
    ASSERT_TRUE(res.has_value());

    const auto& [key, val] = as_tuple(res.value());
    EXPECT_EQ(key, std::string_view("xyz"));
    EXPECT_EQ(val, 101);
}

TEST(Scan, ParsesMultipleSpacesAndSigns) {
    std::string_view input  = "a=   -12  b=+34  c=0";
    std::string_view format = "a={%d}  b={%d}  c={%d}";

    auto res = stdx::scan<int, int, int>(input, format);
    ASSERT_TRUE(res.has_value());

    const auto& [a, b, c] = as_tuple(res.value());
    EXPECT_EQ(a, -12);
    EXPECT_EQ(b, 34);
    EXPECT_EQ(c, 0);
}

TEST(Scan, ParsesEmptyFormat) {
    std::string_view input  = "a=   -12  b=+34  c=0";
    std::string_view format = "a={%d}  b={}  c={%d}";

    auto res = stdx::scan<int, int>(input, format);
    ASSERT_TRUE(res.has_value());

    const auto& [b, c] = as_tuple(res.value());
    EXPECT_EQ(b, -12);
    EXPECT_EQ(c, 0);
}

TEST(Scan, ParsesMultiplyEmptyFormat) {
    std::string_view input  = "a=   -12  b=+34  c=0";
    std::string_view format = "a={}  b={}  c={%d}";

    auto res = stdx::scan<int>(input, format);
    ASSERT_TRUE(res.has_value());

    const auto& [c] = as_tuple(res.value());
    EXPECT_EQ(c, 0);
}

TEST(Scan, FailsWhenPlaceholdersCountDiffersFromTypes) {
    std::string_view input  = "x=1 y=2";
    std::string_view format = "x=%d y=%d z=%d";

    auto res = stdx::scan<int, int>(input, format);
    EXPECT_FALSE(res.has_value());
}

TEST(Scan, FailsOnSpecifierTypeMismatch_SIntoI) {
    std::string_view input  = "val=foo";
    std::string_view format = "val=%d";

    auto res = stdx::scan<int>(input, format);
    EXPECT_FALSE(res.has_value());
}

TEST(Scan, FailsOnUnsignedWithNegativeInput) {
    std::string_view input  = "u=-3";
    std::string_view format = "u=%u";

    auto res = stdx::scan<unsigned>(input, format);
    EXPECT_FALSE(res.has_value());
}

TEST(Scan, FailsOnInvalidFloat) {
    std::string_view input  = "f=nanX";
    std::string_view format = "f=%f";

    auto res = stdx::scan<double>(input, format);
    EXPECT_FALSE(res.has_value());
}

TEST(Scan, FailsOnUnknownSpecifier) {
    std::string_view input  = "x=1";
    std::string_view format = "x=%q";

    auto res = stdx::scan<int>(input, format);
    EXPECT_FALSE(res.has_value());
}