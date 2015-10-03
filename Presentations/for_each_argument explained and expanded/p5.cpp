// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <utility>
#include <iostream>
#include <tuple>

// In this code segment we're going to implement a generic `forNArgs`
// function that passes its arguments to a callable object in groups
// of `N` arguments, where `N` is divisible by the number of total
// arguments.

// The following implementation was originally written by Daniel Frey.

// It's a very clever implementation that avoids compile-time
// recursion (which can dramatically increase compilation time),
// written as an answer to one of my questions on StackOverflow:
// http://stackoverflow.com/questions/29900928

// ------------------------------------------------------------------

// Forward declaration of the implementation function.
// We will match two index sequences in `forNArgsImpl`.
template<typename, typename>
struct forNArgsImpl;

// Main `forNArgs` function - this will be called by the user
// and generate the required index sequences.
// `TArity` will be the number of parameters passed at once to a
// single `mFn` function call.
// `Ts...` is the parameter pack containing all passed arguments.
template<std::size_t TArity, typename TF, typename... Ts>
void forNArgs(TF&& mFn, Ts&&... mXs)
{
    // The total number of arguments can be retrieved with the
    // `sizeof...` operator.
    constexpr auto numberOfArgs(sizeof...(Ts));

    // The number of passed arguments must be divisible by `N`.
    static_assert(numberOfArgs % TArity == 0,
        "Invalid number of arguments");

    // Call the implementation function with...
    forNArgsImpl
    <
        // ...a sequence from `0` to the number of `mFn` calls that
        // will be executed.
        // (`numberOfArgs` divided by `TArity`)
        std::make_index_sequence<numberOfArgs / TArity>,

        // ...a sequence from `0` to `TArity`.
        // ("size of a group of arguments")
        std::make_index_sequence<TArity>
    >
    ::exec
    (
        // Pass the callable object to the implementation.
        mFn,

        // Forward the passed arguments as an `std::tuple`.
        std::forward_as_tuple(std::forward<Ts>(mXs)...)
    );
}

// Specialization of the implementation function that matches the
// generated index sequences into two `std::size_t` packs.
template<std::size_t... TNCalls, std::size_t... TNArity>
struct forNArgsImpl
<
    // `TNCalls...` goes from `0` to the number of function calls.
    // (`numberOfArgs` divided by `TArity`)
    std::index_sequence<TNCalls...>,

    // `TNArity...` goes from `0` to the number of arguments per
    // function call (`TArity`).
    std::index_sequence<TNArity...>
>
{
    template<typename TF, typename... Ts>
    static void exec(TF&& mFn, const std::tuple<Ts...>& mXs)
    {
        // We can retrieve the arity again using the `sizeof...`
        // operator on the `TNArity` index sequence.
        constexpr auto arity(sizeof...(TNArity));

        // `swallow` is a nice and readable way of creating a
        // context for parameter expansion, like `initializer_list`.
        using swallow = bool[];

        // We'll roughly use the same idea behind `forArgs` here.

        (void) swallow
        {
            // `TNCalls...` is the sequence we are expanding here.

            // The code inside `swallow` gets expanded to the number
            // of function calls previously calculated.
            (execN<TNCalls * arity>(mFn, mXs), true)...
        };

        // Example expansion of the above context for a binary
        // function called with 4 arguments:
        /*
            auto fn([](auto x, auto y){ return x + y; });

            forNArgs<2>
            (
                fn,

                0,
                10,

                20,
                30
            );

            (void) swallow
            {
                (execN<0 * 2>(fn, TUPLE{0, 10, 20, 30}), true),
                (execN<1 * 2>(fn, TUPLE{0, 10, 20, 30}), true)
            };
        */
    }

    // `execN` simply calls the function getting the correct elements
    // from the tuple containing the forwarded arguments.
    template<std::size_t TNBase, typename TF, typename... Ts>
    static void execN(TF&& mFn, const std::tuple<Ts...>& mXs)
    {
        // `TNBase` is the base index of the tuple elements we're
        // going to get.

        // `Cs...` gets expanded from 0 to the number of arguments
        // per function call (`N`).
        mFn
        (
            std::get<TNBase + TNArity>(mXs)...
        );

        // Example expansion of `execN` for the previous binary
        // function example called with 4 arguments:
        /*
            auto fn([](auto x, auto y){ return x + y; });

            forNArgs<2>
            (
                fn,

                0,
                10,

                20,
                30
            );

            (execN<0 * 2>(fn, TUPLE{0, 10, 20, 30}), true)
            // ...expands to...
            fn
            (
                std::get<0 + 0>(TUPLE{0, 10, 20, 30}),
                std::get<0 + 1>(TUPLE{0, 10, 20, 30})
            );
            // ...expands to...
            fn
            (
                0,
                10
            );

            (execN<1 * 2>(fn, TUPLE{0, 10, 20, 30}), true)
            // ...expands to...
            fn
            (
                std::get<2 + 0>(TUPLE{0, 10, 20, 30}),
                std::get<2 + 1>(TUPLE{0, 10, 20, 30})
            );
            // ...expands to...
            fn
            (
                20,
                30
            );
        */
    }
};

int main()
{
    // Prints "2 4 6 8".
    forNArgs<2>
    (
        [](auto x, auto y)
        {
            std::cout << x * y << " ";
        },

        // 2 * 1 = 2
        2, 1,

        // 2 * 2 = 4
        2, 2,

        // 2 * 3 = 6
        2, 3,

        // 2 * 4 = 8
        2, 4
    );

    std::cout << "\n";

    // Prints "3 6 9 12".
    forNArgs<3>
    (
        [](auto x, auto y, auto z)
        {
            std::cout << x + y + z << " ";
        },

        // 1 + 1 + 1 = 3
        1, 1, 1,

        // 2 + 2 + 2 = 6
        2, 2, 2,

        // 3 + 3 + 3 = 9
        3, 3, 3,

        // 4 + 4 + 4 = 12
        4, 4, 4
    );

    std::cout << "\n";
    return 0;
}

// Interesting, isn't it?
// Let's see a possible use case in the next code segment.