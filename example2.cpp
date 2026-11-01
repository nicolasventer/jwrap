#include "jwrap/jwrap.hpp"
#include "jwrap/yyjson/yyjson.c"
#include <iostream>
#include <string>

struct Point
{
	int x;
	int y;
};

namespace jw
{
	template <> struct Converter<Point>
	{
		static Value encode(const Point& point)
		{
			Value result = array();
			result.push(point.x).push(point.y);
			return result;
		}

		static Point decode(const Value& value)
		{
			if (!value.is_array() || value.size() != 2) throw type_error("Expected JSON array [x, y]");

			return Point{value.at<int>(0), value.at<int>(1)};
		}
	};
}

struct Marker
{
	std::string name;
	Point position;
};

JW_JSON_FIELDS(Marker, name, position)

int main()
{
	Marker original{"origin", {1, 2}};

	// C++ struct -> JSON
	auto value = jw::to_value(original);
	std::string text = jw::stringify(value, true);

	std::cout << text << "\n\n";

	// JSON -> C++ struct
	auto parsed = jw::parse(text);
	Marker copy = jw::from_json<Marker>(parsed);

	std::cout << copy.name << '\n';
	std::cout << copy.position.x << '\n';
	std::cout << copy.position.y << '\n';
}
