// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <utility>
#include <iostream>
#include <tuple>
#include <unordered_map>

// We're going to implement a `make_unordered_map` function in this
// code segment, similar to the previous `make_vector`.

// As `std::unordered_map` is an associative container, we  will use
// `forNArgs<2>` to pass "key-value pairs".

// ------------------------------------------------------------------

template<typename, typename>
struct forNArgsImpl;

template<std::size_t TArity, typename TF, typename... Ts>
void forNArgs(TF&& mFn, Ts&&... mXs)
{
    constexpr auto numberOfArgs(sizeof...(Ts));

    static_assert(numberOfArgs % TArity == 0,
        "Invalid number of arguments");

    forNArgsImpl
    <
        std::make_index_sequence<numberOfArgs / TArity>,
        std::make_index_sequence<TArity>
    >
    ::exec
    (
        mFn,
        std::forward_as_tuple(std::forward<Ts>(mXs)...)
    );
}

template<std::size_t... TNCalls, std::size_t... TNArity>
struct forNArgsImpl
<
    std::index_sequence<TNCalls...>,
    std::index_sequence<TNArity...>
>
{
    template<typename TF, typename... Ts>
    static void exec(TF&& mFn, const std::tuple<Ts...>& mXs)
    {
        constexpr auto arity(sizeof...(TNArity));
        using swallow = bool[];

        (void) swallow
        {
            (execN<TNCalls * arity>(mFn, mXs), true)...
        };
    }

    template<std::size_t TNBase, typename TF, typename... Ts>
    static void execN(TF&& mFn, const std::tuple<Ts...>& mXs)
    {
        mFn
        (
            std::get<TNBase + TNArity>(mXs)...
        );
    }
};

// ------------------------------------------------------------------

// Example use case: `make_unordered_map` function.

// `make_unordered_map` will take arguments in groups of two and
// return an `std::unordered_map` instance having the first arguments
// of every group as keys and the second arguments of every group
// as values.

template<typename... TArgs>
auto make_unordered_map(TArgs&&... mArgs);

// ------------------------------------------------------------------

// Our first job is defining an helper that will allow us to deduce
// the common type for all keys and the common type for all values of
// the `std::unordered_map`.

// We're going to use C++11 index sequences to divide the passed
// parameter pack types in two different packs:

// (types)    |K|V|K|V|K|V|K|V|K|V| ...
// (K seq)    |0| |2| |4| |6| |8| | ...
// (V seq)    | |1| |3| |5| |7| |9| ...

// Let's forward-declare an helper struct that will do that for us.
// It's gonna take match an `std::index_sequence` that goes from `0`
// to `sizeof...(Ts) / 2` and will also take a variadic amount of
// types.
template<typename TSeq, typename... Ts>
struct CommonKVHelper;

// Our `CommonKVHelper` specialization will expand the index sequence:
template<std::size_t... TIs, typename... Ts>
struct CommonKVHelper
<
    std::index_sequence<TIs...>,
    Ts...
>
{
    // Let's make sure the number of variadic types is a multiple of
    // two.
    static_assert(sizeof...(Ts) % 2 == 0, "");

    // We need a way to get the type at a specific index from a
    // variadic type list. Fortunately, we can make use of
    // `std::tuple_element_t` to do that.

    // `std::tuple_element_t` takes two template parameters: an index
    // and a tuple type. It then "returns" the type of the tuple
    // element at that specific index.

    // Our `TypeAt` type alias will return the type at index `TI` from
    // the variadic `Ts...` type pack.
    template<std::size_t TI>
    using TypeAt = std::tuple_element_t<TI, std::tuple<Ts...>>;

    // To get the common type of the keys, we'll expand our index
    // sequence multiplying every number by two.
    using KeyType = std::common_type_t<TypeAt<TIs * 2>...>;

    // To get the common type of the values, we'll expand our index
    // sequence multiplying every number by two, adding one.
    using ValueType = std::common_type_t<TypeAt<(TIs * 2) + 1>...>;

    // Example expansion for 6 types:
    /*
        // (TIs...) = (0, 1, 2)

        using KeyType = std::common_type_t
        <
            TypeAt<0 * 2>, // TypeAt<0>
            TypeAt<1 * 2>, // TypeAt<2>
            TypeAt<2 * 2>  // TypeAt<4>
        >;

        using ValueType = std::common_type_t
        <
            TypeAt<(0 * 2) + 1>, // TypeAt<1>
            TypeAt<(1 * 2) + 1>, // TypeAt<3>
            TypeAt<(2 * 2) + 1>  // TypeAt<5>
        >;
    */
};

// We still need an additional helper type alias to generate the
// `std::index_sequence` from `0` to "half the number of types":
template<typename... Ts>
using HelperFor = CommonKVHelper
<
    std::make_index_sequence<sizeof...(Ts) / 2>,
    Ts...
>;

// The last thing we need to do is define two additional type aliases
// that will take our list of key and value types as inputs: one will
// return the common key type, the other one will return the common
// value type.

template<typename... Ts>
using CommonKeyType = typename HelperFor<Ts...>::KeyType;

template<typename... Ts>
using CommonValueType = typename HelperFor<Ts...>::ValueType;

// Let's use `static_assert` to make sure everything works.

static_assert(std::is_same
<
    CommonKeyType<std::string, int>,

    // Deduced key type:
    std::string
>(), "");

static_assert(std::is_same
<
    CommonValueType<std::string, int>,

    // Deduced value type:
    int
>(), "");

static_assert(std::is_same
<
    CommonKeyType
    <
        // Keys         // Values
        std::string,    int,
        std::string,    float,
        const char*,    long
    >,

    // Deduced key type:
    std::string
>(), "");

static_assert(std::is_same
<
    CommonValueType
    <
        // Keys         // Values
        std::string,    int,
        std::string,    float,
        const char*,    long
    >,

    // Deduced value type:
    float
>(), "");

// ------------------------------------------------------------------

// We can finally implement `make_unordered_map`:
template<typename... TArgs>
auto make_unordered_map(TArgs&&... mArgs)
{
    // Let's calculate and alias the common types:
    using KeyType = CommonKeyType<TArgs...>;
    using ValueType = CommonValueType<TArgs...>;

    // Let's instantiate an `std::unordered_map` with the correct
    // type and reserve memory for the passed elements:
    std::unordered_map<KeyType, ValueType> result;
    result.reserve(sizeof...(TArgs) / 2);

    // We can now use `forNArgs<2>` to pass elements two by two to a
    // lambda function that will emplace them as key-value pairs in
    // the `std::unordered_map`.

    forNArgs<2>
    (
        [&result](auto&& k, auto&& v)
        {
            result.emplace
            (
                std::forward<decltype(k)>(k),
                std::forward<decltype(v)>(v)
            );
        },

        std::forward<TArgs>(mArgs)...
    );

    return result;
}

int main()
{
    using namespace std::literals;

    auto m = make_unordered_map
    (
        "zero"s,    0,
        "one"s,     1,
        "two",      2.f
    );

    static_assert(std::is_same
    <
        decltype(m),
        std::unordered_map<std::string, float>
    >(), "");

    // Prints "012".
    std::cout << m["zero"] << m["one"] << m["two"];

    std::cout << "\n";
    return 0;
}

// This use case demonstrated the expressiveness of `make_x`-like
// functions, that automatically deduce the container type by the
// passed arguments.

// If there's enough time left, we'll take a look at an upcoming
// C++17 feature that will make the `std::initializer_list` "tricks"
// obsolete and allow much more terse and expressive variadic template
// metaprogramming: "fold expressions".