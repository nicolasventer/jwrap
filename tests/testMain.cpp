#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../jwrap/jwrap.hpp"
#include "../jwrap/yyjson/yyjson.c"

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct City {
    std::string name;
};

JW_JSON_FIELDS(City, name)

struct Resident {
    std::string name;
    std::optional<City> city;
    std::vector<std::string> tags;
};

JW_JSON_FIELDS(Resident, name, city, tags)

using StringMap = std::map<std::string, int>;
using StringPairs = std::vector<std::pair<std::string, int>>;
using IntMap = std::map<int, int>;
using TwoInts = std::array<int, 2>;

TEST_CASE("parse errors and read flags") {
    CHECK_THROWS_AS(jw::parse("{"), jw::parse_error);

    CHECK_THROWS_AS(jw::parse(R"({ "a": 1, })"), jw::parse_error);
    CHECK(jw::parse(R"({ "a": 1, })", jw::read_flag::allow_trailing_commas).get<int>("a") == 1);

    CHECK_THROWS_AS(jw::parse(R"({ /*note*/ "a": 1 })"), jw::parse_error);
    CHECK(jw::parse(R"({ /*note*/ "a": 1 })", jw::read_flag::allow_comments).get<int>("a") == 1);

    const auto both = jw::parse(
        R"({ /*note*/ "a": 1, })", jw::read_flag::allow_comments_and_trailing_commas);
    CHECK(both.get<int>("a") == 1);
}

TEST_CASE("compact, 4-space, and 2-space output") {
    const auto value = jw::object().set("a", 1);
    const std::string pretty = "{\n    \"a\": 1\n}";
    const std::string two_spaces = "{\n  \"a\": 1\n}";

    CHECK(value.dump() == "{\"a\":1}");
    CHECK(value.dump(false) == "{\"a\":1}");
    CHECK(value.dump(true) == pretty);
    CHECK(value.dump(jw::write_flag::compact) == "{\"a\":1}");
    CHECK(value.dump(jw::write_flag::pretty) == pretty);
    CHECK(value.dump(jw::write_flag::pretty_two_spaces) == two_spaces);
    CHECK(jw::stringify(value, true) == pretty);
    CHECK(jw::stringify(value, jw::write_flag::pretty_two_spaces) == two_spaces);
}

TEST_CASE("primitive conversions, range checks, and type errors") {
    CHECK(jw::parse("true").get<bool>());
    CHECK(jw::parse("\"hi\"").get<std::string>() == "hi");
    CHECK(jw::parse("-5").get<int>() == -5);
    CHECK(jw::parse("1.5").get<double>() == doctest::Approx(1.5));
    CHECK(jw::to_value(std::string("ok")).get<std::string>() == "ok");

    const auto huge = jw::parse("18446744073709551615");
    CHECK(huge.get<uint64_t>() == UINT64_MAX);
    CHECK_THROWS_AS(huge.get<int64_t>(), std::out_of_range);
    CHECK_THROWS_AS(jw::parse("-1").get<unsigned>(), std::out_of_range);
    CHECK_THROWS_AS(jw::parse("1.5").get<int>(), jw::type_error);
    CHECK_THROWS_AS(jw::parse("\"hi\"").get<int>(), jw::type_error);
    CHECK_THROWS_AS(jw::to_value(std::numeric_limits<double>::quiet_NaN()), jw::error);
    CHECK_THROWS_AS(jw::to_value(std::numeric_limits<double>::infinity()), jw::error);
}

TEST_CASE("objects: replace, has_key, and operator[]") {
    auto root = jw::parse(R"({"address":{"city":"Boston"},"nums":[10,20]})");

    CHECK(root.has_key("address"));
    CHECK_FALSE(root.has_key("missing"));
    CHECK_FALSE(root["nums"].has_key("city"));
    CHECK(root["address"]["city"].get<std::string>() == "Boston");
    CHECK(root["nums"][1].get<int>() == 20);

    root.set("address", 7);
    CHECK(root.size() == 2);
    CHECK(root.get<int>("address") == 7);

    CHECK_THROWS_AS(root.get<int>("missing"), jw::missing_key);
    CHECK_THROWS_AS(root["missing"], jw::missing_key);
    CHECK_THROWS_AS(root["nums"][5], std::out_of_range);
    CHECK_THROWS_AS(jw::array()["city"], jw::error);
    CHECK_THROWS_AS(jw::object()[0], jw::error);
}

TEST_CASE("try_get distinguishes a missing key from JSON null") {
    const auto root = jw::parse(R"({"age":null,"name":"Ada"})");

    CHECK_FALSE(root.try_get<int>("missing").has_value());
    CHECK(root.try_get<std::string>("name").value() == "Ada");
    CHECK_THROWS_AS(root.try_get<int>("age"), jw::type_error);
    CHECK_FALSE(root.get<std::optional<int>>("age").has_value());
    CHECK_FALSE(root.get<std::optional<int>>("missing").has_value());
    CHECK_THROWS_AS(jw::array().try_get<int>("age"), jw::error);
}

TEST_CASE("arrays and vectors") {
    auto values = jw::array();
    values.push(1).push(2);
    CHECK(values.size() == 2);
    CHECK(values.at<int>(0) == 1);
    CHECK_THROWS_AS(values.at<int>(2), std::out_of_range);

    const std::vector<int> numbers{3, 4};
    const auto encoded = jw::to_value(numbers);
    CHECK(encoded.get<std::vector<int>>() == numbers);
    CHECK_THROWS_AS(jw::object().get<std::vector<int>>(), jw::type_error);
}

TEST_CASE("maps and vectors of pairs") {
    const StringMap ordered{{"b", 2}, {"a", 1}};
    const auto map_value = jw::to_value(ordered);
    CHECK(map_value.dump() == R"({"a":1,"b":2})");
    CHECK(jw::from_json<StringMap>(map_value) == ordered);

    const StringPairs pairs{{"b", 2}, {"a", 1}};
    const auto pair_value = jw::to_value(pairs);
    CHECK(pair_value.dump() == R"({"b":2,"a":1})");
    CHECK(jw::from_json<StringPairs>(pair_value) == pairs);
    CHECK_THROWS_AS(jw::to_value(IntMap{{1, 2}}), jw::type_error);
    CHECK_THROWS_AS(jw::array().get<StringMap>(), jw::type_error);
}

TEST_CASE("std::array and C arrays") {
    const TwoInts fixed{4, 5};
    const auto encoded = jw::to_value(fixed);
    CHECK(jw::from_json<TwoInts>(encoded) == fixed);
    CHECK_THROWS_AS(jw::parse("[1]").get<TwoInts>(), jw::type_error);

    const int raw[3] = {7, 8, 9};
    const auto array_value = jw::to_value(raw);
    int destination[3] = {};
    array_value.read_to(destination);
    CHECK(destination[0] == 7);
    CHECK(destination[1] == 8);
    CHECK(destination[2] == 9);

    int short_destination[2] = {};
    CHECK_THROWS_AS(array_value.read_to(short_destination), jw::type_error);
}

TEST_CASE("optional values are omitted from objects and become null in arrays") {
    auto root = jw::object();
    root.set("keep", std::optional<int>{7});
    root.set("drop", std::optional<int>{});
    CHECK(root.has_key("keep"));
    CHECK(root.get<int>("keep") == 7);
    CHECK_FALSE(root.has_key("drop"));

    root.set("keep", std::optional<int>{});
    CHECK_FALSE(root.has_key("keep"));

    auto values = jw::array();
    values.push(std::optional<int>{});
    values.push(std::optional<int>{4});
    CHECK(values[0].is_null());
    CHECK_FALSE(values.at<std::optional<int>>(0).has_value());
    CHECK(values.at<std::optional<int>>(1).value() == 4);
}

TEST_CASE("nested structs registered with JW_JSON_FIELDS") {
    const Resident original{"Ada", City{"London"}, {"math", "logic"}};
    const auto value = jw::to_value(original);
    const Resident copy = jw::from_json<Resident>(value);

    CHECK(copy.name == "Ada");
    REQUIRE(copy.city.has_value());
    CHECK(copy.city->name == "London");
    CHECK(copy.tags == std::vector<std::string>{"math", "logic"});

    const Resident without_city{"Bob", std::nullopt, {}};
    const std::string dumped = jw::to_value(without_city).dump();
    CHECK(dumped.find("city") == std::string::npos);

    const Resident read = jw::from_json<Resident>(jw::parse(dumped));
    CHECK(read.name == "Bob");
    CHECK_FALSE(read.city.has_value());
    CHECK(read.tags.empty());

    const auto null_city = jw::parse(R"({"name":"C","city":null,"tags":[]})");
    CHECK_FALSE(jw::from_json<Resident>(null_city).city.has_value());
    CHECK_THROWS_AS(jw::parse(R"({"tags":[]})").get<Resident>(), jw::missing_key);
}

TEST_CASE("yyjson pointers refer to the owned mutable document") {
    const auto root = jw::object().set("a", 1);
    CHECK(root.mut_val() != nullptr);
    CHECK(root.mut_doc() != nullptr);
    CHECK(yyjson_mut_is_obj(root.mut_val()));
    CHECK(yyjson_mut_doc_get_root(root.mut_doc()) == root.mut_val());
}
