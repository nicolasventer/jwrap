# jwrap

## Description

jwrap is a **C++ library** that lets you **read and write JSON** and convert between JSON and your own types. It wraps [yyjson](https://github.com/ibireme/yyjson) (version: 21st May 2026, commit: 6945ccb).

## Features

- Parse JSON text into a mutable `jw::Value`, build objects and arrays, and serialize them
- Typed `get`, `try_get`, `at`, and `set` / `push`, with exceptions for missing keys, type mismatches, and out-of-range numbers
- `std::optional`: a missing key or JSON null reads as `nullopt`, and an empty optional is omitted from an object
- `std::vector`, `std::array`, C arrays, `std::map`, and `std::vector<std::pair>`
- Struct fields registered with `JW_JSON_FIELDS` (up to 8 fields), or a custom `jw::Converter`
- Optional parse flags for comments and trailing commas
- Compact output, 4-space pretty print, and 2-space pretty print
- Access to the underlying `yyjson_mut_val` and `yyjson_mut_doc`
- Fast and dependency-free (yyjson is included in the repo)

## Installation

1. Add the `jwrap` folder to your include path, or include it by its path from the project root.
2. Include [`jwrap.hpp`](jwrap/jwrap.hpp) where you use the library.
3. Compile [`yyjson.c`](jwrap/yyjson/yyjson.c) in **exactly one** translation unit. The example does that by including the `.c` file:

```cpp
#include "jwrap/jwrap.hpp"
#include "jwrap/yyjson/yyjson.c"
```

You can instead pass `jwrap/yyjson/yyjson.c` to the compiler and include only the header in your sources.

**Getting the code:** Clone this repo or download the `jwrap` folder via [this link](https://download-directory.github.io/?url=https%3A%2F%2Fgithub.com%2Fnicolasventer%2Fyyjson-wrap%2Ftree%2Fmain%2Fjwrap). The yyjson sources are included under `jwrap/yyjson/`; you can also fetch the latest from the [yyjson repo](https://github.com/ibireme/yyjson/tree/master/src) if needed.

### Requirements

- **C++17** or later
- No external dependencies (yyjson is vendored in the repository)

## Example

Build the example with `build.bat` or `./build.sh`.

Content of [example.cpp](example.cpp)

```cpp
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
```

**Output:**

```
{
    "name": "Alice",
    "age": 30,
    "tags": [
        "cpp",
        "yyjson"
    ],
    "address": {
        "city": "Singapore",
        "zip": 123456
    }
}

Alice
30
Singapore
cpp
```

## Testing

Tests use [doctest](https://github.com/doctest/doctest) and live under `tests/`. From the project root:

```bat
tests\exec_test.bat
```

This compiles `tests/testMain.cpp` and runs the resulting executable. A coverage script is also available at `tests/exec_cov.bat`.

## Usage

### Reading JSON

```cpp
jw::Value root = jw::parse(jsonString);

std::string name = root.get<std::string>("name");
int age = root["age"].get<int>();
std::vector<std::string> hobbies = root.get<std::vector<std::string>>("hobbies");

if (root.has_key("address")) {
    std::string city = root["address"]["city"].get<std::string>();
}

// nullopt when the key is absent. A present null or a wrong type still throws.
std::optional<int> maybeAge = root.try_get<int>("age");

// nullopt when the key is absent or the value is JSON null.
std::optional<int> score = root.get<std::optional<int>>("score");
```

Parse comments and trailing commas with `jw::e_read_flag`:

```cpp
jw::Value relaxed = jw::parse(
    text, jw::e_read_flag::allow_comments_and_trailing_commas);
```

`e_read_flag` also has `allow_comments` and `allow_trailing_commas`.

### Writing JSON

```cpp
jw::Value root = jw::object();
root.set("name", "Alice").set("age", 25);

jw::Value hobbies = jw::array();
hobbies.push("reading").push("coding");
root.set("hobbies", hobbies);

// An empty optional removes the key. A present one writes the value.
root.set("score", std::optional<int>{7});
root.set("nickname", std::optional<std::string>{});

std::string compact = root.dump();
std::string pretty = root.dump(true); // 4 spaces
std::string twoSpaces = root.dump(jw::e_write_flag::pretty_two_spaces);
```

`jw::stringify` is the same as `dump`. `set` replaces an existing key.

### Containers

```cpp
std::map<std::string, int> metadata{{"level", 2}};
root.set("metadata", metadata);

std::vector<std::pair<std::string, int>> ordered{{"b", 2}, {"a", 1}};
root.set("ordered", ordered);

std::array<int, 3> scores{1, 2, 3};
root.set("scores", scores);

int raw[3] = {4, 5, 6};
root.set("raw", raw);
int destination[3] = {};
root["raw"].read_to(destination);
```

Object keys must encode as JSON strings. A `std::map` is ordered by key. A `std::vector<std::pair>` keeps insertion order. `std::array` and `read_to` throw when the JSON array length does not match.

### Custom types

`JW_JSON_FIELDS` is invoked at global scope and accepts up to 8 fields. A `std::optional` field is omitted when empty.

```cpp
struct Address {
    std::string city;
    int zip;
};

JW_JSON_FIELDS(Address, city, zip)
```

For a type the macro does not cover, specialize `jw::Converter<T>` with `encode` and `decode`.

### Errors and yyjson handles

`jw::parse_error`, `jw::type_error`, and `jw::missing_key` derive from `jw::error`. Number conversions throw `std::out_of_range` when the value does not fit. NaN and infinity cannot be encoded.

`mut_val()` and `mut_doc()` return the `yyjson_mut_val*` and `yyjson_mut_doc*` owned by the value. Do not free them.

## License

MIT License. See [LICENSE file](LICENSE).  
Please refer me with:

    Copyright (c) Nicolas VENTER. All rights reserved.
