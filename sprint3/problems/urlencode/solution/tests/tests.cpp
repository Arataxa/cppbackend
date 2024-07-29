#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
}

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
}

TEST(UrlEncodeTestSuite, SpaceIsEncoded) {
    EXPECT_EQ(UrlEncode("hello world"sv), "hello%20world"s);
}

TEST(UrlEncodeTestSuite, SpecialCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("!#$&'()*+,/:;=?@[]"sv), "%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"s);
}

TEST(UrlEncodeTestSuite, MixedChars) {
    EXPECT_EQ(UrlEncode("hello world!@$"sv), "hello%20world%21%40%24"s);
}

TEST(UrlEncodeTestSuite, UnreservedCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("abc-_.~123"sv), "abc-_.~123"s);
}

TEST(UrlEncodeTestSuite, PlusSignIsEncoded) {
    EXPECT_EQ(UrlEncode("hello+world"sv), "hello%2Bworld"s);
}

TEST(UrlEncodeTestSuite, NonAsciiChars) {
    EXPECT_EQ(UrlEncode("こんにちは"sv), "%E3%81%93%E3%82%93%E3%81%AB%E3%81%A1%E3%81%AF"s);
}

/* Напишите остальные тесты самостоятельно */
