#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
}

TEST_CASE("HtmlDecode decodes basic entities correctly", "[HtmlDecode]") {
    REQUIRE(HtmlDecode("Johnson&amp;Johnson") == "Johnson&Johnson");
    REQUIRE(HtmlDecode("M&amp;M&apos;s") == "M&M's");
    REQUIRE(HtmlDecode("&lt;div&gt;Hello &quot;world&quot;&lt;/div&gt;") == "<div>Hello \"world\"</div>");
}

TEST_CASE("HtmlDecode handles entities without semicolons", "[HtmlDecode]") {
    REQUIRE(HtmlDecode("Johnson&ampJohnson") == "Johnson&Johnson");
    REQUIRE(HtmlDecode("M&ampM&apos;s") == "M&M's");
    REQUIRE(HtmlDecode("&ltdiv&gtHello &quotworld&quot&lt/div&gt") == "<div>Hello \"world\"</div>");
}

TEST_CASE("HtmlDecode handles mixed cases correctly", "[HtmlDecode]") {
    REQUIRE(HtmlDecode("Johnson&AMP;Johnson") == "Johnson&Johnson");
    REQUIRE(HtmlDecode("M&AMP&M&aposS") == "M&M's");
    REQUIRE(HtmlDecode("&LTdiv&gtHello &QUOTworld&quot&lt/DIV&gt") == "<div>Hello \"world\"<div>");
}

TEST_CASE("HtmlDecode leaves unknown entities unchanged", "[HtmlDecode]") {
    REQUIRE(HtmlDecode("Johnson&unknown;Johnson") == "Johnson&unknown;Johnson");
    REQUIRE(HtmlDecode("M&abracadabra&M&apos;s") == "M&abracadabra&M's");
    REQUIRE(HtmlDecode("&lt;div&gt;Hello &unknown;&lt;/div&gt;") == "<div>Hello &unknown;<div>");
}

TEST_CASE("HtmlDecode does not double decode entities", "[HtmlDecode]") {
    REQUIRE(HtmlDecode("&amp;lt;div&amp;gt;") == "&lt;div&gt;");
    REQUIRE(HtmlDecode("&amp;amp;") == "&amp;");
    REQUIRE(HtmlDecode("&amp;quot;Hello&amp;quot;") == "&quot;Hello&quot;");
}

// Напишите недостающие тесты самостоятельно
