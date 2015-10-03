// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <utility>
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

// When writing template code, having contiguous compile-time integer
// sequences is often very useful.

// C++14 introduced `integer_sequence`, in the `<utility>` header.

// Positive integer sequences using `std::size_t` as their underlying
// index type are called "index sequences".
// They can be generated using `std::make_index_sequence`.

// `Seq0` is a compile-time index sequence.
// It goes from `0` (inclusive) to `10` (non-inclusive).
// You can think of `Seq0` as a compile-time list that wraps a
// sequence of integers in a single type.
using Seq0 = std::make_index_sequence<10>;

// To retrieve the numbers in the sequence, we must match it using
// template specializations and expand it with `...`.

// Let's forward-declare a `struct` that will print an index sequence
//to the standard output.
template<typename>
struct SeqPrinter;

// Let's now specialize it to match an index sequence:
template<std::size_t... TIs>
struct SeqPrinter<std::index_sequence<TIs...>>
{
    // And let's use our `forArgs` function to print the indices:
    static void print()
    {
        forArgs
        (
            [](auto x){ std::cout << x << " "; },

            // We can expand the matched indices here:
            TIs...
        );
    }
};

int main()
{
    // Let's try it out now.

    // Prints "0 1 2 3 4 5 6 7 8 9".
    SeqPrinter<Seq0>::print();
    std::cout << "\n";

    // Prints "0 1 2 3 4".
    SeqPrinter<std::make_index_sequence<5>>::print();
    std::cout << "\n";

    return 0;
}

// In the next code segment we'll implement a function that iterates
// over an `std::tuple`'s elements, using `forArgs` and integer
// sequences as building blocks.