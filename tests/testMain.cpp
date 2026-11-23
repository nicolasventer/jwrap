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

JW_JSON_OBJECT(City, name)

struct Resident
{
	std::string name;
	std::optional<City> city;
	std::vector<std::string> tags;
};

JW_JSON_OBJECT(Resident, name, city, tags)

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

JW_JSON_OBJECT(Task, status, note, colors)

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

JW_JSON_OBJECT(Circle, radius)
JW_JSON_OBJECT(Rect, w, h)

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

JW_JSON_OBJECT(KeyCircle, radius)
JW_JSON_OBJECT(KeyRect, w, h)

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

JW_JSON_OBJECT(Drawing, name, shape)

struct Lone
{
	int n;
};

JW_JSON_OBJECT(Lone, n)

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

struct TuplePoint
{
	int x;
	std::optional<int> y;
	std::optional<int> z;
};

JW_JSON_ARRAY(TuplePoint, x, y, z)

struct TailRequired
{
	std::optional<int> a;
	int b;
};

JW_JSON_ARRAY(TailRequired, a, b)

struct AllOptional
{
	std::optional<int> a;
	std::optional<int> b;
};

JW_JSON_ARRAY(AllOptional, a, b)

struct Placed
{
	std::string name;
	TuplePoint position;
};

JW_JSON_OBJECT(Placed, name, position)

struct Chain
{
	int n;
	std::vector<Chain> rest;
};

JW_JSON_ARRAY(Chain, n, rest)

struct SchemaPoint
{
	int x;
	int y;
};

namespace jw
{
	template <> struct Converter<SchemaPoint>
	{
		static Value encode(const SchemaPoint& point)
		{
			Value result = array();
			result.push(point.x).push(point.y);
			return result;
		}

		static SchemaPoint decode(const Value& value)
		{
			if (!value.is_array() || value.size() != 2) throw type_error("Expected JSON array [x, y]");

			return SchemaPoint{value.at<int>(0), value.at<int>(1)};
		}

		static Value schema() { return schema::array(schema::integer(), 2, 2); }
	};
}

struct SchemaMarker
{
	std::string name;
	SchemaPoint position;
};

JW_JSON_OBJECT(SchemaMarker, name, position)

struct BareSchema
{
	int n;
};

namespace jw
{
	template <> struct StructTraits<BareSchema>
	{
		static constexpr auto fields() { return std::make_tuple(std::make_pair("n", &BareSchema::n)); }
	};
}

struct Node
{
	std::string name;
	std::vector<Node> children;
};

JW_JSON_OBJECT(Node, name, children)

struct Loop
{
	std::vector<Loop> kids;
};

namespace jw
{
	template <> struct StructTraits<Loop>
	{
		static constexpr auto fields() { return std::make_tuple(std::make_pair("kids", &Loop::kids)); }
	};
}

struct Address
{
	std::string city;
	int zip;
};

JW_JSON_OBJECT(Address, city, zip)

struct LabeledAddress : Address
{
	std::string label;
};

JW_JSON_OBJECT_DERIVED(LabeledAddress, Address, label)

struct MarkedAddress : LabeledAddress
{
	std::string mark;
};

JW_JSON_OBJECT_DERIVED(MarkedAddress, LabeledAddress, mark)

struct PostalAddress : Address
{
};

JW_JSON_OBJECT_DERIVED(PostalAddress, Address)

struct Point
{
	int x;
	std::optional<int> y;
	std::optional<int> z;
};

JW_JSON_ARRAY(Point, x, y, z)

struct WeightedPoint : Point
{
	std::optional<int> w;
};

JW_JSON_ARRAY_DERIVED(WeightedPoint, Point, w)

struct LabeledPoint : Point
{
	std::string label;
};

JW_JSON_ARRAY_DERIVED(LabeledPoint, Point, label)

struct MarkedPoint : WeightedPoint
{
	std::string label;
};

JW_JSON_ARRAY_DERIVED(MarkedPoint, WeightedPoint, label)

struct SamePoint : Point
{
};

JW_JSON_ARRAY_DERIVED(SamePoint, Point)

struct Place : std::variant<Address, Point>
{
	using variant::variant;
};

JW_JSON_VARIANT_KEY(Place, address, point)

struct AddressKind : std::variant<Address, LabeledAddress>
{
	using variant::variant;
};

JW_JSON_VARIANT_TAG(AddressKind, type, address, labeled)

using AddressKindAlias = std::variant<Address, LabeledAddress>;

JW_JSON_VARIANT_TAG(AddressKindAlias, type, address, labeled)

struct Item;

struct Repeat
{
	std::vector<Item> items;
};

JW_JSON_OBJECT(Repeat, items)

struct Item : std::variant<std::string, Repeat>
{
	using variant::variant;
};

JW_JSON_VARIANT_KEY(Item, text, repeat)

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

TEST_CASE("nested structs registered with JW_JSON_OBJECT")
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

TEST_CASE("json schema for built-in and container types")
{
	CHECK(jw::schema::of<std::string>().get<std::string>("type") == "string");
	CHECK(jw::schema::of<bool>().get<std::string>("type") == "boolean");
	CHECK(jw::schema::of<int>().get<std::string>("type") == "integer");
	CHECK(jw::schema::of<double>().get<std::string>("type") == "number");
	CHECK(jw::schema::of<std::nullptr_t>().get<std::string>("type") == "null");
	CHECK(jw::schema::of<jw::Value>().get<std::string>("$schema") == "https://json-schema.org/draft/2020-12/schema");
	CHECK_FALSE(jw::schema::of<jw::Value>().has_key("type"));

	const auto text = jw::schema::of<const char*>();
	CHECK(text["anyOf"][0].get<std::string>("type") == "string");
	CHECK(text["anyOf"][1].get<std::string>("type") == "null");
	CHECK_FALSE(text["anyOf"][0].has_key("$schema"));

	const auto values = jw::schema::of<std::vector<std::optional<int>>>();
	CHECK(values.get<std::string>("type") == "array");
	CHECK(values["items"]["anyOf"][0].get<std::string>("type") == "integer");
	CHECK(values["items"]["anyOf"][1].get<std::string>("type") == "null");

	const auto scores = jw::schema::of<TwoInts>();
	CHECK(scores.get<int>("minItems") == 2);
	CHECK(scores.get<int>("maxItems") == 2);
	CHECK(scores["items"].get<std::string>("type") == "integer");

	const auto raw = jw::schema::of<int[3]>();
	CHECK(raw.get<int>("minItems") == 3);
	CHECK(raw.get<int>("maxItems") == 3);

	const auto metadata = jw::schema::of<StringMap>();
	CHECK(metadata.get<std::string>("type") == "object");
	CHECK(metadata["additionalProperties"].get<std::string>("type") == "integer");
	CHECK(jw::schema::of<StringPairs>()["additionalProperties"].get<std::string>("type") == "integer");
}

TEST_CASE("json schema for structs, enums, and variants")
{
	const auto resident = jw::schema::of<Resident>();
	CHECK(resident.get<std::string>("$schema") == "https://json-schema.org/draft/2020-12/schema");
	CHECK(resident.get<std::string>("title") == "Resident");
	CHECK(resident.get<std::string>("type") == "object");
	CHECK_FALSE(resident.has_key("additionalProperties"));
	CHECK(resident["required"].size() == 2);
	CHECK(resident["required"].at<std::string>(0) == "name");
	CHECK(resident["required"].at<std::string>(1) == "tags");
	CHECK(resident["properties"]["name"].get<std::string>("type") == "string");
	CHECK(resident["properties"]["tags"].get<std::string>("type") == "array");
	CHECK(resident["properties"]["tags"]["items"].get<std::string>("type") == "string");

	const auto city = resident["properties"]["city"];
	CHECK(city["anyOf"].size() == 2);
	CHECK(city["anyOf"][0].get<std::string>("title") == "City");
	CHECK(city["anyOf"][0]["properties"]["name"].get<std::string>("type") == "string");
	CHECK(city["anyOf"][0]["required"].at<std::string>(0) == "name");
	CHECK(city["anyOf"][1].get<std::string>("type") == "null");
	CHECK_FALSE(city.has_key("$schema"));

	const auto status = jw::schema::of<Status>();
	CHECK(status.get<std::string>("title") == "Status");
	CHECK(status.get<std::string>("type") == "string");
	CHECK(status["enum"].size() == 2);
	CHECK(status["enum"].at<std::string>(0) == "Ok");
	CHECK(status["enum"].at<std::string>(1) == "Error");

	const auto task = jw::schema::of<Task>();
	CHECK(task["properties"]["note"]["anyOf"][0]["enum"].at<std::string>(0) == "Ok");
	CHECK(task["properties"]["colors"]["items"]["enum"].at<std::string>(2) == "Blue");
	CHECK(task["required"].size() == 2);

	const auto shape = jw::schema::of<Shape>();
	CHECK(shape.get<std::string>("title") == "Shape");
	CHECK(shape["oneOf"].size() == 2);
	const auto circle = shape["oneOf"][0];
	CHECK(circle["properties"]["type"].get<std::string>("const") == "circle");
	CHECK(circle["properties"]["radius"].get<std::string>("type") == "number");
	CHECK(circle["required"].at<std::string>(0) == "radius");
	CHECK(circle["required"].at<std::string>(1) == "type");
	const auto rect = shape["oneOf"][1];
	CHECK(rect["properties"]["type"].get<std::string>("const") == "rect");
	CHECK(rect["required"].at<std::string>(2) == "type");

	const auto keyed = jw::schema::of<ShapeKey>();
	CHECK(keyed["oneOf"].size() == 2);
	CHECK(keyed["oneOf"][0].get<std::string>("type") == "object");
	CHECK(keyed["oneOf"][0]["required"].at<std::string>(0) == "circle");
	CHECK(keyed["oneOf"][0].get<bool>("additionalProperties") == false);
	CHECK(keyed["oneOf"][0]["properties"]["circle"].get<std::string>("title") == "KeyCircle");
	CHECK(keyed["oneOf"][1]["required"].at<std::string>(0) == "rect");

	const auto setting = jw::schema::of<Setting>();
	CHECK(setting["oneOf"][0]["properties"]["number"].get<std::string>("type") == "integer");
	CHECK(setting["oneOf"][1]["properties"]["text"].get<std::string>("type") == "string");
	CHECK(setting["oneOf"][1].get<bool>("additionalProperties") == false);

	CHECK_THROWS_AS(jw::schema::of<NumberOrCircle>(), jw::type_error);

	const auto bare = jw::schema::of<BareSchema>();
	CHECK_FALSE(bare.has_key("title"));
	CHECK(bare["properties"]["n"].get<std::string>("type") == "integer");
	CHECK(bare["required"].at<std::string>(0) == "n");
	CHECK(jw::schema::of<BareSchema>()["properties"].has_key("n"));
}

TEST_CASE("json schema for custom converters and recursive types")
{
	const auto point = jw::schema::of<SchemaPoint>();
	CHECK(point.get<std::string>("type") == "array");
	CHECK(point["items"].get<std::string>("type") == "integer");
	CHECK_FALSE(point["items"].has_key("$schema"));
	CHECK(point.get<int>("minItems") == 2);
	CHECK(point.get<int>("maxItems") == 2);

	const auto marker = jw::schema::of<SchemaMarker>();
	CHECK(marker.get<std::string>("title") == "SchemaMarker");
	CHECK(marker["properties"]["position"].get<int>("minItems") == 2);
	CHECK_FALSE(marker["properties"]["position"].has_key("$schema"));
	CHECK(marker["required"].at<std::string>(1) == "position");

	const auto node = jw::schema::of<Node>();
	CHECK(node.get<std::string>("title") == "Node");
	CHECK(node["properties"]["children"].get<std::string>("type") == "array");
	CHECK(node["properties"]["children"]["items"].get<std::string>("$ref") == "#/$defs/Node");
	CHECK(node["$defs"]["Node"].get<std::string>("title") == "Node");
	CHECK(node["$defs"]["Node"]["properties"]["children"]["items"].get<std::string>("$ref") == "#/$defs/Node");
	CHECK(node["required"].at<std::string>(0) == "name");
	CHECK(node["required"].at<std::string>(1) == "children");

	const auto again = jw::schema::of<Node>();
	CHECK(again["properties"].has_key("name"));
	CHECK(again.has_key("$defs"));

	CHECK_THROWS_AS(jw::schema::of<Loop>(), jw::type_error);
	CHECK(jw::schema::of<City>().get<std::string>("title") == "City");
}

TEST_CASE("json schema helpers")
{
	CHECK(jw::schema::string().get<std::string>("type") == "string");
	CHECK(jw::schema::boolean().get<std::string>("type") == "boolean");
	CHECK(jw::schema::integer().get<std::string>("type") == "integer");
	CHECK(jw::schema::number().get<std::string>("type") == "number");
	CHECK(jw::schema::null().get<std::string>("type") == "null");
	CHECK_FALSE(jw::schema::any().has_key("type"));

	const auto open = jw::schema::array(jw::schema::string());
	CHECK(open.get<std::string>("type") == "array");
	CHECK(open["items"].get<std::string>("type") == "string");
	CHECK_FALSE(open.has_key("minItems"));
	CHECK_FALSE(open.has_key("maxItems"));

	const auto minimum = jw::schema::array(jw::schema::string(), 1);
	CHECK(minimum.get<int>("minItems") == 1);
	CHECK_FALSE(minimum.has_key("maxItems"));

	const auto bounded = jw::schema::array(jw::schema::integer(), 2, 4);
	CHECK(bounded.get<int>("minItems") == 2);
	CHECK(bounded.get<int>("maxItems") == 4);

	const auto maximum = jw::schema::array(jw::schema::string(), std::nullopt, 4);
	CHECK_FALSE(maximum.has_key("minItems"));
	CHECK(maximum.get<int>("maxItems") == 4);

	const auto object = jw::schema::object({
		jw::schema::required("name", jw::schema::string()),
		jw::schema::optional("nickname", jw::schema::nullable(jw::schema::string())),
	});
	CHECK(object["properties"].has_key("name"));
	CHECK(object["properties"].has_key("nickname"));
	CHECK(object["required"].size() == 1);
	CHECK(object["required"].at<std::string>(0) == "name");
	CHECK(object["properties"]["nickname"]["anyOf"][0].get<std::string>("type") == "string");
	CHECK(object["properties"]["nickname"]["anyOf"][1].get<std::string>("type") == "null");

	const auto absent = jw::schema::object({jw::schema::optional("nickname", jw::schema::string())});
	CHECK_FALSE(absent.has_key("required"));

	CHECK(jw::schema::map(jw::schema::integer())["additionalProperties"].get<std::string>("type") == "integer");

	const auto names = jw::schema::enumeration({"Admin", "Member"});
	CHECK(names.get<std::string>("type") == "string");
	CHECK(names["enum"].at<std::string>(0) == "Admin");
	CHECK(names["enum"].at<std::string>(1) == "Member");

	CHECK(jw::schema::any_of({jw::schema::string(), jw::schema::integer()})["anyOf"].size() == 2);
	CHECK(jw::schema::one_of({jw::schema::string(), jw::schema::integer()})["oneOf"].size() == 2);
	CHECK(jw::schema::all_of({jw::schema::string(), jw::schema::integer()})["allOf"].size() == 2);
	CHECK(jw::schema::nullable(jw::schema::integer())["anyOf"][1].get<std::string>("type") == "null");

	CHECK(jw::schema::ref("Node").get<std::string>("$ref") == "#/$defs/Node");
	CHECK(jw::schema::ref("Node", nullptr).get<std::string>("$ref") == "#/$defs/Node");
	CHECK(jw::schema::ref("Node", "").get<std::string>("$ref") == "#/$defs/Node");
	CHECK(jw::schema::ref("a/b").get<std::string>("$ref") == "#/$defs/a~1b");
	CHECK(jw::schema::ref("a~b").get<std::string>("$ref") == "#/$defs/a~0b");
	CHECK(jw::schema::ref("City", "city.json").get<std::string>("$ref") == "city.json#/$defs/City");
	CHECK(jw::schema::ref("a/b", "https://example.com/schema.json").get<std::string>("$ref")
		== "https://example.com/schema.json#/$defs/a~1b");
	CHECK(jw::schema::ref("a~b", "dir/tree.json").get<std::string>("$ref") == "dir/tree.json#/$defs/a~0b");

	const auto unchanged = jw::schema::defs(jw::schema::string(), {});
	CHECK_FALSE(unchanged.has_key("$defs"));

	const auto tree = jw::schema::defs(jw::schema::array(jw::schema::ref("Node")),
		{
			{"Node", jw::schema::string()},
			{"Leaf", jw::schema::integer()},
		});
	CHECK(tree["items"].get<std::string>("$ref") == "#/$defs/Node");
	CHECK(tree["$defs"]["Node"].get<std::string>("type") == "string");
	CHECK(tree["$defs"]["Leaf"].get<std::string>("type") == "integer");
	const auto defs_text = tree["$defs"].dump();
	CHECK(defs_text.find("\"Node\"") < defs_text.find("\"Leaf\""));

	CHECK_THROWS_AS(jw::schema::defs(jw::array(), {{"A", jw::schema::string()}}), jw::type_error);
}

TEST_CASE("tuples registered with JW_JSON_ARRAY")
{
	const TuplePoint only_x{1, std::nullopt, std::nullopt};
	CHECK(jw::to_value(only_x).dump() == "[1]");
	const TuplePoint read_x = jw::from_json<TuplePoint>(jw::parse("[1]"));
	CHECK(read_x.x == 1);
	CHECK_FALSE(read_x.y.has_value());
	CHECK_FALSE(read_x.z.has_value());

	const TuplePoint xy{1, 2, std::nullopt};
	CHECK(jw::to_value(xy).dump() == "[1,2]");
	const TuplePoint read_xy = jw::from_json<TuplePoint>(jw::parse("[1,2]"));
	CHECK(read_xy.x == 1);
	REQUIRE(read_xy.y.has_value());
	CHECK(*read_xy.y == 2);
	CHECK_FALSE(read_xy.z.has_value());

	const TuplePoint full{1, 2, 3};
	CHECK(jw::to_value(full).dump() == "[1,2,3]");
	const TuplePoint read_full = jw::from_json<TuplePoint>(jw::parse("[1,2,3]"));
	CHECK(read_full.x == 1);
	REQUIRE(read_full.y.has_value());
	CHECK(*read_full.y == 2);
	REQUIRE(read_full.z.has_value());
	CHECK(*read_full.z == 3);

	const TuplePoint hole{1, std::nullopt, 3};
	CHECK(jw::to_value(hole).dump() == "[1,null,3]");
	const TuplePoint read_hole = jw::from_json<TuplePoint>(jw::parse("[1,null,3]"));
	CHECK(read_hole.x == 1);
	CHECK_FALSE(read_hole.y.has_value());
	REQUIRE(read_hole.z.has_value());
	CHECK(*read_hole.z == 3);

	const TailRequired tail{std::nullopt, 5};
	CHECK(jw::to_value(tail).dump() == "[null,5]");
	const TailRequired read_tail = jw::from_json<TailRequired>(jw::parse("[null,5]"));
	CHECK_FALSE(read_tail.a.has_value());
	CHECK(read_tail.b == 5);
	CHECK(jw::schema::of<TailRequired>().get<int>("minItems") == 2);
	CHECK_THROWS_AS(jw::parse("[5]").get<TailRequired>(), jw::type_error);

	const AllOptional empty{std::nullopt, std::nullopt};
	CHECK(jw::to_value(empty).dump() == "[]");
	const AllOptional read_empty = jw::from_json<AllOptional>(jw::parse("[]"));
	CHECK_FALSE(read_empty.a.has_value());
	CHECK_FALSE(read_empty.b.has_value());
	const AllOptional hole_then_value{std::nullopt, 2};
	CHECK(jw::to_value(hole_then_value).dump() == "[null,2]");
	CHECK(jw::schema::of<AllOptional>().get<int>("minItems") == 0);
	CHECK(jw::schema::of<AllOptional>().get<int>("maxItems") == 2);

	CHECK_THROWS_AS(jw::parse("[]").get<TuplePoint>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse("[1,2,3,4]").get<TuplePoint>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse("{}").get<TuplePoint>(), jw::type_error);
	CHECK_THROWS_AS(jw::parse("[null]").get<TuplePoint>(), jw::type_error);

	const auto schema = jw::schema::of<TuplePoint>();
	CHECK(schema.get<std::string>("title") == "TuplePoint");
	CHECK(schema.get<std::string>("type") == "array");
	CHECK(schema["prefixItems"].size() == 3);
	CHECK(schema["prefixItems"][0].get<std::string>("description") == "x");
	CHECK(schema["prefixItems"][0].get<std::string>("type") == "integer");
	CHECK(schema["prefixItems"][1].get<std::string>("description") == "y");
	CHECK(schema["prefixItems"][1]["anyOf"][0].get<std::string>("type") == "integer");
	CHECK(schema["prefixItems"][1]["anyOf"][1].get<std::string>("type") == "null");
	CHECK(schema["prefixItems"][2].get<std::string>("description") == "z");
	CHECK(schema["prefixItems"][2]["anyOf"][0].get<std::string>("type") == "integer");
	CHECK(schema.get<bool>("items") == false);
	CHECK(schema.get<int>("minItems") == 1);
	CHECK(schema.get<int>("maxItems") == 3);
	CHECK_FALSE(schema["prefixItems"][0].has_key("$schema"));

	const Placed placed{"origin", {1, 2, std::nullopt}};
	const auto placed_value = jw::to_value(placed);
	CHECK(placed_value.get<std::string>("name") == "origin");
	CHECK(placed_value["position"].dump() == "[1,2]");
	const Placed placed_copy = jw::from_json<Placed>(placed_value);
	CHECK(placed_copy.name == "origin");
	CHECK(placed_copy.position.x == 1);
	REQUIRE(placed_copy.position.y.has_value());
	CHECK(*placed_copy.position.y == 2);
	CHECK_FALSE(placed_copy.position.z.has_value());

	const auto placed_schema = jw::schema::of<Placed>();
	CHECK(placed_schema["properties"]["position"].get<std::string>("title") == "TuplePoint");
	CHECK(placed_schema["properties"]["position"]["prefixItems"][0].get<std::string>("description") == "x");
	CHECK(placed_schema["required"].at<std::string>(1) == "position");

	const auto chain = jw::schema::of<Chain>();
	CHECK(chain.get<std::string>("title") == "Chain");
	CHECK(chain["prefixItems"][1].get<std::string>("description") == "rest");
	CHECK(chain["prefixItems"][1]["items"].get<std::string>("$ref") == "#/$defs/Chain");
	CHECK(chain["$defs"]["Chain"].get<std::string>("title") == "Chain");
	CHECK(chain["$defs"]["Chain"]["prefixItems"][1]["items"].get<std::string>("$ref") == "#/$defs/Chain");
	CHECK(chain.get<int>("minItems") == 2);
	CHECK(chain.get<int>("maxItems") == 2);
}

TEST_CASE("objects registered with JW_JSON_OBJECT_DERIVED reuse base fields")
{
	const LabeledAddress original{{"Singapore", 123456}, "home"};
	CHECK(jw::to_value(original).dump() == R"({"city":"Singapore","zip":123456,"label":"home"})");

	const LabeledAddress copy = jw::from_json<LabeledAddress>(jw::to_value(original));
	CHECK(copy.city == "Singapore");
	CHECK(copy.zip == 123456);
	CHECK(copy.label == "home");

	const Address base{"Singapore", 123456};
	CHECK(jw::to_value(base).dump() == R"({"city":"Singapore","zip":123456})");
	const Address base_copy = jw::from_json<Address>(jw::parse(R"({"city":"Singapore","zip":123456})"));
	CHECK(base_copy.city == "Singapore");
	CHECK(base_copy.zip == 123456);

	const MarkedAddress marked{{{"Singapore", 123456}, "home"}, "pin"};
	CHECK(jw::to_value(marked).dump() == R"({"city":"Singapore","zip":123456,"label":"home","mark":"pin"})");
	const MarkedAddress marked_copy = jw::from_json<MarkedAddress>(jw::to_value(marked));
	CHECK(marked_copy.city == "Singapore");
	CHECK(marked_copy.label == "home");
	CHECK(marked_copy.mark == "pin");

	const PostalAddress postal{{"Singapore", 123456}};
	CHECK(jw::to_value(postal).dump() == jw::to_value(base).dump());
	const PostalAddress postal_copy = jw::from_json<PostalAddress>(jw::parse(R"({"city":"Singapore","zip":123456})"));
	CHECK(postal_copy.city == "Singapore");
	CHECK(postal_copy.zip == 123456);

	CHECK_THROWS_AS(jw::parse(R"({"zip":123456,"label":"home"})").get<LabeledAddress>(), jw::missing_key);

	const auto schema = jw::schema::of<LabeledAddress>();
	CHECK(schema.get<std::string>("title") == "LabeledAddress");
	const std::string properties = schema["properties"].dump();
	CHECK(properties.find("\"city\"") < properties.find("\"zip\""));
	CHECK(properties.find("\"zip\"") < properties.find("\"label\""));
	CHECK(schema["required"].size() == 3);
	CHECK(schema["required"].at<std::string>(0) == "city");
	CHECK(schema["required"].at<std::string>(1) == "zip");
	CHECK(schema["required"].at<std::string>(2) == "label");
	CHECK(jw::schema::of<Address>().get<std::string>("title") == "Address");
	CHECK(jw::schema::of<PostalAddress>().get<std::string>("title") == "PostalAddress");
}

TEST_CASE("arrays registered with JW_JSON_ARRAY_DERIVED reuse base positions")
{
	const WeightedPoint only_x{{1, std::nullopt, std::nullopt}, std::nullopt};
	const WeightedPoint with_y{{1, 2, std::nullopt}, std::nullopt};
	const WeightedPoint with_z{{1, std::nullopt, 3}, std::nullopt};
	const WeightedPoint with_w{{1, std::nullopt, std::nullopt}, 4};

	CHECK(jw::to_value(only_x).dump() == "[1]");
	CHECK(jw::to_value(with_y).dump() == "[1,2]");
	CHECK(jw::to_value(with_z).dump() == "[1,null,3]");
	CHECK(jw::to_value(with_w).dump() == "[1,null,null,4]");

	const WeightedPoint copy = jw::from_json<WeightedPoint>(jw::parse("[1,null,null,4]"));
	CHECK(copy.x == 1);
	CHECK_FALSE(copy.y.has_value());
	CHECK_FALSE(copy.z.has_value());
	REQUIRE(copy.w.has_value());
	CHECK(*copy.w == 4);

	const auto weighted = jw::schema::of<WeightedPoint>();
	CHECK(weighted.get<std::string>("title") == "WeightedPoint");
	CHECK(weighted["prefixItems"][0].get<std::string>("description") == "x");
	CHECK(weighted["prefixItems"][1].get<std::string>("description") == "y");
	CHECK(weighted["prefixItems"][2].get<std::string>("description") == "z");
	CHECK(weighted["prefixItems"][3].get<std::string>("description") == "w");
	CHECK(weighted.get<int>("minItems") == 1);
	CHECK(weighted.get<int>("maxItems") == 4);

	const LabeledPoint labeled{{1, std::nullopt, std::nullopt}, "home"};
	CHECK(jw::to_value(labeled).dump() == R"([1,null,null,"home"])");
	CHECK(jw::from_json<LabeledPoint>(jw::parse(R"([1,null,null,"home"])")).label == "home");
	CHECK_THROWS_AS(jw::parse("[1,null,null]").get<LabeledPoint>(), jw::type_error);

	const auto labeled_schema = jw::schema::of<LabeledPoint>();
	CHECK(labeled_schema.get<std::string>("title") == "LabeledPoint");
	CHECK(labeled_schema["prefixItems"][3].get<std::string>("description") == "label");
	CHECK(labeled_schema.get<int>("minItems") == 4);
	CHECK(labeled_schema.get<int>("maxItems") == 4);

	const MarkedPoint marked{{{1, std::nullopt, std::nullopt}, 4}, "home"};
	CHECK(jw::to_value(marked).dump() == R"([1,null,null,4,"home"])");
	const MarkedPoint marked_copy = jw::from_json<MarkedPoint>(jw::to_value(marked));
	CHECK(marked_copy.x == 1);
	REQUIRE(marked_copy.w.has_value());
	CHECK(*marked_copy.w == 4);
	CHECK(marked_copy.label == "home");
	CHECK_THROWS_AS(jw::parse("[1,null,null,4]").get<MarkedPoint>(), jw::type_error);

	const auto marked_schema = jw::schema::of<MarkedPoint>();
	CHECK(marked_schema["prefixItems"][0].get<std::string>("description") == "x");
	CHECK(marked_schema["prefixItems"][3].get<std::string>("description") == "w");
	CHECK(marked_schema["prefixItems"][4].get<std::string>("description") == "label");
	CHECK(marked_schema.get<int>("minItems") == 5);
	CHECK(marked_schema.get<int>("maxItems") == 5);

	const Point point{1, 2, std::nullopt};
	const SamePoint same{{1, 2, std::nullopt}};
	CHECK(jw::to_value(same).dump() == jw::to_value(point).dump());
	CHECK(jw::from_json<SamePoint>(jw::parse("[1]")).x == 1);
	CHECK(jw::schema::of<SamePoint>().get<std::string>("title") == "SamePoint");
	CHECK(jw::schema::of<SamePoint>().get<int>("minItems") == 1);
	CHECK(jw::schema::of<SamePoint>().get<int>("maxItems") == 3);
}

TEST_CASE("variants that inherit std::variant read and write through the base")
{
	const Place address{Address{"Singapore", 123456}};
	CHECK(jw::to_value(address).dump() == R"({"address":{"city":"Singapore","zip":123456}})");
	const Place address_copy = jw::from_json<Place>(jw::to_value(address));
	REQUIRE(std::holds_alternative<Address>(address_copy));
	CHECK(std::get<Address>(address_copy).city == "Singapore");
	CHECK(std::get<Address>(address_copy).zip == 123456);

	const Place point{Point{1, 2, std::nullopt}};
	CHECK(jw::to_value(point).dump() == R"({"point":[1,2]})");
	const Place point_copy = jw::from_json<Place>(jw::to_value(point));
	REQUIRE(std::holds_alternative<Point>(point_copy));
	CHECK(std::get<Point>(point_copy).x == 1);
	REQUIRE(std::get<Point>(point_copy).y.has_value());
	CHECK(*std::get<Point>(point_copy).y == 2);
	CHECK_FALSE(std::get<Point>(point_copy).z.has_value());

	const AddressKind labeled{LabeledAddress{{"Singapore", 123456}, "home"}};
	const AddressKindAlias labeled_alias{LabeledAddress{{"Singapore", 123456}, "home"}};
	CHECK(jw::to_value(labeled).dump() == jw::to_value(labeled_alias).dump());
	CHECK(jw::to_value(labeled).dump() == R"({"city":"Singapore","zip":123456,"label":"home","type":"labeled"})");
	const AddressKind labeled_copy = jw::from_json<AddressKind>(jw::to_value(labeled));
	REQUIRE(std::holds_alternative<LabeledAddress>(labeled_copy));
	CHECK(std::get<LabeledAddress>(labeled_copy).label == "home");

	const AddressKind plain{Address{"Singapore", 123456}};
	const AddressKindAlias plain_alias{Address{"Singapore", 123456}};
	CHECK(jw::to_value(plain).dump() == jw::to_value(plain_alias).dump());
	CHECK(jw::to_value(plain).get<std::string>("type") == "address");

	const auto place_schema = jw::schema::of<Place>();
	CHECK(place_schema.get<std::string>("title") == "Place");
	CHECK(place_schema["oneOf"].size() == 2);

	Repeat nested;
	nested.items.push_back(Item{std::string{"a"}});
	const Item loop{nested};
	const auto loop_value = jw::to_value(loop);
	CHECK(loop_value["repeat"]["items"][0].get<std::string>("text") == "a");
	const Item loop_copy = jw::from_json<Item>(loop_value);
	REQUIRE(std::holds_alternative<Repeat>(loop_copy));
	REQUIRE(std::get<Repeat>(loop_copy).items.size() == 1);
	REQUIRE(std::holds_alternative<std::string>(std::get<Repeat>(loop_copy).items[0]));
	CHECK(std::get<std::string>(std::get<Repeat>(loop_copy).items[0]) == "a");

	const auto item_schema = jw::schema::of<Item>();
	CHECK(item_schema.get<std::string>("title") == "Item");
	CHECK(item_schema["oneOf"][1]["properties"]["repeat"]["properties"]["items"]["items"].get<std::string>("$ref")
		== "#/$defs/Item");
	CHECK(item_schema["$defs"]["Item"].get<std::string>("title") == "Item");
}

TEST_CASE("yyjson pointers refer to the owned mutable document")
{
	const auto root = jw::object().set("a", 1);
	CHECK(root.mut_val() != nullptr);
	CHECK(root.mut_doc() != nullptr);
	CHECK(yyjson_mut_is_obj(root.mut_val()));
	CHECK(yyjson_mut_doc_get_root(root.mut_doc()) == root.mut_val());
}
