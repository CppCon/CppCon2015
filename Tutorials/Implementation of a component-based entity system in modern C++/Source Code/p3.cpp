// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <memory>
#include "../Other/Other.hpp"

// Let's take a look at our "OOP hierarchy":
//
//    GameObject
//    |
//    |----PhysObject
//         |
//         |----CirclePhysObject
//              |
//              |----CirclePhysDrawableObject
//              |    |----Bullet
//              |    |----PlayerBullet
//              |
//              |----CirclePhysDrawableRectObject
//                   |----Player
//                   |----Boss
//

// The issue here is that sharing data and/or logic between game
// objects requires inheritance of an entire other branch of the
// hierarchy.

// Sometimes we want to share just a small piece of logic or data.
// What we're looking for is not inheritance, but composition.

// What if an object only was an aggregate of small components?

// This would lead to better flexibility and reusability.
// It would prevent many `virtual` calls and reduce run-time overhead.

// Also, we know how our entities should behave while writing code.
// Therefore... we could just tell the compiler and have it generate
// efficient data structures at compile-time.

// All of this can be achieved using an entity-component system.
// In contrast to a traditional object-oriented approach, such
// systems make use of a more "data-oriented" approach.

// Separating logic from data and storing components contiguously
// results in cache-friendliness, which has significant run-time
// performance benefits. Such separation also makes serialization
// easier and gives the programmer much more flexibility.

// {image: p3_d0}

// Let's think about the architecture we want to implement to
// achieve this result.

// Elements of the architecture:
//
//  * Component:
//      Simple, logic-less, data-containing struct.
//
//  * ComponentList:
//      Compile-time list of component types.
//
//  * Tag:
//      Simple, logic-less and data-less struct.
//
//  * TagList:
//      Compile-time list of tag types.
//
//  * Signature:
//      Compile-time mixed list of required components and tags.
//
//  * SignatureList:
//      Compile-time list of signature types.
//
//  * Entity:
//      Simple struct "pointing" to the correct component data.
//      Contains a bitset to keep track of available components
//      and tags, and a flag to check if the entity is alive.
//
//  * Manager:
//      "Context" class. Will contain entities, components,
//      signatures, metadata.
//      It will deal with entity/component creation/removal,
//      with entity iteration and much more.
//
//  * Handle:
//      Layer of indirection between the entities and the user.
//      Handles will be used to keep track and access an entity.
//      It will be possible to check if the entity is still valid
//      through an existing handle.
//

// Let's start defining our basic elements in a dedicated namespace.

namespace ecs
{
// NOTE: I've implemented a simple MPL library for this talk, but any
// existing MPL library will do fine.

// The framework will require the user to define its components, tags
// and signature compile-time lists in advance, and pass them to a
// compile-time `Settings` class.

template
<
    typename TComponentList,
    typename TTagList,
    typename TSignatureList
>
struct Settings;

// Example:
/*
    using MyComponents = ComponentList<...>;
    using MyTags = TagList<...>;
    using MySignatures = SignatureList<...>;

    using MySettings = Settings
    <
        MyComponents,
        MyTags,
        MySignatures
    >;
*/

// Our "list classes" will be simple wrappers around MPL type lists.

// Signatures will be compile-time lists of required components
// and tags. They will be listed in an MPL type list.

template<typename... Ts> using Signature = MPL::TypeList<Ts...>;
template<typename... Ts> using SignatureList = MPL::TypeList<Ts...>;

// Components and tags require no library-side code.
// They will be simple structs defined by the user, listed in MPL
// type lists.

// Example:
/*
    struct C0 { int foo; };
    struct C1 { float bar; };
    struct T0 { };
    struct T1 { };

    using MyComponents = ComponentList<C0, C1>;
    using MyTags = TagList<T0, T1>;
    
*/

template<typename... Ts> using ComponentList = MPL::TypeList<Ts...>;
template<typename... Ts> using TagList = MPL::TypeList<Ts...>;

// To actually check if an entity matches a specific signature
// at run-time, we'll make use of `std::bitset` bit operations.
// Every entity will have a bitset keeping track of its current
// components and tags.
// Every signature will have a bitset representing the required
// components and tags.

// {image: p3_d1}

// Let's forward-declare a class that will match signatures
// to their respective bitsets, and also a class that will later
// be instantiated to actually store the signature bitsets.

namespace Impl
{
    template<typename TSettings>
    struct SignatureBitsets;

    template<typename TSettings>
    class SignatureBitsetsStorage;
}

// Let's now implement our `Settings` class.
// It will take the type lists defined by the user as template
// parameters.
// It will be easy to expand with additional compile-time settings
// if required in the future.

template
<
    typename TComponentList,
    typename TTagList,
    typename TSignatureList
>
struct Settings
{
    // Type aliases.
    using ComponentList = typename TComponentList::TypeList;
    using TagList = typename TTagList::TypeList;
    using SignatureList = typename TSignatureList::TypeList;
    using ThisType = Settings<ComponentList, TagList, SignatureList>;

    // "Inner" types aliases.
    using SignatureBitsets = Impl::SignatureBitsets<ThisType>;
    using SignatureBitsetsStorage =
        Impl::SignatureBitsetsStorage<ThisType>;

    // "Type traits".
    template<typename T>
    static constexpr bool isComponent() noexcept
    {
        return MPL::Contains<T, ComponentList>{};
    }
    template<typename T>
    static constexpr bool isTag() noexcept
    {
        return MPL::Contains<T, TagList>{};
    }
    template<typename T>
    static constexpr bool isSignature() noexcept
    {
        return MPL::Contains<T, SignatureList>{};
    }

    // Count of the various elements.
    static constexpr std::size_t componentCount() noexcept
    {
        return MPL::size<ComponentList>();
    }
    static constexpr std::size_t tagCount() noexcept
    {
        return MPL::size<TagList>();
    }
    static constexpr std::size_t signatureCount() noexcept
    {
        return MPL::size<SignatureList>();
    }

    // Unique IDs for the various elements.
    template<typename T>
    static constexpr std::size_t componentID() noexcept
    {
        return MPL::IndexOf<T, ComponentList>{};
    }
    template<typename T>
    static constexpr std::size_t tagID() noexcept
    {
        return MPL::IndexOf<T, TagList>{};
    }
    template<typename T>
    static constexpr std::size_t signatureID() noexcept
    {
        return MPL::IndexOf<T, SignatureList>{};
    }

    // NOTE: C++14 variable templates could have been used here,
    // but a bug present both in clang and gcc prevents it.
    // See: http://stackoverflow.com/questions/32050119

    // Bitsets for component types and tag types.
    using Bitset = std::bitset<componentCount() + tagCount()>;

    // Bit indices for components and tags.
    template<typename T>
    static constexpr std::size_t componentBit() noexcept
    {
        return componentID<T>();
    }
    template<typename T>
    static constexpr std::size_t tagBit() noexcept
    {
        // Tag bits will be stored towards the end of the bitset.
        return componentCount() + tagID<T>();
    }
};

namespace Impl
{
// Implementation of the `SignatureBitsets` class.
template<typename TSettings>
struct SignatureBitsets
{
    // Type aliases.
    using Settings = TSettings;
    using ThisType = SignatureBitsets<Settings>;
    using SignatureList = typename Settings::SignatureList;
    using Bitset = typename Settings::Bitset;

    // We will store signature bitsets in an `std::tuple` of
    // `Bitset`, where `Bitset` is repeated by the count of
    // signature types.

    // `Bitset` will be repeated `signatureCount()` times.
    using BitsetRepeatedList = MPL::Repeat
    <
        Settings::signatureCount(),
        Bitset
    >;

    // Our repeated list will then be converted to a `std::tuple`.
    // We will then be able to get the bitset for the 'N'th
    // signature using `std::get<N>`.
    using BitsetStorage = MPL::Tuple<BitsetRepeatedList>;

    // We're going to filter types at compile time to separate
    // components and tags in signature definitions.
    // My MPL library requires predicates to be classes, so it
    // is necessary to wrap the previously defined functions.
    template<typename T>
    using IsComponentFilter = std::integral_constant
    <
        bool, Settings::template isComponent<T>()
    >;

    template<typename T>
    using IsTagFilter = std::integral_constant
    <
        bool, Settings::template isTag<T>()
    >;

    // Let's also write some shortcut aliases for signature
    // elements filtering.

    // This will return all required components.
    template<typename TSignature>
    using SignatureComponents = MPL::Filter
    <
        IsComponentFilter,
        TSignature
    >;

    // This will return all required tags.
    template<typename TSignature>
    using SignatureTags = MPL::Filter
    <
        IsTagFilter,
        TSignature
    >;
};

// Let's now implement the class that will actually store
// the bitsets. This will be instantiated in our `Manager` later.
template<typename TSettings>
class SignatureBitsetsStorage
{
private:
    using Settings = TSettings;
    using SignatureBitsets = typename Settings::SignatureBitsets;
    using SignatureList = typename SignatureBitsets::SignatureList;
    using BitsetStorage = typename SignatureBitsets::BitsetStorage;

    BitsetStorage storage;

public:
    template<typename T>
    auto& getSignatureBitset() noexcept
    {
        static_assert(Settings::template isSignature<T>(), "");

        // As previously mentioned, we can easily get the bitset
        // by using `std::get` with the unique ID of the signature.
        return std::get<Settings::template signatureID<T>()>(storage);
    }

private:
    // We'll have to initialize the bitsets at run-time. This
    // function will take care of initializing the bitset for a
    // single signature `T`.
    template<typename T>
    void initializeBitset() noexcept
    {
        auto& b(this->getSignatureBitset<T>());

        // Compile-time list of required components.
        using SignatureComponents =
            typename SignatureBitsets::template SignatureComponents<T>;

        // Compile-time list of required tags.
        using SignatureTags =
            typename SignatureBitsets::template SignatureTags<T>;

        // I use a `boost::hana`-like lambda-based type iteration
        // technique to iterate over a compile-time type list.

        // The following calls to `forTypes` will simply execute the
        // body of the lambda for every type contained in the passed 
        // compile-time type list.

        // `t` will be an instance of a simple `MPL::Type` wrapper:
        /*
            template<typename T>
            struct Type
            {
                using type = T;
            };
        */

        MPL::forTypes<SignatureComponents>([this, &b](auto t)
        {
            // The `ECS_TYPE(t)` macro expands to:
            // `typename decltype(t)::type`.

            b[Settings::template componentBit<ECS_TYPE(t)>()] = true;
        });

        MPL::forTypes<SignatureTags>([this, &b](auto t)
        {
            b[Settings::template tagBit<ECS_TYPE(t)>()] = true;
        });
    }

public:
    // Lastly, let's initialize all bitsets on construction.
    SignatureBitsetsStorage() noexcept
    {
        MPL::forTypes<SignatureList>([this](auto t)
        {
            this->initializeBitset<ECS_TYPE(t)>();
        });
    }
};
}
}

// It is a good idea to run some compile-time and run-time tests to
// make sure our implementation is correct and working as intended.

namespace tests
{
// Here's how the user will define components:
struct C0 { };
struct C1 { };
struct C2 { };
struct C3 { };
struct C4 { };
using MyComponentList = ecs::ComponentList<C0, C1, C2, C3, C4>;

// Here's how the user will define tags:
struct T0 { };
struct T1 { };
struct T2 { };
using MyTagList = ecs::TagList<T0, T1, T2>;

// Here's how the user will define signatures:
using S0 = ecs::Signature<>;
using S1 = ecs::Signature<C0, C1>;
using S2 = ecs::Signature<C0, C4, T0>;
using S3 = ecs::Signature<C1, T0, C3, T2>;
using MySignatureList = ecs::SignatureList<S0, S1, S2, S3>;

// After defining everything, the user will alias `Settings`
// with its compile-time lists.
using MySettings =
    ecs::Settings<MyComponentList, MyTagList, MySignatureList>;

// Let's run some basic compile-time tests to make sure everything
// is fine.

static_assert(MySettings::componentCount() == 5, "");
static_assert(MySettings::tagCount() == 3, "");
static_assert(MySettings::signatureCount() == 4, "");

static_assert(MySettings::componentID<C0>() == 0, "");
static_assert(MySettings::componentID<C1>() == 1, "");
static_assert(MySettings::componentID<C2>() == 2, "");
static_assert(MySettings::componentID<C3>() == 3, "");
static_assert(MySettings::componentID<C4>() == 4, "");

static_assert(MySettings::tagID<T0>() == 0, "");
static_assert(MySettings::tagID<T1>() == 1, "");
static_assert(MySettings::tagID<T2>() == 2, "");

static_assert(MySettings::signatureID<S0>() == 0, "");
static_assert(MySettings::signatureID<S1>() == 1, "");
static_assert(MySettings::signatureID<S2>() == 2, "");
static_assert(MySettings::signatureID<S3>() == 3, "");

using MySignatureBitsets = typename MySettings::SignatureBitsets;

static_assert(std::is_same
<
    MySignatureBitsets::SignatureComponents<S0>,
    ecs::MPL::TypeList<>
>{}, "");

static_assert(std::is_same
<
    MySignatureBitsets::SignatureComponents<S3>,
    ecs::MPL::TypeList<C1, C3>
>{}, "");

static_assert(std::is_same
<
    MySignatureBitsets::SignatureTags<S3>,
    ecs::MPL::TypeList<T0, T2>
>{}, "");

// We'll also run some compile-time tests that will initialize
// the signature bitsets and make sure their values are correct.
void runtimeTests()
{
    using Bitset = typename MySignatureBitsets::Bitset;
    ecs::Impl::SignatureBitsetsStorage<MySettings> msb;

    const auto& bS0(msb.getSignatureBitset<S0>());
    const auto& bS1(msb.getSignatureBitset<S1>());
    const auto& bS2(msb.getSignatureBitset<S2>());
    const auto& bS3(msb.getSignatureBitset<S3>());

    assert(bS0 == Bitset{"00000000"});
    assert(bS1 == Bitset{"00000011"});
    assert(bS2 == Bitset{"00110001"});
    assert(bS3 == Bitset{"10101010"});
}
}

namespace example
{
// Nothing here... yet.

struct Game : Boilerplate::TestApp
{
    Game(ssvs::GameWindow& mX) : Boilerplate::TestApp{mX}
    {
        onTxtInfoUpdate += [this](auto&, FT){ };
    }

    void update(FT) override { }
    void draw() override { }
};
}

int main()
{
    tests::runtimeTests();
    std::cout << "Tests passed!" << std::endl;

    // Boilerplate::AppRunner<example::Game>{"ECS", 320, 240};
    return 0;
}

// In the following code segment we'll start implementing
// the core of our ECS system.