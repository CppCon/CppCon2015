// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <iostream>
#include <initializer_list>

// Welcome to my talk:
// "`for_each_argument` explained and expanded".

// During this presentation we'll take a look at a very interesting
// code snippet, originally posted on Twitter by Sean Parent.

// {image: p0_d0}

// We'll also cover a very useful C++14 standard library addition,
// "compile-time integer sequences", and briefly take a look at C++17
// "fold expressions".

// ------------------------------------------------------------------

// So, what does `for_each_argument` do?
// Well, the name is pretty self-explanatory...

// It invokes a callable object on every passed argument.

template <class F, class... Ts>
void for_each_argument(F f, Ts&&... a) {
    (void)std::initializer_list<int>{(f(std::forward<Ts>(a)), 0)...};
}

int main()
{
    // Prints "hello123".
    for_each_argument
    (
        [](const auto& x){ std::cout << x; },

        "hello",
        1,
        2u,
        3.f
    );

    std::cout << "\n";
    return 0;
}

// That is cool. How does it work?
// Let's discover that in the next code segment.