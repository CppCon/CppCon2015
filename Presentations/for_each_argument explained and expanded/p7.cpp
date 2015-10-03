// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <utility>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <vector>

// "Fold expressions" are a new C++17 language construct that allow
// "folding" a parameter pack over a binary operator.
// http://en.cppreference.com/w/cpp/language/fold

// These expressions are extremely convenient when dealing with
// variadic template metaprogramming. Here's an example:

// (NOTE: g++ 5.2.0 does not compile this code segment properly.)

// ------------------------------------------------------------------

// Are all boolean values true? (C++14 implementation)

template<bool... Ts>
struct AllTrueCPP14 : std::true_type { };

template<bool T, bool... Ts>
struct AllTrueCPP14
<
    T, Ts...
> : std::integral_constant<bool, T && AllTrueCPP14<Ts...>{}> { };

static_assert(AllTrueCPP14<>{}, "");
static_assert(AllTrueCPP14<true, true, true>{}, "");
static_assert(!AllTrueCPP14<true, true, false>{}, "");
static_assert(!AllTrueCPP14<false, false, false>{}, "");


// ------------------------------------------------------------------

// Are all boolean values true? (C++17 implementation)

template<bool... Ts>
using AllTrueCPP17 = std::integral_constant<bool, (Ts && ...)>;

// (Ideally, we would use `bool_constant`, in a C++17-compliant
// standard library implementation.)

static_assert(AllTrueCPP17<>{}, "");
static_assert(AllTrueCPP17<true, true, true>{}, "");
static_assert(!AllTrueCPP17<true, true, false>{}, "");
static_assert(!AllTrueCPP17<false, false, false>{}, "");

// (Ts && ...) is a "fold expression".
// It basically expands every element in `Ts...`, interleaving them
// with the provided binary operator (`&&` in this case).

// ------------------------------------------------------------------

// "Fold expressions" make many uses of `forArgs` redundant.
// What if we want to print every argument?
// We can simply fold over the `<<` operator as such:
template<typename... Ts>
void printAll(Ts&&... mXs)
{
    (std::cout << ... << mXs) << '\n';
}

// Can we re-implement `forArgs` using "fold expressions"?
// Turns out we easily can, as the comma operator `,` is supported
// by them:
template<typename TF, typename... Ts>
void forArgs(TF&& mFn, Ts&&... mXs)
{
    (mFn(mXs), ...);
}

// How about `make_vector`?
template<typename... Ts>
auto make_vector(Ts&&... mXs)
{
    std::vector<std::common_type_t<Ts...>> result;
    result.reserve(sizeof...(Ts));
    (result.emplace_back(std::forward<Ts>(mXs)), ...);

    return result;
}

// And what about non-unary `forNArgs`?
// We just need to manipulate the argument packs beforehand, so that
// we can use comma operator folds properly.

// "Third step": call `mFn` once getting the correct elements from the
// forwarded tuple.
template<std::size_t TIStart, typename TF, typename TTpl,
    std::size_t... TIs>
void forNArgsStep(TF&& mFn, TTpl&& mTpl,
    std::index_sequence<TIs...>)
{
    mFn(std::get<TIStart + TIs>(mTpl)...);
}

// "Second step": use a comma operator fold expression to call the
// "third step" `numberOfArgs / TArity` times.
template<std::size_t TArity, typename TF, typename TTpl,
    std::size_t... TIs>
void forNArgsExpansion(TF&& mFn, TTpl&& mTpl,
    std::index_sequence<TIs...>)
{
    using SeqGet = std::make_index_sequence<TArity>;
    (forNArgsStep<TIs * TArity>(mFn, mTpl, SeqGet{}), ...);
}

// "First step / interface".
template<std::size_t TArity, typename TF, typename... Ts>
void forNArgs(TF&& mFn, Ts&&... mXs)
{
    constexpr auto numberOfArgs(sizeof...(Ts));

    static_assert(numberOfArgs % TArity == 0,
        "Invalid number of arguments");

    auto&& asTpl(std::forward_as_tuple(std::forward<Ts>(mXs)...));

    // We need to "convert" the `Ts...` pack into a "list" of packs
    // of size `TArity`.

    using SeqCalls = std::make_index_sequence<numberOfArgs / TArity>;
    forNArgsExpansion<TArity>(mFn, asTpl, SeqCalls{});
}

// I'm confident the `forNArgs` code above can be simplified further.

int main()
{
    auto printWrapper([](auto... xs)
    {
        (std::cout << ... << xs);
        std::cout << " ";
    });

    // Prints "0123":
    printWrapper(0, 1, 2, 3);
    std::cout << "\n";

    // Prints "0 1 2 3":
    forArgs(printWrapper, 0, 1, 2, 3);
    std::cout << "\n";

    // Prints "0 1 2 3":
    for(auto x : make_vector(0, 1, 2, 3)) printWrapper(x);
    std::cout << "\n";

    // Prints "01 23 45 67":
    forNArgs<2>(printWrapper, 0, 1, 2, 3, 4, 5, 6, 7);
    std::cout << "\n";

    // Prints "abc def ghi":
    forNArgs<3>
    (
        printWrapper,
        "a", "b", "c", "d", "e", "f", "g", "h", "i"
    );
    std::cout << "\n";
}

// Thanks for attending!
// Questions?

// http://vittorioromeo.info
// vittorio.romeo@outlook.com