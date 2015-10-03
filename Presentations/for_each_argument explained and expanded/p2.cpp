// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <iostream>
#include <vector>
#include <type_traits>

// Here's our `forArgs` function without any comment.
// Doesn't look so scary anymore, does it?

template<typename TF, typename... Ts>
void forArgs(TF&& mFn, Ts&&... mArgs)
{
    return (void) std::initializer_list<int>
    {
        (
            mFn(std::forward<Ts>(mArgs)),
            0
        )...
    };
}

// So, what is `forArgs` useful for?
// Let's see some potential use cases for it.

// ------------------------------------------------------------------

// Example use case: `make_vector` function.

// `make_vector` will take one or more arguments and return an
// `std::vector` containing them.

template<typename... TArgs>
auto make_vector(TArgs&&... mArgs)
{
    // First problem: what type should we return?
    // We need to deduce a type suitable for all the passed arguments.

    // `std::common_type_t` comes into play here.

    // Note:
    //
    //    * C++14 added many versions of commonly used
    //      type traits functions ending with "_t", that
    //      do not require the user to write `typename`
    //      at the beginning and `::type` at the end.
    //
    //    * C++11:
    //      using IntPtr = typename std::add_pointer<int>::type;
    //
    //    * C++14:
    //      using IntPtr = std::add_pointer_t<int>;
    //

    // `std::common_type_t` determines the common type among all
    // passed types - that is the type all passed types can be
    // implicitly converted to.

    // Computing the correct return type is now easy:
    using VectorItem = std::common_type_t<TArgs...>;
    std::vector<VectorItem> result;

    // We also know how many items we're going to put into the vector.
    // As a minor optimization, let's reserve memory beforehand.
    result.reserve(sizeof...(TArgs));

    // Now we use `forArgs` to generate the code that emplaces the
    // items into our vector:
    forArgs
    (
        // Our lambda needs to capture the vector and use a
        // "forwarding reference" to correctly forward the passed
        // argument to `std::vector::emplace_back`.
        [&result](auto&& x)
        {
            // We do not know the type of `x` - but we can retrieve
            // it using `decltype(x)`.
            result.emplace_back(std::forward<decltype(x)>(x));
        },

        std::forward<TArgs>(mArgs)...
    );

    return result;
}

int main()
{
    // Deduced as `std::vector<int>`;
    auto v0(make_vector(1, 2, 3, 4, 5));

    // This is roughly equivalent to writing:
    /*
        std::vector<int> result;
        result.reserve(5);

        result.emplace_back(1);
        result.emplace_back(2);
        result.emplace_back(3);
        result.emplace_back(4);
        result.emplace_back(5);

        return result;
    */

    static_assert(std::is_same<decltype(v0),
        std::vector<int>>(), "");

    // Prints "12345".
    for(const auto& x : v0) std::cout << x;
    std::cout << "\n";



    // Deduced as `std::vector<const char*>`;
    auto v1(make_vector("hello", " ", "everyone!"));

    static_assert(std::is_same<decltype(v1),
        std::vector<const char*>>(), "");

    // Prints "hello everyone!".
    for(const auto& x : v1) std::cout << x;
    std::cout << "\n";



    // Deduced as `std::vector<std::string>`.
    // This happens because `const char*` can be implicitly
    // converted to `std::string`, but not vice versa.
    auto v2(make_vector("hello", " ", std::string{"world"}));

    static_assert(std::is_same<decltype(v2),
        std::vector<std::string>>(), "");

    // Prints "hello world".
    for(const auto& x : v2) std::cout << x;
    std::cout << "\n";

    return 0;
}

// Before moving on, let's digress to cover a very useful C++14
// feature: compile-time integer sequences.

// This feature will be heavily used in the next code segments, to
// implement a version of `forArgs` with generic arity.