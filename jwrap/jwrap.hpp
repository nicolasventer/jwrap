
#pragma once

#include "yyjson/yyjson.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace jw {

// ============================================================
// Exceptions
// ============================================================

class error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class parse_error : public error {
public:
    using error::error;
};

class type_error : public error {
public:
    using error::error;
};

class missing_key : public error {
public:
    using error::error;
};

enum class read_flag : uint32_t {
    none = YYJSON_READ_NOFLAG,
    allow_trailing_commas = YYJSON_READ_ALLOW_TRAILING_COMMAS,
    allow_comments = YYJSON_READ_ALLOW_COMMENTS,
    allow_comments_and_trailing_commas =
        YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS,
};

enum class write_flag : uint32_t {
    compact = YYJSON_WRITE_NOFLAG,
    pretty = YYJSON_WRITE_PRETTY,
    pretty_two_spaces = YYJSON_WRITE_PRETTY_TWO_SPACES,
};

template<class T>
struct is_optional : std::false_type {};

template<class T>
struct is_optional<std::optional<T>> : std::true_type {};

template<class T>
inline constexpr bool is_optional_v =
    is_optional<std::remove_cv_t<std::remove_reference_t<T>>>::value;

// ============================================================
// Document ownership
// ============================================================

struct Doc {
    yyjson_mut_doc* ptr = nullptr;

    Doc() : ptr(yyjson_mut_doc_new(nullptr)) {
        if (!ptr)
            throw std::bad_alloc();
    }

    explicit Doc(yyjson_mut_doc* p) : ptr(p) {
        if (!ptr)
            throw std::bad_alloc();
    }

    ~Doc() {
        if (ptr)
            yyjson_mut_doc_free(ptr);
    }

    Doc(const Doc&) = delete;
    Doc& operator=(const Doc&) = delete;
};

// ============================================================
// Forward declarations
// ============================================================

class Value;

template<class T, class Enable = void>
struct Converter;

template<class T>
struct StructTraits;

template<class T>
Value to_value(const T& value);

template<class T>
T from_json(const Value& value);

// ============================================================
// Value
// ============================================================

class Value {
    std::shared_ptr<Doc> doc_;
    yyjson_mut_val* root_ = nullptr;

    Value(std::shared_ptr<Doc> doc, yyjson_mut_val* root)
        : doc_(std::move(doc)), root_(root) {
        if (!doc_ || !root_)
            throw error("Invalid JSON value");
    }

    static void require(bool condition, const char* message) {
        if (!condition)
            throw error(message);
    }

    yyjson_mut_val* clone_into(const Value& child) const {
        auto* copy =
            yyjson_mut_val_mut_copy(doc_->ptr, child.root_);

        if (!copy)
            throw std::bad_alloc();

        return copy;
    }

    template<class Fn>
    void for_each_member(Fn&& fn) const {
        std::size_t idx = 0;
        std::size_t max = 0;
        yyjson_mut_val* key = nullptr;
        yyjson_mut_val* item = nullptr;
        yyjson_mut_obj_foreach(root_, idx, max, key, item) {
            fn(Value(doc_, key), Value(doc_, item));
        }
    }

    friend struct Converter<Value>;

    template<class T, class Enable>
    friend struct Converter;

    template<class T>
    friend T from_json(const Value&);

public:
    Value(const Value&) = default;
    Value(Value&&) noexcept = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) noexcept = default;

    // A standalone JSON null value.
    Value() : doc_(std::make_shared<Doc>()) {
        root_ = yyjson_mut_null(doc_->ptr);

        if (!root_)
            throw std::bad_alloc();

        yyjson_mut_doc_set_root(doc_->ptr, root_);
    }

    static Value null() {
        return Value();
    }

    static Value object() {
        auto doc = std::make_shared<Doc>();
        auto* root = yyjson_mut_obj(doc->ptr);

        if (!root)
            throw std::bad_alloc();

        yyjson_mut_doc_set_root(doc->ptr, root);
        return Value(std::move(doc), root);
    }

    static Value array() {
        auto doc = std::make_shared<Doc>();
        auto* root = yyjson_mut_arr(doc->ptr);

        if (!root)
            throw std::bad_alloc();

        yyjson_mut_doc_set_root(doc->ptr, root);
        return Value(std::move(doc), root);
    }

    static Value parse(
        const std::string& text, read_flag flags = read_flag::none) {
        yyjson_read_err err{};

        yyjson_doc* parsed =
            yyjson_read_opts(
                const_cast<char*>(text.data()),
                text.size(),
                static_cast<yyjson_read_flag>(flags),
                nullptr,
                &err);

        if (!parsed) {
            throw parse_error(
                std::string("JSON parse error: ") +
                (err.msg ? err.msg : "unknown error"));
        }

        yyjson_mut_doc* mutable_doc =
            yyjson_doc_mut_copy(parsed, nullptr);

        yyjson_doc_free(parsed);

        if (!mutable_doc)
            throw std::bad_alloc();

        std::shared_ptr<Doc> doc;

        try {
            doc = std::make_shared<Doc>(mutable_doc);
        } catch (...) {
            yyjson_mut_doc_free(mutable_doc);
            throw;
        }

        auto* root = yyjson_mut_doc_get_root(doc->ptr);

        if (!root)
            throw parse_error("Parsed JSON has no root");

        return Value(std::move(doc), root);
    }

    bool is_null() const {
        return yyjson_mut_is_null(root_);
    }

    bool is_object() const {
        return yyjson_mut_is_obj(root_);
    }

    bool is_array() const {
        return yyjson_mut_is_arr(root_);
    }

    bool is_string() const {
        return yyjson_mut_is_str(root_);
    }

    bool is_number() const {
        return yyjson_mut_is_num(root_);
    }

    bool is_bool() const {
        return yyjson_mut_is_bool(root_);
    }

    std::size_t size() const {
        if (is_object())
            return yyjson_mut_obj_size(root_);

        if (is_array())
            return yyjson_mut_arr_size(root_);

        return 0;
    }

    bool has_key(const std::string& key) const {
        if (!is_object())
            return false;

        return yyjson_mut_obj_get(root_, key.c_str()) != nullptr;
    }

    Value operator[](const std::string& key) const {
        require(is_object(), "operator[] requires a JSON object");

        auto* item = yyjson_mut_obj_get(root_, key.c_str());

        if (!item)
            throw missing_key("Missing JSON key: " + key);

        return Value(doc_, item);
    }

    Value operator[](std::size_t index) const {
        require(is_array(), "operator[] requires a JSON array");

        auto* item = yyjson_mut_arr_get(root_, index);

        if (!item)
            throw std::out_of_range("JSON array index out of range");

        return Value(doc_, item);
    }

    // Insert or replace an object member.
    // An empty std::optional removes the key.
    template<class T>
    Value& set(const std::string& key, const T& value) {
        require(is_object(), "set() requires a JSON object");

        if constexpr (is_optional_v<T>) {
            if (!value.has_value()) {
                if (yyjson_mut_obj_get(root_, key.c_str()))
                    yyjson_mut_obj_remove_key(root_, key.c_str());
                return *this;
            }

            return set(key, *value);
        }

        Value child = to_value(value);

        if (yyjson_mut_obj_get(root_, key.c_str()))
            yyjson_mut_obj_remove_key(root_, key.c_str());

        auto* key_value =
            yyjson_mut_strncpy(doc_->ptr, key.data(), key.size());

        if (!key_value)
            throw std::bad_alloc();

        auto* child_value = clone_into(child);

        if (!yyjson_mut_obj_add(root_, key_value, child_value))
            throw error("Failed to add JSON object member");

        return *this;
    }

    // Convenient support for C strings and string literals.
    Value& set(const std::string& key, const char* value) {
        if (!value)
            return set(key, nullptr);

        return set(key, std::string(value));
    }

    // Append an item to an array.
    template<class T>
    Value& push(const T& value) {
        require(is_array(), "push() requires a JSON array");

        Value child = to_value(value);
        auto* child_value = clone_into(child);

        if (!yyjson_mut_arr_append(root_, child_value))
            throw error("Failed to append JSON array item");

        return *this;
    }

    // Read an array item as a typed value.
    template<class T>
    T at(std::size_t index) const {
        require(is_array(), "at() requires a JSON array");

        auto* item = yyjson_mut_arr_get(root_, index);

        if (!item)
            throw std::out_of_range("JSON array index out of range");

        Value view(doc_, item);
        return from_json<T>(view);
    }

    // Read an object member as a typed value.
    // std::optional<T> is nullopt when the key is missing or the value is null.
    template<class T>
    T get(const std::string& key) const {
        require(is_object(), "get(key) requires a JSON object");

        auto* item = yyjson_mut_obj_get(root_, key.c_str());

        if (!item) {
            if constexpr (is_optional_v<T>)
                return std::nullopt;
            else
                throw missing_key("Missing JSON key: " + key);
        }

        Value view(doc_, item);
        return from_json<T>(view);
    }

    // nullopt only when the key is missing. A present value is decoded as T.
    template<class T>
    std::optional<T> try_get(const std::string& key) const {
        require(is_object(), "try_get(key) requires a JSON object");

        auto* item = yyjson_mut_obj_get(root_, key.c_str());

        if (!item)
            return std::nullopt;

        Value view(doc_, item);
        return from_json<T>(view);
    }

    // Convert this value to a C++ type.
    template<class T>
    T get() const {
        return from_json<T>(*this);
    }

    template<class T, std::size_t N>
    void read_to(T (&destination)[N]) const {
        if (!is_array() || size() != N)
            throw type_error(
                "Expected JSON array of the requested length");

        for (std::size_t i = 0; i < N; ++i)
            destination[i] = at<T>(i);
    }

    yyjson_mut_val* mut_val() const noexcept {
        return root_;
    }

    yyjson_mut_doc* mut_doc() const noexcept {
        return doc_ ? doc_->ptr : nullptr;
    }

    std::string dump(write_flag flags) const {
        size_t length = 0;

        char* output = yyjson_mut_val_write(
            root_, static_cast<yyjson_write_flag>(flags), &length);

        if (!output)
            throw error("JSON serialization failed");

        try {
            std::string result(output, length);
            std::free(output);
            return result;
        } catch (...) {
            std::free(output);
            throw;
        }
    }

    std::string dump(bool pretty = false) const {
        return dump(pretty ? write_flag::pretty : write_flag::compact);
    }
};

// ============================================================
// Converter declarations and built-in conversions
// ============================================================

template<class T, class Enable>
struct Converter {
    static_assert(
        !std::is_same_v<T, T>,
        "No jw::Converter available for this type. "
        "Use JW_JSON_FIELDS or define a Converter specialization.");
};

// Strings
template<>
struct Converter<std::string> {
    static Value encode(const std::string& value) {
        auto doc = std::make_shared<Doc>();

        auto* root = yyjson_mut_strncpy(
            doc->ptr, value.data(), value.size());

        if (!root)
            throw std::bad_alloc();

        yyjson_mut_doc_set_root(doc->ptr, root);
        return Value(std::move(doc), root);
    }

    static std::string decode(const Value& value) {
        if (!value.is_string())
            throw type_error("Expected JSON string");

        return std::string(
            yyjson_mut_get_str(value.root_),
            yyjson_mut_get_len(value.root_));
    }
};

// C string pointers
template<>
struct Converter<const char*> {
    static Value encode(const char* value) {
        if (!value)
            return Value::null();

        return Converter<std::string>::encode(std::string(value));
    }
};

// Booleans
template<>
struct Converter<bool> {
    static Value encode(bool value) {
        auto doc = std::make_shared<Doc>();
        auto* root = yyjson_mut_bool(doc->ptr, value);

        if (!root)
            throw std::bad_alloc();

        yyjson_mut_doc_set_root(doc->ptr, root);
        return Value(std::move(doc), root);
    }

    static bool decode(const Value& value) {
        if (!value.is_bool())
            throw type_error("Expected JSON boolean");

        return yyjson_mut_get_bool(value.root_);
    }
};

// Integral types
template<class T>
struct Converter<T, std::enable_if_t<
    std::is_integral_v<T> &&
    !std::is_same_v<T, bool>>> {

    static Value encode(T value) {
        auto doc = std::make_shared<Doc>();
        yyjson_mut_val* root = nullptr;

        if constexpr (std::is_signed_v<T>) {
            root = yyjson_mut_sint(
                doc->ptr, static_cast<int64_t>(value));
        } else {
            root = yyjson_mut_uint(
                doc->ptr, static_cast<uint64_t>(value));
        }

        if (!root)
            throw std::bad_alloc();

        yyjson_mut_doc_set_root(doc->ptr, root);
        return Value(std::move(doc), root);
    }

    static T decode(const Value& value) {
        if (!value.is_number() ||
            !yyjson_mut_is_int(value.root_)) {
            throw type_error("Expected JSON integer");
        }

        // Positive integers are stored as uint64, including values above
        // int64 max. Negatives are stored as int64.
        if (yyjson_mut_is_sint(value.root_)) {
            const int64_t n = yyjson_mut_get_sint(value.root_);

            if constexpr (std::is_signed_v<T>) {
                if (n < static_cast<int64_t>(
                            std::numeric_limits<T>::lowest()) ||
                    n > static_cast<int64_t>(
                            std::numeric_limits<T>::max())) {
                    throw std::out_of_range(
                        "JSON integer out of range");
                }

                return static_cast<T>(n);
            }

            if (n < 0) {
                throw std::out_of_range(
                    "JSON unsigned integer out of range");
            }

            const auto positive = static_cast<uint64_t>(n);
            if (positive > static_cast<uint64_t>(
                               std::numeric_limits<T>::max())) {
                throw std::out_of_range(
                    "JSON unsigned integer out of range");
            }

            return static_cast<T>(positive);
        }

        const uint64_t n = yyjson_mut_get_uint(value.root_);

        if constexpr (std::is_signed_v<T>) {
            if (n > static_cast<uint64_t>(
                        std::numeric_limits<T>::max())) {
                throw std::out_of_range(
                    "JSON integer out of range");
            }

            return static_cast<T>(n);
        }

        if (n > static_cast<uint64_t>(
                    std::numeric_limits<T>::max())) {
            throw std::out_of_range(
                "JSON unsigned integer out of range");
        }

        return static_cast<T>(n);
    }
};

// Floating-point types
template<class T>
struct Converter<T, std::enable_if_t<
    std::is_floating_point_v<T>>> {

    static Value encode(T value) {
        if (!std::isfinite(value))
            throw error("JSON cannot represent NaN or infinity");

        auto doc = std::make_shared<Doc>();
        auto* root = yyjson_mut_real(
            doc->ptr, static_cast<double>(value));

        if (!root)
            throw std::bad_alloc();

        yyjson_mut_doc_set_root(doc->ptr, root);
        return Value(std::move(doc), root);
    }

    static T decode(const Value& value) {
        if (!value.is_number())
            throw type_error("Expected JSON number");

        const double n = yyjson_mut_get_num(value.root_);

        if (!std::isfinite(n) ||
            n < -static_cast<double>(
                    std::numeric_limits<T>::max()) ||
            n > static_cast<double>(
                    std::numeric_limits<T>::max())) {
            throw std::out_of_range(
                "JSON floating-point value out of range");
        }

        return static_cast<T>(n);
    }
};

// Null pointer -> JSON null
template<>
struct Converter<std::nullptr_t> {
    static Value encode(std::nullptr_t) {
        return Value::null();
    }

    static std::nullptr_t decode(const Value& value) {
        if (!value.is_null())
            throw type_error("Expected JSON null");

        return nullptr;
    }
};

// Value itself
template<>
struct Converter<Value> {
    static Value encode(const Value& value) {
        return value;
    }

    static Value decode(const Value& value) {
        return value;
    }
};

// Vectors
template<class T>
struct Converter<std::vector<T>> {
    static Value encode(const std::vector<T>& items) {
        Value result = Value::array();

        for (const auto& item : items)
            result.push(item);

        return result;
    }

    static std::vector<T> decode(const Value& value) {
        if (!value.is_array())
            throw type_error("Expected JSON array");

        std::vector<T> result;
        result.reserve(value.size());

        for (std::size_t i = 0; i < value.size(); ++i)
            result.push_back(value.at<T>(i));

        return result;
    }
};

// Optional values. Empty encodes as JSON null.
// Object fields omit an empty optional through Value::set.
template<class T>
struct Converter<std::optional<T>> {
    static Value encode(const std::optional<T>& value) {
        if (!value.has_value())
            return Value::null();

        return to_value(*value);
    }

    static std::optional<T> decode(const Value& value) {
        if (value.is_null())
            return std::nullopt;

        return from_json<T>(value);
    }
};

namespace detail {

template<class K>
std::string object_key(const K& key) {
    Value encoded = to_value(key);

    if (!encoded.is_string())
        throw type_error("JSON object key must be a string");

    return from_json<std::string>(encoded);
}

} // namespace detail

template<class K, class V>
struct Converter<std::map<K, V>> {
    static Value encode(const std::map<K, V>& items) {
        Value result = Value::object();

        for (const auto& [key, item] : items)
            result.set(detail::object_key(key), item);

        return result;
    }

    static std::map<K, V> decode(const Value& value) {
        if (!value.is_object())
            throw type_error("Expected JSON object");

        std::map<K, V> result;
        value.for_each_member([&](Value key, Value item) {
            result.emplace(from_json<K>(key), from_json<V>(item));
        });
        return result;
    }
};

template<class K, class V>
struct Converter<std::vector<std::pair<K, V>>> {
    static Value encode(const std::vector<std::pair<K, V>>& items) {
        Value result = Value::object();

        for (const auto& [key, item] : items)
            result.set(detail::object_key(key), item);

        return result;
    }

    static std::vector<std::pair<K, V>> decode(const Value& value) {
        if (!value.is_object())
            throw type_error("Expected JSON object");

        std::vector<std::pair<K, V>> result;
        result.reserve(value.size());
        value.for_each_member([&](Value key, Value item) {
            result.emplace_back(from_json<K>(key), from_json<V>(item));
        });
        return result;
    }
};

template<class T, std::size_t N>
struct Converter<std::array<T, N>> {
    static Value encode(const std::array<T, N>& items) {
        Value result = Value::array();

        for (const auto& item : items)
            result.push(item);

        return result;
    }

    static std::array<T, N> decode(const Value& value) {
        if (!value.is_array() || value.size() != N)
            throw type_error(
                "Expected JSON array of the requested length");

        std::array<T, N> result{};

        for (std::size_t i = 0; i < N; ++i)
            result[i] = value.at<T>(i);

        return result;
    }
};

// C arrays can be encoded. Read them with Value::read_to.
template<class T, std::size_t N>
struct Converter<T[N]> {
    static Value encode(const T (&items)[N]) {
        Value result = Value::array();

        for (std::size_t i = 0; i < N; ++i)
            result.push(items[i]);

        return result;
    }
};

// ============================================================
// Generic conversion entry points
// ============================================================

template<class T>
Value to_value(const T& value) {
    if constexpr (std::is_array_v<T>)
        return Converter<T>::encode(value);
    else
        return Converter<std::decay_t<T>>::encode(value);
}

template<class T>
T from_json(const Value& value) {
    return Converter<T>::decode(value);
}

inline Value object() {
    return Value::object();
}

inline Value array() {
    return Value::array();
}

inline Value parse(
    const std::string& text, read_flag flags = read_flag::none) {
    return Value::parse(text, flags);
}

inline std::string stringify(
    const Value& value, bool pretty = false) {
    return value.dump(pretty);
}

inline std::string stringify(const Value& value, write_flag flags) {
    return value.dump(flags);
}

// ============================================================
// Struct field registration
// ============================================================

template<class T, class = void>
struct has_struct_traits : std::false_type {};

template<class T>
struct has_struct_traits<T, std::void_t<
    decltype(StructTraits<T>::fields())
>> : std::true_type {};

// Encode a registered struct.
template<class T, class Tuple, std::size_t... I>
Value encode_struct_fields(
    const T& object,
    const Tuple& fields,
    std::index_sequence<I...>) {

    Value result = Value::object();

    (result.set(
        std::get<I>(fields).first,
        object.*(std::get<I>(fields).second)
    ), ...);

    return result;
}

// Decode a registered struct.
template<class T, class Tuple, std::size_t... I>
T decode_struct_fields(
    const Value& value,
    const Tuple& fields,
    std::index_sequence<I...>) {

    T result{};

    ((
        result.*(std::get<I>(fields).second) =
            value.get<std::remove_cv_t<
                std::remove_reference_t<
                    decltype(
                        result.*(std::get<I>(fields).second))
                >
            >>(std::get<I>(fields).first)
    ), ...);

    return result;
}

// Registered struct converter.
template<class T>
struct Converter<T, std::enable_if_t<
    has_struct_traits<T>::value>> {

    static Value encode(const T& object) {
        constexpr std::size_t count = std::tuple_size_v<
            decltype(StructTraits<T>::fields())>;

        return encode_struct_fields(
            object,
            StructTraits<T>::fields(),
            std::make_index_sequence<count>{});
    }

    static T decode(const Value& value) {
        if (!value.is_object())
            throw type_error(
                "Expected JSON object for registered struct");

        constexpr std::size_t count = std::tuple_size_v<
            decltype(StructTraits<T>::fields())>;

        return decode_struct_fields<T>(
            value,
            StructTraits<T>::fields(),
            std::make_index_sequence<count>{});
    }
};

} // namespace jw

// ============================================================
// JW_JSON_FIELDS macro (up to 8 fields)
// Invoke at global namespace scope.
// ============================================================

#define JW_DETAIL_FIELD(TYPE, FIELD) \
    std::make_pair(#FIELD, &TYPE::FIELD)

#define JW_DETAIL_FIELDS_1(T, a) \
    JW_DETAIL_FIELD(T, a)

#define JW_DETAIL_FIELDS_2(T, a, b) \
    JW_DETAIL_FIELDS_1(T, a), JW_DETAIL_FIELD(T, b)

#define JW_DETAIL_FIELDS_3(T, a, b, c) \
    JW_DETAIL_FIELDS_2(T, a, b), JW_DETAIL_FIELD(T, c)

#define JW_DETAIL_FIELDS_4(T, a, b, c, d) \
    JW_DETAIL_FIELDS_3(T, a, b, c), JW_DETAIL_FIELD(T, d)

#define JW_DETAIL_FIELDS_5(T, a, b, c, d, e) \
    JW_DETAIL_FIELDS_4(T, a, b, c, d), JW_DETAIL_FIELD(T, e)

#define JW_DETAIL_FIELDS_6(T, a, b, c, d, e, f) \
    JW_DETAIL_FIELDS_5(T, a, b, c, d, e), JW_DETAIL_FIELD(T, f)

#define JW_DETAIL_FIELDS_7(T, a, b, c, d, e, f, g) \
    JW_DETAIL_FIELDS_6(T, a, b, c, d, e, f), \
    JW_DETAIL_FIELD(T, g)

#define JW_DETAIL_FIELDS_8(T, a, b, c, d, e, f, g, h) \
    JW_DETAIL_FIELDS_7(T, a, b, c, d, e, f, g), \
    JW_DETAIL_FIELD(T, h)

#define JW_DETAIL_GET_MACRO( \
    _1,_2,_3,_4,_5,_6,_7,_8,NAME,...) NAME

#define JW_DETAIL_FIELDS(T, ...) \
    JW_DETAIL_GET_MACRO(__VA_ARGS__, \
        JW_DETAIL_FIELDS_8, \
        JW_DETAIL_FIELDS_7, \
        JW_DETAIL_FIELDS_6, \
        JW_DETAIL_FIELDS_5, \
        JW_DETAIL_FIELDS_4, \
        JW_DETAIL_FIELDS_3, \
        JW_DETAIL_FIELDS_2, \
        JW_DETAIL_FIELDS_1, \
        extra)(T, __VA_ARGS__)

#define JW_JSON_FIELDS(TYPE, ...) \
    namespace jw { \
    template<> struct StructTraits<TYPE> { \
        static constexpr auto fields() { \
            return std::make_tuple( \
                JW_DETAIL_FIELDS(TYPE, __VA_ARGS__)); \
        } \
    }; \
    }
