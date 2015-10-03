// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <iostream>

// Let's rewrite the code snippet in a more organized manner, using
// comments to make obvious what it is actually happening.

template<typename TF, typename... Ts>
void forArgs(TF&& mFn, Ts&&... mArgs)
{
    // We require a context where variadic parameter expansion can
    // take place, so that we can expand the function call with every
    // passed argument.

    // We can use an `std::initializer_list` to create such context,
    // but any "brace-initializable" container works, like C-style
    // arrays. (Examples: `bool[]`, `int[]`)

    // These contexts guarantee that the arguments will be evaluated
    // in the correct order, unlike a generic variadic lambda, for
    // example.

    return (void) std::initializer_list<int>
    {
        // Every element of the `initializer_list` is an expression
        // enclosed in round parenthesis.
        (
            // In the parenthesis, the result of the `mFn` function
            // call is followed by the comma operator and an integer
            // (zero in this case).

            // Thanks to the comma operator, the expression evaluates
            // to an (unused) integer, which is accepted by the
            // `initializer_list`.

            mFn
            (
                // As we're taking the variadic arguments by
                // "forwarding reference", it is important
                // to use `std::forward` to correctly forward
                // their reference types to `mFn`.
                std::forward<Ts>(mArgs)
            ),

            0
        )...
    };
}

// Still confused? Everything becomes clearer with an example
// expansion of a `forArgs` call:

/*

// The following `forArgs` call...

forArgs
(
    [](const auto& x){ std::cout << x << " "; },

    "hello",
    1,
    2,
    3
);

// ..roughly expands to...

(void) std::initializer_list<int>
{
    ([](const auto& x){ std::cout << x; }("hello"), 0),
    ([](const auto& x){ std::cout << x; }(1), 0),
    ([](const auto& x){ std::cout << x; }(2), 0),
    ([](const auto& x){ std::cout << x; }(3), 0)
}

// ...which is the same as writing...

std::cout << "hello";
std::cout << 1;
std::cout << 2;
std::cout << 3;

*/

int main()
{
    // Prints "hello123".
    forArgs
    (
        [](const auto& x){ std::cout << x; },

        "hello",
        1,
        2,
        3
    );

    std::cout << "\n";
    return 0;
}

// In the next code segment we'll take a look at an example use case.