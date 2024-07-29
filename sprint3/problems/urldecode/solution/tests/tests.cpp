#define BOOST_TEST_MODULE urlencode_tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode("") == "");
    
    // Тест строки без изменений
    BOOST_TEST(UrlDecode("simpletext") == "simpletext");

    // Тест строки с одним пробелом
    BOOST_TEST(UrlDecode("simple+text") == "simple text");

    // Тест строки с кодированным символом
    BOOST_TEST(UrlDecode("hello%20world") == "hello world");

    // Тест строки с несколькими кодированными символами
    BOOST_TEST(UrlDecode("%7B%7D%5B%5D") == "{}[]");

    // Тест строки с комбинацией кодированных и некодированных символов
    BOOST_TEST(UrlDecode("a%20b%20c") == "a b c");

    // Тест строки с некорректными кодированными символами (оставляем как есть)
    BOOST_TEST(UrlDecode("%ZZ") == "%ZZ");

    // Тест строки с частично некорректными кодированными символами
    BOOST_TEST(UrlDecode("text%2") == "text%2");

    // Тест строки с символами `+` и `%`
    BOOST_TEST(UrlDecode("+%2B") == " +");

    // Тест строки с символом `%` в конце строки
    BOOST_TEST(UrlDecode("text%") == "text%");
}