// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <iostream>
#include <tuple>

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

// This code segments shows another interesting use case: iteration
// over `std::tuple` elements.

// Example use case: `forTuple` function.

// We can use `forArgs` as a building block for an `std::tuple`
// element iteration function.

// To do so, we require an helper function that expands the elements
// of the `std::tuple` into a function call.

// The following helper function was written roughly following paper
// N3802, proposed by Peter Sommerlad.

// You can find the paper at the following address:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3802.pdf

// Many similar implementations of the same function can be easily
// found online.

// `apply` is also available in the `std::experimental` namespace.
// http://en.cppreference.com/w/cpp/experimental/apply

// ------------------------------------------------------------------

// `apply` is composed by an `applyImpl` implementation function and
// an `apply` interface function.

// Let's start by defining the "impl" function.
// Here we see another way of matching `index_sequence` instances,
// using anonymous function arguments.
template<typename TF, typename TTpl, std::size_t... TIs>
decltype(auto) applyImpl(TF&& mFn, TTpl&& mTpl,
    std::index_sequence<TIs...>)
{
    // The `applyImpl` function calls `mFn` once, expanding the
    // contents of the `mTpl` tuple as the function parameters.
    return std::forward<TF>(mFn)
    (
        std::get<TIs>
        (
            std::forward<TTpl>(mTpl)
        )...
    );
}

// Let's now define the "interface" function that will be called by
// the user.

// The first parameter will be a callable object, that will be invoked
// using the tuple's contents. The second parameter will be the tuple.
template<typename TF, typename TTpl>
decltype(auto) apply(TF&& mFn, TTpl&& mTpl)
{
    // `applyImpl` requires an index sequence that goes from `0` to
    // the number of tuple items.
    // We can build that using `std::make_index_sequence` and
    // `std::tuple_size`.
    using Indices = std::make_index_sequence
    <
        std::tuple_size
        <
            // We use `std::decay_t` here to get rid of the reference.
            std::decay_t<TTpl>
        >{}
    >;

    return applyImpl(std::forward<TF>(mFn), std::forward<TTpl>(mTpl),
        Indices{});
}

// ------------------------------------------------------------------

// `forTuple` is a function that takes a callable object and an
// `std::tuple` as its parameters.

// It then calls the passed function individually passing every
// element of the tuple as its argument.

template<typename TFn, typename TTpl>
void forTuple(TFn&& mFn, TTpl&& mTpl)
{
    // We basically expand the tuple into a function call to a
    // variadic polymorphic lambda with `apply`, which in turn passes
    // the expanded tuple elements to `forArgs`, one by one...
    // ...which in turn calls `mFn` with every single tuple element
    // individually.

    apply
    (
        // The callable object we will pass to `apply` is a generic
        // variadic lambda that forwards its arguments to `forArgs`.
        [&mFn](auto&&... xs)
        {
            // The `xs...` parameter pack contains the `mTpl` tuple
            // elements, expanded thanks to `apply`.

            // We will call the `mFn` unary function for each expanded
            // tuple element, thanks to `forArgs`.
            forArgs
            (
                mFn,
                std::forward<decltype(xs)>(xs)...
            );
        },

        std::forward<TTpl>(mTpl)
    );
}

int main()
{
    // Prints "10 hello 15 c".
    forTuple
    (
        [](const auto& x){ std::cout << x << " "; },
        std::make_tuple(10, "hello", 15.f, 'c')
    );

    // This is roughly equivalent to writing:
    /*
        forArgs
        (
            [](const auto& x){ std::cout << x << " "; },

            10,
            "hello",
            15.f,
            'c'
        );

        // ...which, in turn, is roughly equivalent to:

        std::cout << 10 << " ";
        std::cout << "hello" << " ";
        std::cout << "15.f" << " ";
        std::cout << 'c' << " ";
    */

    std::cout << "\n";
    return 0;
}

// All of this is very useful - but limited to unary functions.

// What if we want to take arguments two by two?
// Or three by three?

// It is actually possible to create a generic version of `forArgs`
// that takes the arity of the passed callable object as a template
// parameter.

// Let's see a possible implementation in the next code segment.