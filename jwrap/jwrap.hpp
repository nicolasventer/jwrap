
#pragma once

#include "yyjson/yyjson.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace jw
{

	// ============================================================
	// Exceptions
	// ============================================================

	class error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class parse_error : public error
	{
	public:
		using error::error;
	};

	class type_error : public error
	{
	public:
		using error::error;
	};

	class missing_key : public error
	{
	public:
		using error::error;
	};

	enum class e_read_flag : uint8_t
	{
		none = YYJSON_READ_NOFLAG,
		allow_trailing_commas = YYJSON_READ_ALLOW_TRAILING_COMMAS,
		allow_comments = YYJSON_READ_ALLOW_COMMENTS,
		allow_comments_and_trailing_commas = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS,
	};

	enum class e_write_flag : uint8_t
	{
		compact = YYJSON_WRITE_NOFLAG,
		pretty = YYJSON_WRITE_PRETTY,
		pretty_two_spaces = YYJSON_WRITE_PRETTY_TWO_SPACES,
	};

	template <class T> struct is_optional : std::false_type
	{
	};

	template <class T> struct is_optional<std::optional<T>> : std::true_type
	{
	};

	template <class T> inline constexpr bool is_optional_v = is_optional<std::remove_cv_t<std::remove_reference_t<T>>>::value;

	// ============================================================
	// Document ownership
	// ============================================================

	struct Doc
	{
		yyjson_mut_doc* ptr = nullptr;

		Doc() : ptr(yyjson_mut_doc_new(nullptr))
		{
			if (ptr == nullptr) throw std::bad_alloc();
		}

		Doc(Doc&&) = delete;
		Doc& operator=(Doc&&) = delete;
		explicit Doc(yyjson_mut_doc* p) : ptr(p)
		{
			if (ptr == nullptr) throw std::bad_alloc();
		}

		~Doc()
		{
			if (ptr != nullptr) yyjson_mut_doc_free(ptr);
		}

		Doc(const Doc&) = delete;
		Doc& operator=(const Doc&) = delete;
	};

	// ============================================================
	// Forward declarations
	// ============================================================

	class Value;

	template <class T, class Enable = void> struct Converter;

	template <class T> struct StructTraits;

	template <class T> struct EnumTraits;

	template <class T> struct VariantTraits;

	template <class T> Value to_value(const T& value);

	template <class T> T from_json(const Value& value);

	namespace schema
	{

		template <class T> Value of();

	} // namespace schema

	// ============================================================
	// Value
	// ============================================================

	class Value
	{
		std::shared_ptr<Doc> doc_;
		yyjson_mut_val* root_ = nullptr;

		Value(std::shared_ptr<Doc> doc, yyjson_mut_val* root) : doc_(std::move(doc)), root_(root)
		{
			if (!doc_ || (root_ == nullptr)) throw error("Invalid JSON value");
		}

		static void require(bool condition, const char* message)
		{
			if (!condition) throw error(message);
		}

		[[nodiscard]] yyjson_mut_val* clone_into(const Value& child) const
		{
			auto* copy = yyjson_mut_val_mut_copy(doc_->ptr, child.root_);

			if (copy == nullptr) throw std::bad_alloc();

			return copy;
		}

		template <class Fn> void for_each_member(const Fn& fn) const
		{
			std::size_t idx = 0;
			std::size_t max = 0;
			yyjson_mut_val* key = nullptr;
			yyjson_mut_val* item = nullptr;
			yyjson_mut_obj_foreach(root_, idx, max, key, item) { fn(Value(doc_, key), Value(doc_, item)); }
		}

		friend struct Converter<Value>;

		template <class T, class Enable> friend struct Converter;

		template <class T> friend T from_json(const Value& value);

		template <class T> friend Value schema::of();

	public:
		Value(const Value&) = default;
		Value(Value&&) noexcept = default;
		Value& operator=(const Value&) = default;
		Value& operator=(Value&&) noexcept = default;

		// A standalone JSON null value.
		Value() : doc_(std::make_shared<Doc>()), root_(yyjson_mut_null(doc_->ptr))
		{
			if (root_ == nullptr) throw std::bad_alloc();

			yyjson_mut_doc_set_root(doc_->ptr, root_);
		}

		static Value null() { return Value(); }

		static Value object()
		{
			auto doc = std::make_shared<Doc>();
			auto* root = yyjson_mut_obj(doc->ptr);

			if (root == nullptr) throw std::bad_alloc();

			yyjson_mut_doc_set_root(doc->ptr, root);
			return Value(std::move(doc), root);
		}

		static Value array()
		{
			auto doc = std::make_shared<Doc>();
			auto* root = yyjson_mut_arr(doc->ptr);

			if (root == nullptr) throw std::bad_alloc();

			yyjson_mut_doc_set_root(doc->ptr, root);
			return Value(std::move(doc), root);
		}

		static Value parse(const std::string& text, e_read_flag flags = e_read_flag::none)
		{
			yyjson_read_err err{};

			yyjson_doc* parsed = yyjson_read_opts(
				const_cast<char*>(text.data()), text.size(), static_cast<yyjson_read_flag>(flags), nullptr, &err);

			if (parsed == nullptr)
			{
				throw parse_error(std::string("JSON parse error: ") + (err.msg != nullptr ? err.msg : "unknown error"));
			}

			yyjson_mut_doc* mutable_doc = yyjson_doc_mut_copy(parsed, nullptr);

			yyjson_doc_free(parsed);

			if (mutable_doc == nullptr) throw std::bad_alloc();

			std::shared_ptr<Doc> doc;

			try
			{
				doc = std::make_shared<Doc>(mutable_doc);
			}
			catch (...)
			{
				yyjson_mut_doc_free(mutable_doc);
				throw;
			}

			auto* root = yyjson_mut_doc_get_root(doc->ptr);

			if (root == nullptr) throw parse_error("Parsed JSON has no root");

			return Value(std::move(doc), root);
		}

		bool is_null() const { return yyjson_mut_is_null(root_); }

		bool is_object() const { return yyjson_mut_is_obj(root_); }

		bool is_array() const { return yyjson_mut_is_arr(root_); }

		bool is_string() const { return yyjson_mut_is_str(root_); }

		bool is_number() const { return yyjson_mut_is_num(root_); }

		bool is_bool() const { return yyjson_mut_is_bool(root_); }

		std::size_t size() const
		{
			if (is_object()) return yyjson_mut_obj_size(root_);

			if (is_array()) return yyjson_mut_arr_size(root_);

			return 0;
		}

		bool has_key(const std::string& key) const
		{
			if (!is_object()) return false;

			return yyjson_mut_obj_get(root_, key.c_str()) != nullptr;
		}

		Value operator[](const std::string& key) const
		{
			require(is_object(), "operator[] requires a JSON object");

			auto* item = yyjson_mut_obj_get(root_, key.c_str());

			if (item == nullptr) throw missing_key("Missing JSON key: " + key);

			return Value(doc_, item);
		}

		Value operator[](std::size_t index) const
		{
			require(is_array(), "operator[] requires a JSON array");

			auto* item = yyjson_mut_arr_get(root_, index);

			if (item == nullptr) throw std::out_of_range("JSON array index out of range");

			return Value(doc_, item);
		}

		// Insert or replace an object member.
		// An empty std::optional removes the key.
		template <class T> Value& set(const std::string& key, const T& value)
		{
			require(is_object(), "set() requires a JSON object");

			if constexpr (is_optional_v<T>)
			{
				if (!value.has_value())
				{
					if (yyjson_mut_obj_get(root_, key.c_str())) yyjson_mut_obj_remove_key(root_, key.c_str());
					return *this;
				}

				return set(key, *value);
			}

			Value child = to_value(value);

			if (yyjson_mut_obj_get(root_, key.c_str())) yyjson_mut_obj_remove_key(root_, key.c_str());

			auto* key_value = yyjson_mut_strncpy(doc_->ptr, key.data(), key.size());

			if (!key_value) throw std::bad_alloc();

			auto* child_value = clone_into(child);

			if (!yyjson_mut_obj_add(root_, key_value, child_value)) throw error("Failed to add JSON object member");

			return *this;
		}

		// Convenient support for C strings and string literals.
		Value& set(const std::string& key, const char* value)
		{
			if (value == nullptr) return set(key, nullptr);

			return set(key, std::string(value));
		}

		// Append an item to an array.
		template <class T> Value& push(const T& value)
		{
			require(is_array(), "push() requires a JSON array");

			Value child = to_value(value);
			auto* child_value = clone_into(child);

			if (!yyjson_mut_arr_append(root_, child_value)) throw error("Failed to append JSON array item");

			return *this;
		}

		// Read an array item as a typed value.
		template <class T> T at(std::size_t index) const
		{
			require(is_array(), "at() requires a JSON array");

			auto* item = yyjson_mut_arr_get(root_, index);

			if (!item) throw std::out_of_range("JSON array index out of range");

			Value view(doc_, item);
			return from_json<T>(view);
		}

		// Read an object member as a typed value.
		// std::optional<T> is nullopt when the key is missing or the value is null.
		template <class T> T get(const std::string& key) const
		{
			require(is_object(), "get(key) requires a JSON object");

			auto* item = yyjson_mut_obj_get(root_, key.c_str());

			if (!item)
			{
				if constexpr (is_optional_v<T>) return std::nullopt;
				else throw missing_key("Missing JSON key: " + key);
			}

			Value view(doc_, item);
			return from_json<T>(view);
		}

		// nullopt only when the key is missing. A present value is decoded as T.
		template <class T> std::optional<T> try_get(const std::string& key) const
		{
			require(is_object(), "try_get(key) requires a JSON object");

			auto* item = yyjson_mut_obj_get(root_, key.c_str());

			if (!item) return std::nullopt;

			Value view(doc_, item);
			return from_json<T>(view);
		}

		// Convert this value to a C++ type.
		template <class T> T get() const { return from_json<T>(*this); }

		template <class T, std::size_t N> void read_to(T (&destination)[N]) const
		{
			if (!is_array() || size() != N) throw type_error("Expected JSON array of the requested length");

			for (std::size_t i = 0; i < N; ++i) destination[i] = at<T>(i);
		}

		[[nodiscard]] yyjson_mut_val* mut_val() const noexcept { return root_; }

		[[nodiscard]] yyjson_mut_doc* mut_doc() const noexcept { return doc_ ? doc_->ptr : nullptr; }

		[[nodiscard]] std::string dump(e_write_flag flags) const
		{
			size_t length = 0;

			char* output = yyjson_mut_val_write(root_, static_cast<yyjson_write_flag>(flags), &length);

			if (output == nullptr) throw error("JSON serialization failed");

			try
			{
				std::string result(output, length);
				std::free(output);
				return result;
			}
			catch (...)
			{
				std::free(output);
				throw;
			}
		}

		[[nodiscard]] std::string dump(bool pretty = false) const
		{
			return dump(pretty ? e_write_flag::pretty : e_write_flag::compact);
		}
	};

	// ============================================================
	// Converter declarations and built-in conversions
	// ============================================================

	template <class T, class Enable> struct Converter
	{
		static_assert(!std::is_same_v<T, T>,
			"No jw::Converter available for this type. "
			"Use JW_JSON_OBJECT, JW_JSON_ENUM, JW_JSON_VARIANT_TAG, JW_JSON_VARIANT_KEY, "
			"or define a Converter specialization.");
	};

	namespace detail
	{

		template <class T> Value schema_of();

		template <class T> void set_schema_title(Value& schema);

		template <class T, class Tuple, std::size_t... I>
		Value schema_from_struct(const Tuple& fields, std::index_sequence<I...> /* indices */);

		inline Value with_variant_tag(Value schema, const char* tag, const char* name);

		inline Value with_variant_key(Value payload, const char* name);

		template <class Variant, std::size_t I> void append_variant_schemas(Value& branches);

	} // namespace detail

	namespace schema
	{

		struct field
		{
			const char* name = nullptr;
			Value schema;
			bool required = true;
		};

		inline Value string()
		{
			Value result = Value::object();
			result.set("type", "string");
			return result;
		}

		inline Value boolean()
		{
			Value result = Value::object();
			result.set("type", "boolean");
			return result;
		}

		inline Value integer()
		{
			Value result = Value::object();
			result.set("type", "integer");
			return result;
		}

		inline Value number()
		{
			Value result = Value::object();
			result.set("type", "number");
			return result;
		}

		inline Value null()
		{
			Value result = Value::object();
			result.set("type", "null");
			return result;
		}

		inline Value any() { return Value::object(); }

		inline Value array(const Value& items,
			std::optional<std::size_t> min_length = std::nullopt,
			std::optional<std::size_t> max_length = std::nullopt)
		{
			Value result = Value::object();
			result.set("type", "array");
			result.set("items", items);

			if (min_length.has_value()) result.set("minItems", *min_length);

			if (max_length.has_value()) result.set("maxItems", *max_length);

			return result;
		}

		inline Value map(const Value& value_schema)
		{
			Value result = Value::object();
			result.set("type", "object");
			result.set("additionalProperties", value_schema);
			return result;
		}

		inline field required(const char* name, Value schema) { return field{name, std::move(schema), true}; }

		inline field optional(const char* name, Value schema) { return field{name, std::move(schema), false}; }

	} // namespace schema

	namespace detail
	{

		template <class Range> Value object_schema(const Range& fields)
		{
			Value properties = Value::object();
			Value required_names = Value::array();

			for (const auto& item : fields)
			{
				properties.set(item.name, item.schema);

				if (item.required) required_names.push(std::string(item.name));
			}

			Value result = Value::object();
			result.set("type", "object");
			result.set("properties", properties);

			if (required_names.size() != 0) result.set("required", required_names);

			return result;
		}

		template <class Range> Value string_enum(const Range& names)
		{
			Value result = schema::string();
			Value values = Value::array();

			for (const auto& name : names) values.push(std::string(name));

			result.set("enum", values);
			return result;
		}

		inline Value keyword_schema(const char* key, Value value)
		{
			Value result = Value::object();
			result.set(key, std::move(value));
			return result;
		}

	} // namespace detail

	namespace schema
	{

		inline Value object(std::initializer_list<field> fields) { return detail::object_schema(fields); }

		inline Value enumeration(std::initializer_list<const char*> names) { return detail::string_enum(names); }

		inline Value any_of(std::initializer_list<Value> branches)
		{
			Value options = Value::array();

			for (const Value& branch : branches) options.push(branch);

			return detail::keyword_schema("anyOf", std::move(options));
		}

		inline Value one_of(std::initializer_list<Value> branches)
		{
			Value options = Value::array();

			for (const Value& branch : branches) options.push(branch);

			return detail::keyword_schema("oneOf", std::move(options));
		}

		inline Value all_of(std::initializer_list<Value> branches)
		{
			Value options = Value::array();

			for (const Value& branch : branches) options.push(branch);

			return detail::keyword_schema("allOf", std::move(options));
		}

		inline Value nullable(const Value& schema) { return any_of({schema, null()}); }

		inline Value ref(const char* name)
		{
			std::string escaped;
			escaped.reserve(std::strlen(name));

			for (const char* cursor = name; *cursor != '\0'; ++cursor)
			{
				if (*cursor == '~') escaped += "~0";
				else if (*cursor == '/') escaped += "~1";
				else escaped += *cursor;
			}

			Value result = Value::object();
			result.set("$ref", "#/$defs/" + escaped);
			return result;
		}

		inline Value defs(Value schema, std::initializer_list<std::pair<const char*, Value>> definitions)
		{
			if (definitions.size() == 0) return schema;

			if (!schema.is_object()) throw type_error("JSON schema must be an object");

			Value body = Value::object();

			for (const auto& entry : definitions) body.set(entry.first, entry.second);

			schema.set("$defs", body);
			return schema;
		}

	} // namespace schema

	// Strings
	template <> struct Converter<std::string>
	{
		static Value encode(const std::string& value)
		{
			auto doc = std::make_shared<Doc>();

			auto* root = yyjson_mut_strncpy(doc->ptr, value.data(), value.size());

			if (root == nullptr) throw std::bad_alloc();

			yyjson_mut_doc_set_root(doc->ptr, root);
			return Value(std::move(doc), root);
		}

		static std::string decode(const Value& value)
		{
			if (!value.is_string()) throw type_error("Expected JSON string");

			return std::string(yyjson_mut_get_str(value.root_), yyjson_mut_get_len(value.root_));
		}

		static Value schema() { return schema::string(); }
	};

	// C string pointers
	template <> struct Converter<const char*>
	{
		static Value encode(const char* value)
		{
			if (value == nullptr) return Value::null();

			return Converter<std::string>::encode(std::string(value));
		}

		static Value schema() { return schema::any_of({schema::string(), schema::null()}); }
	};

	// Booleans
	template <> struct Converter<bool>
	{
		static Value encode(bool value)
		{
			auto doc = std::make_shared<Doc>();
			auto* root = yyjson_mut_bool(doc->ptr, value);

			if (root == nullptr) throw std::bad_alloc();

			yyjson_mut_doc_set_root(doc->ptr, root);
			return Value(std::move(doc), root);
		}

		static bool decode(const Value& value)
		{
			if (!value.is_bool()) throw type_error("Expected JSON boolean");

			return yyjson_mut_get_bool(value.root_);
		}

		static Value schema() { return schema::boolean(); }
	};

	// Integral types
	template <class T> struct Converter<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
	{

		static Value encode(T value)
		{
			auto doc = std::make_shared<Doc>();
			yyjson_mut_val* root = nullptr;

			if constexpr (std::is_signed_v<T>)
			{
				root = yyjson_mut_sint(doc->ptr, static_cast<int64_t>(value));
			}
			else
			{
				root = yyjson_mut_uint(doc->ptr, static_cast<uint64_t>(value));
			}

			if (root == nullptr) throw std::bad_alloc();

			yyjson_mut_doc_set_root(doc->ptr, root);
			return Value(std::move(doc), root);
		}

		static T decode(const Value& value)
		{
			if (!value.is_number() || !yyjson_mut_is_int(value.root_))
			{
				throw type_error("Expected JSON integer");
			}

			// Positive integers are stored as uint64, including values above
			// int64 max. Negatives are stored as int64.
			if (yyjson_mut_is_sint(value.root_))
			{
				const int64_t n = yyjson_mut_get_sint(value.root_);

				if constexpr (std::is_signed_v<T>)
				{
					if (n < static_cast<int64_t>(std::numeric_limits<T>::lowest())
						|| n > static_cast<int64_t>(std::numeric_limits<T>::max()))
					{
						throw std::out_of_range("JSON integer out of range");
					}

					return static_cast<T>(n);
				}

				if (n < 0)
				{
					throw std::out_of_range("JSON unsigned integer out of range");
				}

				const auto positive = static_cast<uint64_t>(n);
				if (positive > static_cast<uint64_t>(std::numeric_limits<T>::max()))
				{
					throw std::out_of_range("JSON unsigned integer out of range");
				}

				return static_cast<T>(positive);
			}

			const uint64_t n = yyjson_mut_get_uint(value.root_);

			if constexpr (std::is_signed_v<T>)
			{
				if (n > static_cast<uint64_t>(std::numeric_limits<T>::max()))
				{
					throw std::out_of_range("JSON integer out of range");
				}

				return static_cast<T>(n);
			}

			if (n > static_cast<uint64_t>(std::numeric_limits<T>::max()))
			{
				throw std::out_of_range("JSON unsigned integer out of range");
			}

			return static_cast<T>(n);
		}

		static Value schema() { return schema::integer(); }
	};

	// Floating-point types
	template <class T> struct Converter<T, std::enable_if_t<std::is_floating_point_v<T>>>
	{

		static Value encode(T value)
		{
			if (!std::isfinite(value)) throw error("JSON cannot represent NaN or infinity");

			auto doc = std::make_shared<Doc>();
			auto* root = yyjson_mut_real(doc->ptr, static_cast<double>(value));

			if (root == nullptr) throw std::bad_alloc();

			yyjson_mut_doc_set_root(doc->ptr, root);
			return Value(std::move(doc), root);
		}

		static T decode(const Value& value)
		{
			if (!value.is_number()) throw type_error("Expected JSON number");

			const double n = yyjson_mut_get_num(value.root_);

			if (!std::isfinite(n) || n < -static_cast<double>(std::numeric_limits<T>::max())
				|| n > static_cast<double>(std::numeric_limits<T>::max()))
			{
				throw std::out_of_range("JSON floating-point value out of range");
			}

			return static_cast<T>(n);
		}

		static Value schema() { return schema::number(); }
	};

	// Null pointer -> JSON null
	template <> struct Converter<std::nullptr_t>
	{
		static Value encode(std::nullptr_t) { return Value::null(); }

		static std::nullptr_t decode(const Value& value)
		{
			if (!value.is_null()) throw type_error("Expected JSON null");

			return nullptr;
		}

		static Value schema() { return schema::null(); }
	};

	// Value itself
	template <> struct Converter<Value>
	{
		static Value encode(const Value& value) { return value; }

		static Value decode(const Value& value) { return value; }

		// Any JSON instance.
		static Value schema() { return schema::any(); }
	};

	// Vectors
	template <class T> struct Converter<std::vector<T>>
	{
		static Value encode(const std::vector<T>& items)
		{
			Value result = Value::array();

			for (const auto& item : items) result.push(item);

			return result;
		}

		static std::vector<T> decode(const Value& value)
		{
			if (!value.is_array()) throw type_error("Expected JSON array");

			std::vector<T> result;
			result.reserve(value.size());

			for (std::size_t i = 0; i < value.size(); ++i) result.push_back(value.at<T>(i));

			return result;
		}

		static Value schema() { return schema::array(detail::schema_of<T>()); }
	};

	// Optional values. Empty encodes as JSON null.
	// Object fields omit an empty optional through Value::set.
	template <class T> struct Converter<std::optional<T>>
	{
		static Value encode(const std::optional<T>& value)
		{
			if (!value.has_value()) return Value::null();

			return to_value(*value);
		}

		static std::optional<T> decode(const Value& value)
		{
			if (value.is_null()) return std::nullopt;

			return from_json<T>(value);
		}

		static Value schema() { return schema::nullable(detail::schema_of<T>()); }
	};

	namespace detail
	{

		template <class K> std::string object_key(const K& key)
		{
			Value encoded = to_value(key);

			if (!encoded.is_string()) throw type_error("JSON object key must be a string");

			return from_json<std::string>(encoded);
		}

	} // namespace detail

	template <class K, class V> struct Converter<std::map<K, V>>
	{
		static Value encode(const std::map<K, V>& items)
		{
			Value result = Value::object();

			for (const auto& [key, item] : items) result.set(detail::object_key(key), item);

			return result;
		}

		static std::map<K, V> decode(const Value& value)
		{
			if (!value.is_object()) throw type_error("Expected JSON object");

			std::map<K, V> result;
			value.for_each_member([&](Value key, Value item) { result.emplace(from_json<K>(key), from_json<V>(item)); });
			return result;
		}

		static Value schema() { return schema::map(detail::schema_of<V>()); }
	};

	template <class K, class V> struct Converter<std::vector<std::pair<K, V>>>
	{
		static Value encode(const std::vector<std::pair<K, V>>& items)
		{
			Value result = Value::object();

			for (const auto& [key, item] : items) result.set(detail::object_key(key), item);

			return result;
		}

		static std::vector<std::pair<K, V>> decode(const Value& value)
		{
			if (!value.is_object()) throw type_error("Expected JSON object");

			std::vector<std::pair<K, V>> result;
			result.reserve(value.size());
			value.for_each_member([&](Value key, Value item) { result.emplace_back(from_json<K>(key), from_json<V>(item)); });
			return result;
		}

		static Value schema() { return schema::map(detail::schema_of<V>()); }
	};

	template <class T, std::size_t N> struct Converter<std::array<T, N>>
	{
		static Value encode(const std::array<T, N>& items)
		{
			Value result = Value::array();

			for (const auto& item : items) result.push(item);

			return result;
		}

		static std::array<T, N> decode(const Value& value)
		{
			if (!value.is_array() || value.size() != N) throw type_error("Expected JSON array of the requested length");

			std::array<T, N> result{};

			for (std::size_t i = 0; i < N; ++i) result[i] = value.at<T>(i);

			return result;
		}

		static Value schema() { return schema::array(detail::schema_of<T>(), N, N); }
	};

	// C arrays can be encoded. Read them with Value::read_to.
	template <class T, std::size_t N> struct Converter<T[N]>
	{
		static Value encode(const T (&items)[N])
		{
			Value result = Value::array();

			for (std::size_t i = 0; i < N; ++i) result.push(items[i]);

			return result;
		}

		static Value schema() { return schema::array(detail::schema_of<T>(), N, N); }
	};

	// ============================================================
	// Generic conversion entry points
	// ============================================================

	template <class T> Value to_value(const T& value)
	{
		if constexpr (std::is_array_v<T>) return Converter<T>::encode(value);
		else return Converter<std::decay_t<T>>::encode(value);
	}

	template <class T> T from_json(const Value& value) { return Converter<T>::decode(value); }

	inline Value object() { return Value::object(); }

	inline Value array() { return Value::array(); }

	inline Value parse(const std::string& text, e_read_flag flags = e_read_flag::none) { return Value::parse(text, flags); }

	inline std::string stringify(const Value& value, bool pretty = false) { return value.dump(pretty); }

	inline std::string stringify(const Value& value, e_write_flag flags) { return value.dump(flags); }

	// ============================================================
	// Struct field registration
	// ============================================================

	template <class T, class = void> struct has_struct_traits : std::false_type
	{
	};

	template <class T> struct has_struct_traits<T, std::void_t<decltype(StructTraits<T>::fields())>> : std::true_type
	{
	};

	template <class T, class = void> struct has_enum_traits : std::false_type
	{
	};

	template <class T> struct has_enum_traits<T, std::void_t<decltype(EnumTraits<T>::entries())>> : std::true_type
	{
	};

	template <class T, class = void> struct has_variant_traits : std::false_type
	{
	};

	template <class T> struct has_variant_traits<T, std::void_t<decltype(VariantTraits<T>::names())>> : std::true_type
	{
	};

	template <class T, class = void> struct variant_has_tag : std::false_type
	{
	};

	template <class T> struct variant_has_tag<T, std::void_t<decltype(VariantTraits<T>::tag())>> : std::true_type
	{
	};

	// Encode a registered struct.
	template <class T, class Tuple, std::size_t... I>
	Value encode_struct_fields(const T& object, const Tuple& fields, std::index_sequence<I...> /* indices */)
	{

		Value result = Value::object();

		(result.set(std::get<I>(fields).first, object.*(std::get<I>(fields).second)), ...);

		return result;
	}

	// Decode a registered struct.
	template <class T, class Tuple, std::size_t... I>
	T decode_struct_fields(const Value& value, const Tuple& fields, std::index_sequence<I...> /* indices */)
	{

		T result{};

		((result.*(std::get<I>(fields).second)
			 = value.get<std::remove_cv_t<std::remove_reference_t<decltype(result.*(std::get<I>(fields).second))>>>(
				 std::get<I>(fields).first)),
			...);

		return result;
	}

	// Registered struct converter.
	template <class T> struct Converter<T, std::enable_if_t<has_struct_traits<T>::value>>
	{

		static Value encode(const T& object)
		{
			constexpr std::size_t count = std::tuple_size_v<decltype(StructTraits<T>::fields())>;

			return encode_struct_fields(object, StructTraits<T>::fields(), std::make_index_sequence<count>{});
		}

		static T decode(const Value& value)
		{
			if (!value.is_object()) throw type_error("Expected JSON object for registered struct");

			constexpr std::size_t count = std::tuple_size_v<decltype(StructTraits<T>::fields())>;

			return decode_struct_fields<T>(value, StructTraits<T>::fields(), std::make_index_sequence<count>{});
		}

		static Value schema()
		{
			constexpr std::size_t count = std::tuple_size_v<decltype(StructTraits<T>::fields())>;

			return detail::schema_from_struct<T>(StructTraits<T>::fields(), std::make_index_sequence<count>{});
		}
	};

	// Registered enum converter. Values are JSON strings.
	template <class T> struct Converter<T, std::enable_if_t<has_enum_traits<T>::value>>
	{

		static Value encode(T value)
		{
			for (const auto& entry : EnumTraits<T>::entries())
			{
				if (entry.first == value) return Converter<std::string>::encode(entry.second);
			}

			throw type_error("Unknown enum value");
		}

		static T decode(const Value& value)
		{
			if (!value.is_string()) throw type_error("Expected JSON string for enum");

			const std::string text = Converter<std::string>::decode(value);

			for (const auto& entry : EnumTraits<T>::entries())
			{
				if (text == entry.second) return entry.first;
			}

			throw type_error("Unknown enum name");
		}

		static Value schema()
		{
			std::vector<const char*> names;

			for (const auto& entry : EnumTraits<T>::entries()) names.push_back(entry.second);

			Value enumerated = detail::string_enum(names);
			Value result = Value::object();
			detail::set_schema_title<T>(result);
			result.set("type", enumerated.get<std::string>("type"));
			result.set("enum", enumerated["enum"]);
			return result;
		}
	};

	namespace detail
	{

		template <class Names> constexpr bool variant_names_are_unique(const Names& names)
		{
			for (std::size_t i = 0; i < names.size(); ++i)
			{
				for (std::size_t j = i + 1; j < names.size(); ++j)
				{
					if (std::string_view(names[i]) == std::string_view(names[j])) return false;
				}
			}

			return true;
		}

		// Match a tag field value to an alternative, in registration order.
		template <class Variant, std::size_t I> Variant decode_variant_tag(const Value& value, const std::string& tag)
		{
			constexpr auto names = VariantTraits<Variant>::names();

			if constexpr (I >= std::variant_size_v<Variant>)
			{
				throw type_error("Unknown variant tag");
			}
			else if (tag == names[I])
			{
				return Variant(std::in_place_index<I>, from_json<std::variant_alternative_t<I, Variant>>(value));
			}
			else
			{
				return decode_variant_tag<Variant, I + 1>(value, tag);
			}
		}

		// Match the single object key to an alternative, in registration order.
		template <class Variant, std::size_t I> Variant decode_variant_key(const Value& value)
		{
			constexpr auto names = VariantTraits<Variant>::names();

			if constexpr (I >= std::variant_size_v<Variant>)
			{
				throw type_error("Unknown variant key");
			}
			else if (value.has_key(names[I]))
			{
				return Variant(std::in_place_index<I>, from_json<std::variant_alternative_t<I, Variant>>(value[names[I]]));
			}
			else
			{
				return decode_variant_key<Variant, I + 1>(value);
			}
		}

	} // namespace detail

	// Registered variant converter.
	// Tag mode reads a discriminator field and decodes the same object.
	// Key mode reads the value of the single object key.
	template <class... Ts> struct Converter<std::variant<Ts...>, std::enable_if_t<has_variant_traits<std::variant<Ts...>>::value>>
	{
		using variant_type = std::variant<Ts...>;

		static constexpr auto alternative_names = VariantTraits<variant_type>::names();

		static_assert(alternative_names.size() == sizeof...(Ts), "JW_JSON_VARIANT name count must match the variant");
		static_assert(detail::variant_names_are_unique(alternative_names), "JW_JSON_VARIANT names must be unique");

		static Value encode(const variant_type& value)
		{
			if (value.valueless_by_exception()) throw type_error("Valueless variant");

			Value payload = std::visit([](const auto& alternative) { return to_value(alternative); }, value);

			if constexpr (variant_has_tag<variant_type>::value)
			{
				if (!payload.is_object()) throw type_error("Tagged variant alternative must be a JSON object");

				payload.set(VariantTraits<variant_type>::tag(), alternative_names[value.index()]);
				return payload;
			}

			Value result = Value::object();
			result.set(alternative_names[value.index()], payload);
			return result;
		}

		static variant_type decode(const Value& value)
		{
			if (!value.is_object()) throw type_error("Expected JSON object for variant");

			if constexpr (variant_has_tag<variant_type>::value)
			{
				const std::string tag = value.get<std::string>(VariantTraits<variant_type>::tag());
				return detail::decode_variant_tag<variant_type, 0>(value, tag);
			}

			if (value.size() != 1) throw type_error("Expected a single-key JSON object for variant");

			return detail::decode_variant_key<variant_type, 0>(value);
		}

		static Value schema()
		{
			Value branches = Value::array();
			detail::append_variant_schemas<variant_type, 0>(branches);

			Value body = detail::keyword_schema("oneOf", std::move(branches));
			Value result = Value::object();
			detail::set_schema_title<variant_type>(result);
			result.set("oneOf", body["oneOf"]);
			return result;
		}
	};

	// ============================================================
	// JSON Schema (draft 2020-12)
	// ============================================================

	namespace detail
	{

		struct SchemaDef
		{
			const char* name = nullptr;
			Value body;
			void (*id)() = nullptr;
		};

		struct SchemaScope
		{
			static inline thread_local int depth = 0;
			static inline thread_local std::vector<SchemaDef>* defs = nullptr;

			std::vector<SchemaDef> local;
			bool outer;

			SchemaScope() : outer(depth++ == 0)
			{
				if (outer) defs = &local;
			}

			SchemaScope(const SchemaScope&) = delete;
			SchemaScope& operator=(const SchemaScope&) = delete;

			~SchemaScope()
			{
				if (outer) defs = nullptr;

				--depth;
			}
		};

		template <class T> struct schema_state
		{
			static inline thread_local int depth = 0;
			static inline thread_local bool used_ref = false;
		};

		template <class T> void schema_identity() {}

		template <class T, class = void> struct has_struct_type_name : std::false_type
		{
		};

		template <class T>
		struct has_struct_type_name<T, std::void_t<decltype(StructTraits<T>::type_name())>> : std::true_type
		{
		};

		template <class T, class = void> struct has_enum_type_name : std::false_type
		{
		};

		template <class T>
		struct has_enum_type_name<T, std::void_t<decltype(EnumTraits<T>::type_name())>> : std::true_type
		{
		};

		template <class T, class = void> struct has_variant_type_name : std::false_type
		{
		};

		template <class T>
		struct has_variant_type_name<T, std::void_t<decltype(VariantTraits<T>::type_name())>> : std::true_type
		{
		};

		template <class T, class = void> struct converter_has_schema : std::false_type
		{
		};

		template <class T>
		struct converter_has_schema<T, std::void_t<decltype(Converter<T>::schema())>> : std::true_type
		{
		};

		template <class T>
		inline constexpr bool is_schema_named_v = has_struct_traits<T>::value || has_enum_traits<T>::value
			|| has_variant_traits<T>::value;

		template <class T> void set_schema_title(Value& schema)
		{
			if constexpr (has_struct_traits<T>::value)
			{
				if constexpr (has_struct_type_name<T>::value) schema.set("title", StructTraits<T>::type_name());
			}
			else if constexpr (has_enum_traits<T>::value)
			{
				if constexpr (has_enum_type_name<T>::value) schema.set("title", EnumTraits<T>::type_name());
			}
			else if constexpr (has_variant_traits<T>::value)
			{
				if constexpr (has_variant_type_name<T>::value) schema.set("title", VariantTraits<T>::type_name());
			}
		}

		template <class T> const char* schema_type_name()
		{
			if constexpr (has_struct_traits<T>::value && has_struct_type_name<T>::value)
			{
				return StructTraits<T>::type_name();
			}
			else if constexpr (has_enum_traits<T>::value && has_enum_type_name<T>::value)
			{
				return EnumTraits<T>::type_name();
			}
			else if constexpr (has_variant_traits<T>::value && has_variant_type_name<T>::value)
			{
				return VariantTraits<T>::type_name();
			}
			else
			{
				return nullptr;
			}
		}

		template <class T> void note_schema_def(const char* name, const Value& body)
		{
			auto* defs = SchemaScope::defs;

			if (defs == nullptr) throw type_error("Schema definitions are unavailable");

			void (*id)() = &schema_identity<T>;

			for (const auto& def : *defs)
			{
				if (def.id == id) return;

				if (std::strcmp(def.name, name) == 0) throw type_error("Duplicate schema name");
			}

			defs->push_back(SchemaDef{name, Value::parse(body.dump()), id});
		}

		inline Value with_variant_tag(Value schema, const char* tag, const char* name)
		{
			if (!schema.is_object() || !schema.has_key("type") || schema.get<std::string>("type") != "object")
			{
				throw type_error("Tagged variant alternative must be a JSON object");
			}

			if (!schema.has_key("properties")) schema.set("properties", Value::object());

			Value constant = Value::object();
			constant.set("const", name);
			schema["properties"].set(tag, constant);

			if (!schema.has_key("required"))
			{
				Value required = Value::array();
				required.push(std::string(tag));
				schema.set("required", required);
				return schema;
			}

			Value required = schema["required"];

			for (std::size_t index = 0; index < required.size(); ++index)
			{
				if (required.at<std::string>(index) == tag) return schema;
			}

			required.push(std::string(tag));
			return schema;
		}

		inline Value with_variant_key(Value payload, const char* name)
		{
			Value result = Value::object();
			result.set("type", "object");

			Value properties = Value::object();
			properties.set(name, payload);
			result.set("properties", properties);

			Value required = Value::array();
			required.push(std::string(name));
			result.set("required", required);
			result.set("additionalProperties", false);
			return result;
		}

		template <class Variant, std::size_t I> void append_variant_schemas(Value& branches)
		{
			if constexpr (I < std::variant_size_v<Variant>)
			{
				using Alternative = std::variant_alternative_t<I, Variant>;
				constexpr auto names = VariantTraits<Variant>::names();
				Value alternative = schema_of<Alternative>();

				if constexpr (variant_has_tag<Variant>::value)
				{
					branches.push(with_variant_tag(std::move(alternative), VariantTraits<Variant>::tag(), names[I]));
				}
				else
				{
					branches.push(with_variant_key(std::move(alternative), names[I]));
				}

				append_variant_schemas<Variant, I + 1>(branches);
			}
		}

		template <class T, class Tuple, std::size_t... I>
		Value schema_from_struct(const Tuple& fields, std::index_sequence<I...> /* indices */)
		{
			std::vector<schema::field> items;

			auto add = [&](const char* name, auto member) {
				using Field = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<T>().*member)>>;
				Value child = schema_of<Field>();

				if constexpr (is_optional_v<Field>) items.push_back(schema::optional(name, std::move(child)));
				else items.push_back(schema::required(name, std::move(child)));
			};

			(add(std::get<I>(fields).first, std::get<I>(fields).second), ...);

			Value body = object_schema(items);
			Value result = Value::object();
			set_schema_title<T>(result);
			result.set("type", body.get<std::string>("type"));
			result.set("properties", body["properties"]);

			if (body.has_key("required")) result.set("required", body["required"]);

			return result;
		}

		template <class T> Value schema_of()
		{
			if constexpr (is_schema_named_v<T>)
			{
				if (schema_state<T>::depth > 0)
				{
					const char* name = schema_type_name<T>();

					if (name == nullptr) throw type_error("Recursive type has no schema name");

					schema_state<T>::used_ref = true;
					return schema::ref(name);
				}

				struct DepthGuard
				{
					DepthGuard()
					{
						schema_state<T>::used_ref = false;
						++schema_state<T>::depth;
					}

					~DepthGuard() { --schema_state<T>::depth; }
				} guard;

				Value body = Converter<T>::schema();

				if (schema_state<T>::used_ref) note_schema_def<T>(schema_type_name<T>(), body);

				return body;
			}
			else if constexpr (converter_has_schema<T>::value)
			{
				return Converter<T>::schema();
			}
			else
			{
				static_assert(converter_has_schema<T>::value,
					"No schema for this type. Add static Value schema() to jw::Converter<T>.");
				return Value::object();
			}
		}

	} // namespace detail

	namespace schema
	{

		template <class T> Value of()
		{
			using Type = std::conditional_t<std::is_array_v<std::remove_reference_t<T>>,
				std::remove_reference_t<T>,
				std::decay_t<T>>;

			detail::SchemaScope scope;
			Value fragment = detail::schema_of<Type>();

			if (!scope.outer) return fragment;

			if (!fragment.is_object()) throw type_error("JSON schema must be an object");

			Value result = Value::object();
			result.set("$schema", "https://json-schema.org/draft/2020-12/schema");
			fragment.for_each_member([&](Value key, Value item) { result.set(from_json<std::string>(key), item); });

			if (detail::SchemaScope::defs != nullptr && !detail::SchemaScope::defs->empty())
			{
				Value definitions = Value::object();

				for (const auto& def : *detail::SchemaScope::defs) definitions.set(def.name, def.body);

				result.set("$defs", definitions);
			}

			return result;
		}

	} // namespace schema

} // namespace jw

// ============================================================
// JW_JSON_OBJECT macro (up to 8 fields)
// Invoke at global namespace scope.
// ============================================================

#define JW_DETAIL_FIELD(TYPE, FIELD) std::make_pair(#FIELD, &TYPE::FIELD)

#define JW_DETAIL_FIELDS_1(T, a) JW_DETAIL_FIELD(T, a)

#define JW_DETAIL_FIELDS_2(T, a, b) JW_DETAIL_FIELDS_1(T, a), JW_DETAIL_FIELD(T, b)

#define JW_DETAIL_FIELDS_3(T, a, b, c) JW_DETAIL_FIELDS_2(T, a, b), JW_DETAIL_FIELD(T, c)

#define JW_DETAIL_FIELDS_4(T, a, b, c, d) JW_DETAIL_FIELDS_3(T, a, b, c), JW_DETAIL_FIELD(T, d)

#define JW_DETAIL_FIELDS_5(T, a, b, c, d, e) JW_DETAIL_FIELDS_4(T, a, b, c, d), JW_DETAIL_FIELD(T, e)

#define JW_DETAIL_FIELDS_6(T, a, b, c, d, e, f) JW_DETAIL_FIELDS_5(T, a, b, c, d, e), JW_DETAIL_FIELD(T, f)

#define JW_DETAIL_FIELDS_7(T, a, b, c, d, e, f, g) JW_DETAIL_FIELDS_6(T, a, b, c, d, e, f), JW_DETAIL_FIELD(T, g)

#define JW_DETAIL_FIELDS_8(T, a, b, c, d, e, f, g, h) JW_DETAIL_FIELDS_7(T, a, b, c, d, e, f, g), JW_DETAIL_FIELD(T, h)

#define JW_DETAIL_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

#define JW_DETAIL_FIELDS(T, ...)     \
	JW_DETAIL_GET_MACRO(__VA_ARGS__, \
		JW_DETAIL_FIELDS_8,          \
		JW_DETAIL_FIELDS_7,          \
		JW_DETAIL_FIELDS_6,          \
		JW_DETAIL_FIELDS_5,          \
		JW_DETAIL_FIELDS_4,          \
		JW_DETAIL_FIELDS_3,          \
		JW_DETAIL_FIELDS_2,          \
		JW_DETAIL_FIELDS_1,          \
		extra)(T, __VA_ARGS__)

#define JW_JSON_OBJECT(TYPE, ...)                                                                           \
	namespace jw                                                                                            \
	{                                                                                                       \
		template <> struct StructTraits<TYPE>                                                               \
		{                                                                                                   \
			static constexpr const char* type_name() { return #TYPE; }                                      \
			static constexpr auto fields() { return std::make_tuple(JW_DETAIL_FIELDS(TYPE, __VA_ARGS__)); } \
		};                                                                                                  \
	}

// ============================================================
// JW_JSON_ENUM macro (up to 32 enumerators)
// Invoke at global namespace scope.
// ============================================================

#define JW_DETAIL_ENUM(TYPE, NAME)                     \
	std::pair<TYPE, const char*> { TYPE::NAME, #NAME }

#define JW_DETAIL_ENUMS_1(T, a)		  JW_DETAIL_ENUM(T, a)
#define JW_DETAIL_ENUMS_2(T, a, ...)  JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_1(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_3(T, a, ...)  JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_2(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_4(T, a, ...)  JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_3(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_5(T, a, ...)  JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_4(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_6(T, a, ...)  JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_5(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_7(T, a, ...)  JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_6(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_8(T, a, ...)  JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_7(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_9(T, a, ...)  JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_8(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_10(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_9(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_11(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_10(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_12(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_11(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_13(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_12(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_14(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_13(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_15(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_14(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_16(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_15(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_17(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_16(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_18(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_17(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_19(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_18(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_20(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_19(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_21(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_20(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_22(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_21(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_23(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_22(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_24(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_23(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_25(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_24(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_26(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_25(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_27(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_26(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_28(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_27(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_29(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_28(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_30(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_29(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_31(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_30(T, __VA_ARGS__)
#define JW_DETAIL_ENUMS_32(T, a, ...) JW_DETAIL_ENUM(T, a), JW_DETAIL_ENUMS_31(T, __VA_ARGS__)

#define JW_DETAIL_ENUM_GET(_1, \
	_2,                        \
	_3,                        \
	_4,                        \
	_5,                        \
	_6,                        \
	_7,                        \
	_8,                        \
	_9,                        \
	_10,                       \
	_11,                       \
	_12,                       \
	_13,                       \
	_14,                       \
	_15,                       \
	_16,                       \
	_17,                       \
	_18,                       \
	_19,                       \
	_20,                       \
	_21,                       \
	_22,                       \
	_23,                       \
	_24,                       \
	_25,                       \
	_26,                       \
	_27,                       \
	_28,                       \
	_29,                       \
	_30,                       \
	_31,                       \
	_32,                       \
	NAME,                      \
	...)                       \
	NAME

#define JW_DETAIL_ENUMS(T, ...)     \
	JW_DETAIL_ENUM_GET(__VA_ARGS__, \
		JW_DETAIL_ENUMS_32,         \
		JW_DETAIL_ENUMS_31,         \
		JW_DETAIL_ENUMS_30,         \
		JW_DETAIL_ENUMS_29,         \
		JW_DETAIL_ENUMS_28,         \
		JW_DETAIL_ENUMS_27,         \
		JW_DETAIL_ENUMS_26,         \
		JW_DETAIL_ENUMS_25,         \
		JW_DETAIL_ENUMS_24,         \
		JW_DETAIL_ENUMS_23,         \
		JW_DETAIL_ENUMS_22,         \
		JW_DETAIL_ENUMS_21,         \
		JW_DETAIL_ENUMS_20,         \
		JW_DETAIL_ENUMS_19,         \
		JW_DETAIL_ENUMS_18,         \
		JW_DETAIL_ENUMS_17,         \
		JW_DETAIL_ENUMS_16,         \
		JW_DETAIL_ENUMS_15,         \
		JW_DETAIL_ENUMS_14,         \
		JW_DETAIL_ENUMS_13,         \
		JW_DETAIL_ENUMS_12,         \
		JW_DETAIL_ENUMS_11,         \
		JW_DETAIL_ENUMS_10,         \
		JW_DETAIL_ENUMS_9,          \
		JW_DETAIL_ENUMS_8,          \
		JW_DETAIL_ENUMS_7,          \
		JW_DETAIL_ENUMS_6,          \
		JW_DETAIL_ENUMS_5,          \
		JW_DETAIL_ENUMS_4,          \
		JW_DETAIL_ENUMS_3,          \
		JW_DETAIL_ENUMS_2,          \
		JW_DETAIL_ENUMS_1,          \
		extra)(T, __VA_ARGS__)

#define JW_JSON_ENUM(TYPE, ...)                                                                        \
	namespace jw                                                                                       \
	{                                                                                                  \
		template <> struct EnumTraits<TYPE>                                                            \
		{                                                                                              \
			static constexpr const char* type_name() { return #TYPE; }                                 \
			static constexpr auto entries() { return std::array{JW_DETAIL_ENUMS(TYPE, __VA_ARGS__)}; } \
		};                                                                                             \
	}

// ============================================================
// JW_JSON_VARIANT_TAG and JW_JSON_VARIANT_KEY (up to 8 names)
// Invoke at global namespace scope.
// The variant type must be a using-alias: a comma inside the
// macro argument list would split std::variant<A, B>.
// Names are JSON strings in alternative order.
// ============================================================

#define JW_DETAIL_VARIANT_NAMES_1(a)					  #a
#define JW_DETAIL_VARIANT_NAMES_2(a, b)					  #a, #b
#define JW_DETAIL_VARIANT_NAMES_3(a, b, c)				  #a, #b, #c
#define JW_DETAIL_VARIANT_NAMES_4(a, b, c, d)			  #a, #b, #c, #d
#define JW_DETAIL_VARIANT_NAMES_5(a, b, c, d, e)		  #a, #b, #c, #d, #e
#define JW_DETAIL_VARIANT_NAMES_6(a, b, c, d, e, f)		  #a, #b, #c, #d, #e, #f
#define JW_DETAIL_VARIANT_NAMES_7(a, b, c, d, e, f, g)	  #a, #b, #c, #d, #e, #f, #g
#define JW_DETAIL_VARIANT_NAMES_8(a, b, c, d, e, f, g, h) #a, #b, #c, #d, #e, #f, #g, #h

#define JW_DETAIL_VARIANT_NAMES(...) \
	JW_DETAIL_GET_MACRO(__VA_ARGS__, \
		JW_DETAIL_VARIANT_NAMES_8,   \
		JW_DETAIL_VARIANT_NAMES_7,   \
		JW_DETAIL_VARIANT_NAMES_6,   \
		JW_DETAIL_VARIANT_NAMES_5,   \
		JW_DETAIL_VARIANT_NAMES_4,   \
		JW_DETAIL_VARIANT_NAMES_3,   \
		JW_DETAIL_VARIANT_NAMES_2,   \
		JW_DETAIL_VARIANT_NAMES_1,   \
		extra)(__VA_ARGS__)

#define JW_DETAIL_VARIANT_COUNT_I(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N

#define JW_DETAIL_VARIANT_COUNT(...) JW_DETAIL_VARIANT_COUNT_I(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, extra)

#define JW_JSON_VARIANT_TAG(TYPE, FIELD, ...)                                                                               \
	namespace jw                                                                                                            \
	{                                                                                                                       \
		template <> struct VariantTraits<TYPE>                                                                              \
		{                                                                                                                   \
			static constexpr const char* type_name() { return #TYPE; }                                                       \
			static constexpr const char* tag() { return #FIELD; }                                                           \
			static constexpr auto names()                                                                                   \
			{                                                                                                               \
				return std::array<const char*, JW_DETAIL_VARIANT_COUNT(__VA_ARGS__)>{JW_DETAIL_VARIANT_NAMES(__VA_ARGS__)}; \
			}                                                                                                               \
		};                                                                                                                  \
	}

#define JW_JSON_VARIANT_KEY(TYPE, ...)                                                                                      \
	namespace jw                                                                                                            \
	{                                                                                                                       \
		template <> struct VariantTraits<TYPE>                                                                              \
		{                                                                                                                   \
			static constexpr const char* type_name() { return #TYPE; }                                                       \
			static constexpr auto names()                                                                                   \
			{                                                                                                               \
				return std::array<const char*, JW_DETAIL_VARIANT_COUNT(__VA_ARGS__)>{JW_DETAIL_VARIANT_NAMES(__VA_ARGS__)}; \
			}                                                                                                               \
		};                                                                                                                  \
	}
