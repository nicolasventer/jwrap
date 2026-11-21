#include "jwrap/jwrap.hpp"
#include "jwrap/yyjson/yyjson.c"
#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct Address
{
	std::string city;
	int zip;
};

JW_JSON_OBJECT(Address, city, zip)

enum class Role
{
	Admin,
	Member,
};

JW_JSON_ENUM(Role, Admin, Member)

struct Point
{
	int x;
	std::optional<int> y;
	std::optional<int> z;
};

JW_JSON_ARRAY(Point, x, y, z)

struct User
{
	std::string name;
	int age;
	Role role;
	std::vector<std::string> tags;
	Address address;
	Point position;
};

JW_JSON_OBJECT(User, name, age, role, tags, address, position)

int main()
{
	User original{"Alice", 30, Role::Member, {"cpp", "yyjson"}, {"Singapore", 123456}, {1, 2, std::nullopt}};

	// C++ struct -> JSON
	auto value = jw::to_value(original);
	std::string text = jw::stringify(value, true);

	std::cout << text << "\n\n";

	// JSON -> C++ struct
	auto parsed = jw::parse(text);
	User copy = jw::from_json<User>(parsed);

	std::cout << copy.name << '\n';
	std::cout << copy.age << '\n';
	std::cout << (copy.role == Role::Admin ? "Admin" : "Member") << '\n';
	std::cout << copy.address.city << '\n';
	std::cout << copy.tags.at(0) << '\n';
	std::cout << copy.position.x << '\n';
	std::cout << *copy.position.y << '\n';
	std::cout << (copy.position.z.has_value() ? "z set" : "z empty") << '\n';
}
