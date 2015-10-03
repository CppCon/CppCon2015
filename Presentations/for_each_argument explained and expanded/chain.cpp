// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <iostream>
#include <algorithm>
#include <vector>
#include <type_traits>

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

template<typename... TArgs>
auto make_vector(TArgs&&... mArgs)
{
    using VectorItem = std::common_type_t<TArgs...>;
    std::vector<VectorItem> result;

    result.reserve(sizeof...(TArgs));

    forArgs
    (
        [&result](auto&& x)
        {
            result.emplace_back(std::forward<decltype(x)>(x));
        },

        std::forward<TArgs>(mArgs)...
    );

    return result;
}

// This is a simple implementation of a python-like `chain`
// utility that allows users to iterate over multiple containers
// as if they were the same one.

template<typename TF, typename... TArgs>
void chain(TF&& mFn, TArgs&&... mArgs)
{
    forArgs
    (
        [mFn](auto& mCont)
        {
            for(auto& x : mCont)
                mFn(x);
        },

        std::forward<TArgs>(mArgs)...
    );
}

int main()
{
    auto v1(make_vector(1, 2, 3, 4, 5));
    auto v2(make_vector(5.1f, 5.2f, 5.3f));
    auto v3(make_vector("6", "7", "8"));
    auto v4(make_vector('9'));

    auto print([](auto&& mX){ std::cout << mX << ", "; });
    chain(print, v1, v2, v3, v4);
    std::cout << "\n";

    auto compose([](auto mFn0, auto mFn1)
    {
        return [mFn0, mFn1](auto&&... mXs)
        {
            return mFn0(mFn1(std::forward<decltype(mXs)>(mXs)...));
        };
    });

    auto triplicate([](auto&& mX){ return mX * 3; });

    chain(compose(print, triplicate), v1, v2);
    std::cout << "\n";

    return 0;
}