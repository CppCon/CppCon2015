// Copyright (c) 2015 Vittorio Romeo
// License: MIT License | http://opensource.org/licenses/MIT
// http://vittorioromeo.info | vittorio.romeo@outlook.com

// This code segments shows the "boilerplate" code we're going to
// use.

// The `Other` module will include code that deals with
// graphics/input management and general-purpose utilities.
#include "../Other/Other.hpp"

// Let's begin by creating a simple window that we'll use for our
// game example.

namespace example
{
// By deriving from `Boilerplate::TestApp`, we can easily
// define a class representing a simple game.
struct Game : Boilerplate::TestApp
{
    Game(ssvs::GameWindow& mX) : Boilerplate::TestApp{mX}
    {
        // We can print debug text with this "delegate".
        onTxtInfoUpdate += [this](auto& oss, FT)
        {
            oss << "Hello world!\n";
        };
    }

    // Logic can be defined by overriding the following methods.
    void update(FT) override { }
    void draw() override { }
};
}

int main()
{
    // The app is ran by instantiating `Boilerplate::AppRunner`.
    Boilerplate::AppRunner<example::Game>{"ECS", 320, 240};
    return 0;
}

// In the interest of time, the `p1` and `p2` OOP implementation
// segments will be explained and executed very quickly.