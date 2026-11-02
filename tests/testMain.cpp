#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../jwrap/jwrap.hpp"
#include "../jwrap/yyjson/yyjson.c"
#include "../jwrap/yyjson/yyjson.h"

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct City
{
	std::string name;
};

JW_JSON_FIELDS(City, name)

struct Resident
{
	std::string name;
	std::optional<City> city;
	std::vector<std::string> tags;
};

JW_JSON_FIELDS(Resident, name, city, tags)

enum class Status
{
	Ok,
	Error,
	Hidden,
};

JW_JSON_ENUM(Status, Ok, Error)

enum Color
{
	Red,
	Green,
	Blue,
};

JW_JSON_ENUM(Color, Red, Green, Blue)

enum class Only
{
	Yes,
};

JW_JSON_ENUM(Only, Yes)

enum class Many
{
	E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15,
	E16, E17, E18, E19, E20, E21, E22, E23, E24, E25, E26, E27, E28, E29, E30, E31,
};

JW_JSON_ENUM(Many, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15, E16, E17, E18, E19, E20, E21,
	E22, E23, E24, E25, E26, E27, E28, E29, E30, E31)

struct Task
{
	Status status;
	std::optional<Status> note;
	std::vector<Color> colors;
};

JW_JSON_FIELDS(Task, status, note, colors)

using StringMap = std::map<std::string, int>;
using StringPairs = std::vector<std::pair<std::string, int>>;
using IntMap = std::map<int, int>;
using TwoInts = std::array<int, 2>;

struct Circle
{
	double radius;
};

struct Rect
{
	double w;
	double h;
};

JW_JSON_FIELDS(Circle, radius)
JW_JSON_FIELDS(Rect, w, h)

using Shape = std::variant<Circle, Rect>;

JW_JSON_VARIANT_TAG(Shape, type, circle, rect)

struct KeyCircle
{
	double radius;
};

struct KeyRect
{
	double w;
	double h;
};

JW_JSON_FIELDS(KeyCircle, radius)
JW_JSON_FIELDS(KeyRect, w, h)

using ShapeKey = std::variant<KeyCircle, KeyRect>;

JW_JSON_VARIANT_KEY(ShapeKey, circle, rect)

using Setting = std::variant<int, std::string>;

JW_JSON_VARIANT_KEY(Setting, number, text)

using NumberOrCircle = std::variant<int, Circle>;

JW_JSON_VARIANT_TAG(NumberOrCircle, type, number, circle)

struct Drawing
{
	std::string name;
	Shape shape;
};

JW_JSON_FIELDS(Drawing, name, shape)

struct Lone
{
	int n;
};

JW_JSON_FIELDS(Lone, n)

using LoneShape = std::variant<Lone>;

JW_JSON_VARIANT_TAG(LoneShape, kind, lone)

using Eight = std::variant<std::array<int, 1>,
	std::array<int, 2>,
	std::array<int, 3>,
	std::array<int, 4>,
	std::array<int, 5>,
	std::array<int, 6>,
	std::array<int, 7>,
	std::array<int, 8>>;

JW_JSON_VARIANT_KEY(Eight, a, b, c, d, e, f, g, h)

TEST_CASE("parse errors and read flags")
{
	CHECK_THROWS_AS(jw::parse("{"), jw::parse_error);

	CHECK_THROWS_AS(jw::parse(R"({ "a": 1, })"), jw::parse_error);
	CHECK(jw::parse(R"({ "a": 1, })", jw::e_read_flag::allow_trailing_commas).get<int>("a") == 1);

	CHECK_THROWS_AS(jw::parse(R"({ /*note*/ "a": 1 })"), jw::parse_error);
	CHECK(jw::parse(R"({ /*note*/ "a": 1 })", jw::e_read_flag::allow_comments).get<int>("a") == 1);

	const auto both = jw::parse(R"({ /*note*/ "a": 1, })", jw::e_read_flag::allow_comments_and_trailing_commas);
	CHECK(both.get<int>("a") == 1);
}

TEST_CASE("compact, 4-space, and 2-space output")
{
	const auto value = jw::object().set("a", 1);
	const std::string pretty = "{\n    \"a\": 1\n}";
	const std::string two_spaces = "{\n  \"a\": 1\n}";

	CHECK(value.dump() == "{\"a\":1}");
	CHECK(value.dump(false) == "{\"a\":1}");
	CHECK(value.dump(true) == pretty);
	CHECK(value.dump(jw::e_write_flag::compact) == "{\"a\":1}");
	CHECK(value.dump(jw::e_write_flag::pretty) == pretty);
	CHECK(value.dump(jw::e_write_flag::pretty_two_spaces) == two_spaces);
	CHECK(jw::stringify(value, true) == pretty);
	CHECK(jw::stringify(value, jw::e_write_flag::pretty_two_spaces) == two_spaces);
}

TEST_CASE("primitive conversions, range checks, and type errors")
{
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

TEST_CASE("objects: replace, has_key, and operator[]")
{
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

TEST_CASE("try_get distinguishes a missing key from JSON null")
{
	const auto root = jw::parse(R"({"age":null,"name":"Ada"})");

	CHECK_FALSE(root.try_get<int>("missing").has_value());
	CHECK(root.try_get<std::string>("name").value() == "Ada");
	CHECK_THROWS_AS(root.try_get<int>("age"), jw::type_error);
	CHECK_FALSE(root.get<std::optional<int>>("age").has_value());
	CHECK_FALSE(root.get<std::optional<int>>("missing").has_value());
	CHECK_THROWS_AS(jw::array().try_get<int>("age"), jw::error);
}

TEST_CASE("arrays and vectors")
{
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

TEST_CASE("maps and vectors of pairs")
{
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

TEST_CASE("std::array and C arrays")
{
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

TEST_CASE("optional values are omitted from objects and become null in arrays")
{
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

TEST_CASE("nested structs registered with JW_JSON_FIELDS")
{
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

TEST_CASE("enums registered with JW_JSON_ENUM read and write as strings")
{
	CHECK(jw::to_value(Status::Ok).dump() == "\"Ok\"");
	CHECK(jw::parse("\"Ok\"").get<Status>() == Status::Ok);
	CHECK(jw::to_value(Color::Green).dump() == "\"Green\"");
	CHECK(jw::parse("\"Blue\"").get<Color>() == Color::Blue);
	CHECK(jw::to_value(Only::Yes).dump() == "\"Yes\"");
	CHECK(jw::parse("\"E0\"").get<Many>() == Many::E0);
	CHECK(jw::parse("\"E31\"").get<Many>() == Many::E31);
	CHECK(jw::to_value(Many::E31).dump() == "\"E31\"");

	CHECK_THROWS_AS(jw::parse("1").get<Status>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse("\"Nope\"").get<Status>(), jw::type_error);
	CHECK_THROWS_AS(jw::to_value(Status::Hidden), jw::type_error);

	const Task original{Status::Error, Status::Ok, {Color::Red, Color::Blue}};
	const auto value = jw::to_value(original);
	CHECK(value.dump() == R"({"status":"Error","note":"Ok","colors":["Red","Blue"]})");

	const Task copy = jw::from_json<Task>(value);
	CHECK(copy.status == Status::Error);
	REQUIRE(copy.note.has_value());
	CHECK(*copy.note == Status::Ok);
	CHECK(copy.colors == std::vector<Color>{Color::Red, Color::Blue});

	const Task without_note{Status::Ok, std::nullopt, {}};
	const std::string dumped = jw::to_value(without_note).dump();
	CHECK(dumped == R"({"status":"Ok","colors":[]})");

	const Task read = jw::from_json<Task>(jw::parse(dumped));
	CHECK(read.status == Status::Ok);
	CHECK_FALSE(read.note.has_value());
	CHECK(read.colors.empty());

	const auto null_note = jw::parse(R"({"status":"Ok","note":null,"colors":[]})");
	CHECK_FALSE(jw::from_json<Task>(null_note).note.has_value());
}

TEST_CASE("variants registered with JW_JSON_VARIANT_TAG discriminate by field value")
{
	const Shape circle{Circle{0.5}};
	const auto circle_value = jw::to_value(circle);
	CHECK(circle_value.get<std::string>("type") == "circle");
	CHECK(circle_value.get<double>("radius") == doctest::Approx(0.5));

	const Shape circle_copy = jw::from_json<Shape>(circle_value);
	REQUIRE(std::holds_alternative<Circle>(circle_copy));
	CHECK(std::get<Circle>(circle_copy).radius == doctest::Approx(0.5));

	const Shape from_text = jw::from_json<Shape>(jw::parse(R"({"type":"circle","radius":0.5})"));
	REQUIRE(std::holds_alternative<Circle>(from_text));
	CHECK(std::get<Circle>(from_text).radius == doctest::Approx(0.5));

	const Shape rect = jw::from_json<Shape>(jw::parse(R"({"type":"rect","w":2,"h":3})"));
	REQUIRE(std::holds_alternative<Rect>(rect));
	CHECK(std::get<Rect>(rect).w == doctest::Approx(2));
	CHECK(std::get<Rect>(rect).h == doctest::Approx(3));

	const auto rect_value = jw::to_value(Shape{Rect{2, 3}});
	CHECK(rect_value.get<std::string>("type") == "rect");
	CHECK(rect_value.get<double>("w") == doctest::Approx(2));
	CHECK(rect_value.get<double>("h") == doctest::Approx(3));
	const Shape rect_copy = jw::from_json<Shape>(rect_value);
	REQUIRE(std::holds_alternative<Rect>(rect_copy));
	CHECK(std::get<Rect>(rect_copy).w == doctest::Approx(2));

	const Drawing original{"badge", Circle{0.5}};
	const Drawing drawing = jw::from_json<Drawing>(jw::to_value(original));
	CHECK(drawing.name == "badge");
	REQUIRE(std::holds_alternative<Circle>(drawing.shape));
	CHECK(std::get<Circle>(drawing.shape).radius == doctest::Approx(0.5));

	CHECK_THROWS_AS(jw::parse("[]").get<Shape>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse("{}").get<Shape>(), jw::missing_key);
	CHECK_THROWS_AS(jw::parse(R"({"type":1,"radius":0.5})").get<Shape>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse(R"({"type":"triangle","radius":0.5})").get<Shape>(), jw::type_error);
	CHECK_THROWS_AS(jw::to_value(NumberOrCircle{4}), jw::type_error);

	const NumberOrCircle tagged_circle{Circle{1.5}};
	const auto tagged = jw::to_value(tagged_circle);
	CHECK(tagged.get<std::string>("type") == "circle");
	CHECK(std::get<Circle>(jw::from_json<NumberOrCircle>(tagged)).radius == doctest::Approx(1.5));

	const LoneShape lone = jw::from_json<LoneShape>(jw::parse(R"({"kind":"lone","n":1})"));
	CHECK(std::get<Lone>(lone).n == 1);
	CHECK(jw::to_value(lone).get<std::string>("kind") == "lone");
}

TEST_CASE("variants registered with JW_JSON_VARIANT_KEY discriminate by key presence")
{
	const ShapeKey circle{KeyCircle{0.5}};
	const auto circle_value = jw::to_value(circle);
	CHECK(circle_value.dump() == R"({"circle":{"radius":0.5}})");
	const ShapeKey circle_copy = jw::from_json<ShapeKey>(circle_value);
	REQUIRE(std::holds_alternative<KeyCircle>(circle_copy));
	CHECK(std::get<KeyCircle>(circle_copy).radius == doctest::Approx(0.5));

	const auto rect_value = jw::to_value(ShapeKey{KeyRect{2, 3}});
	const ShapeKey rect_copy = jw::from_json<ShapeKey>(rect_value);
	REQUIRE(std::holds_alternative<KeyRect>(rect_copy));
	CHECK(std::get<KeyRect>(rect_copy).w == doctest::Approx(2));
	CHECK(std::get<KeyRect>(rect_copy).h == doctest::Approx(3));
	CHECK(rect_value.size() == 1);
	CHECK(rect_value["rect"].get<double>("w") == doctest::Approx(2));

	CHECK(jw::to_value(Setting{7}).dump() == R"({"number":7})");
	CHECK(jw::to_value(Setting{std::string{"hi"}}).dump() == R"({"text":"hi"})");
	CHECK(std::get<int>(jw::parse(R"({"number":7})").get<Setting>()) == 7);
	CHECK(std::get<std::string>(jw::parse(R"({"text":"hi"})").get<Setting>()) == "hi");

	CHECK_THROWS_AS(jw::parse("1").get<Setting>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse("{}").get<ShapeKey>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse(R"({"circle":{"radius":1},"rect":{"w":2,"h":3}})").get<ShapeKey>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse(R"({"nope":1})").get<Setting>(), jw::type_error);

	const Eight eight{std::array<int, 8>{1, 2, 3, 4, 5, 6, 7, 8}};
	const auto eight_value = jw::to_value(eight);
	CHECK(eight_value["h"].at<int>(7) == 8);
	CHECK(std::get<7>(jw::from_json<Eight>(eight_value))[7] == 8);
	CHECK(jw::to_value(Eight{std::array<int, 1>{9}}).dump() == R"({"a":[9]})");
}

TEST_CASE("yyjson pointers refer to the owned mutable document")
{
	const auto root = jw::object().set("a", 1);
	CHECK(root.mut_val() != nullptr);
	CHECK(root.mut_doc() != nullptr);
	CHECK(yyjson_mut_is_obj(root.mut_val()));
	CHECK(yyjson_mut_doc_get_root(root.mut_doc()) == root.mut_val());
}
