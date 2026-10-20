#include "jwrap/jwrap.hpp"
#include "jwrap/yyjson/yyjson.c"
#include <iostream>
#include <string>
#include <vector>

struct Address
{
	std::string city;
	int zip;
};

JW_JSON_FIELDS(Address, city, zip)

struct User
{
	std::string name;
	int age;
	std::vector<std::string> tags;
	Address address;
};

JW_JSON_FIELDS(User, name, age, tags, address)

int main()
{
	User original{"Alice", 30, {"cpp", "yyjson"}, {"Singapore", 123456}};

	// C++ struct -> JSON
	auto value = jw::to_value(original);
	std::string text = jw::stringify(value, true);

	std::cout << text << "\n\n";

	// JSON -> C++ struct
	auto parsed = jw::parse(text);
	User copy = jw::from_json<User>(parsed);

	std::cout << copy.name << '\n';
	std::cout << copy.age << '\n';
	std::cout << copy.address.city << '\n';
	std::cout << copy.tags.at(0) << '\n';
}
