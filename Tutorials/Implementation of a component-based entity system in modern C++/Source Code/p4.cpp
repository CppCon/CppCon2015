// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <memory>
#include "../Other/Other.hpp"

// In this code segment we'll implement the `Entity` data
// structure and begin coding our manager class.

// The architecture of our system will be as such:
//
// * Entity metadata will be stored contiguously in a vector.
//
// * Entities can be either "alive" or "dead".
//   Alive entities will be stored towards the beginning of the
//   vector.
//
// * Entity metadata will be re-ordered to satisfy the above
//   constraint.
//
// * Component data will be stored contiguously in vectors.
//
// * Component data will not be re-ordered. Entities will point
//   to their component data through an index.
//
// * Creating/killing entities will not be reflected in
//   the state of the manager until a `refresh()` method is called.
//   The refresh method will also take care of reordering
//   the entity metadata.
//
// * Newly created entities will however be immediately accessible.
//   The user will be able to access their data and add/remove
//   components before `refresh()` is called.
//

// {image: p4_d0}

namespace ecs
{
// Let's begin by defining some useful typedefs for the data
// structures we're going to implement.

// We're going to use "strong typedefs", which will make sure our
// types are actually different and allow us to write function
// overloads and prevent conversion mistakes.

// My implementation is similar to boost's `strong_typedef`
// module.

// Index of "real" data in component storage.
ECS_STRONG_TYPEDEF(std::size_t, DataIndex);

// Index of entity "metadata", stored in a contiguous structure.
ECS_STRONG_TYPEDEF(std::size_t, EntityIndex);

// Let's now define our `Entity` class.
// The metadata stored by `Entity` will depend on the user-defined
// settings. Therefore, we need to pass our settings as a template
// parameter.

namespace Impl
{
template<typename TSettings>
struct Entity
{
    using Settings = TSettings;
    using Bitset = typename Settings::Bitset;

    // Index of data in the components storage.
    DataIndex dataIndex;

    // What components and tags does the entity have?
    Bitset bitset;

    // Is the entity alive?
    bool alive;
};
}

template<typename... Ts> using Signature = MPL::TypeList<Ts...>;
template<typename... Ts> using SignatureList = MPL::TypeList<Ts...>;
template<typename... Ts> using ComponentList = MPL::TypeList<Ts...>;
template<typename... Ts> using TagList = MPL::TypeList<Ts...>;

namespace Impl
{
    template<typename TSettings>
    struct SignatureBitsets;

    template<typename TSettings>
    class SignatureBitsetsStorage;
}

template
<
    typename TComponentList,
    typename TTagList,
    typename TSignatureList
>
struct Settings
{
    using ComponentList = typename TComponentList::TypeList;
    using TagList = typename TTagList::TypeList;
    using SignatureList = typename TSignatureList::TypeList;
    using ThisType = Settings<ComponentList, TagList, SignatureList>;

    using SignatureBitsets = Impl::SignatureBitsets<ThisType>;
    using SignatureBitsetsStorage =
        Impl::SignatureBitsetsStorage<ThisType>;

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

    using Bitset = std::bitset<componentCount() + tagCount()>;

    template<typename T>
    static constexpr std::size_t componentBit() noexcept
    {
        return componentID<T>();
    }
    template<typename T>
    static constexpr std::size_t tagBit() noexcept
    {
        return componentCount() + tagID<T>();
    }
};

namespace Impl
{
template<typename TSettings>
struct SignatureBitsets
{
    using Settings = TSettings;
    using ThisType = SignatureBitsets<Settings>;
    using SignatureList = typename Settings::SignatureList;
    using Bitset = typename Settings::Bitset;

    using BitsetStorage = MPL::Tuple
    <
        MPL::Repeat
        <
            Settings::signatureCount(),
            Bitset
        >
    >;

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

    template<typename TSignature>
    using SignatureComponents = MPL::Filter
    <
        IsComponentFilter,
        TSignature
    >;

    template<typename TSignature>
    using SignatureTags = MPL::Filter
    <
        IsTagFilter,
        TSignature
    >;
};

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
        return std::get<Settings::template signatureID<T>()>(storage);
    }

private:
    template<typename T>
    void initializeBitset() noexcept
    {
        auto& b(this->getSignatureBitset<T>());

        using SignatureComponents =
            typename SignatureBitsets::
                template SignatureComponents<T>;

        using SignatureTags =
            typename SignatureBitsets::
                template SignatureTags<T>;

        MPL::forTypes<SignatureComponents>([this, &b](auto t)
        {
            b[Settings::template componentBit<ECS_TYPE(t)>()] = true;
        });

        MPL::forTypes<SignatureTags>([this, &b](auto t)
        {
            b[Settings::template tagBit<ECS_TYPE(t)>()] = true;
        });
    }

public:
    SignatureBitsetsStorage() noexcept
    {
        MPL::forTypes<SignatureList>([this](auto t)
        {
            this->initializeBitset<ECS_TYPE(t)>();
        });
    }
};
}

// Let's start implementing our manager class.

template<typename TSettings>
class Manager
{
private:
    // Type aliases.
    using Settings = TSettings;
    using ThisType = Manager<Settings>;
    using Bitset = typename Settings::Bitset;
    using Entity = Impl::Entity<Settings>;
    using SignatureBitsetsStorage =
        Impl::SignatureBitsetsStorage<Settings>;

    // We'll need to keep track of the current size,
    // the next size and the total capacity of the manager.

    // Storage capacity (memory is allocated for `capacity` entities).
    std::size_t capacity{0};

    // Current size.
    // Does not take into account newly created entities.
    std::size_t size{0};

    // Next size (after refresh).
    // Takes into account newly created entities.
    std::size_t sizeNext{0};

    // {image: p4_d1}

    // We will store our `Entity` metadata in a `std::vector`.
    std::vector<Entity> entities;

    // Our manager will also instantiate and store signature
    // bitsets.
    SignatureBitsetsStorage signatureBitsets;

    // Our manager class will need to grow and reallocate memory for
    // the entity metadata storage (and for components data in the
    // following code segments).

    // Let's define some helper methods that will deal with memory
    // growth.

    void growTo(std::size_t mNewCapacity)
    {
        assert(mNewCapacity > capacity);

        // Resize the entities vector.
        entities.resize(mNewCapacity);

        // Initialize the newly allocated entities.
        for(auto i(capacity); i < mNewCapacity; ++i)
        {
            auto& e(entities[i]);

            // When inititialized, entity metadata will point to data
            // with index equal to its own.
            e.dataIndex = i;

            // Remove all components/tags and set entity to dead.
            e.bitset.reset();
            e.alive = false;
        }

        capacity = mNewCapacity;
    }

    void growIfNeeded()
    {
        // If we already have enough space to hold all newly created
        // entities, we can exit the function early.
        if(capacity > sizeNext) return;

        // Otherwise, we grow by a fixed amount.
        growTo((capacity + 10) * 2);
    }

    // Let's define some safe methods to retrieve entities via
    // `EntityIndex`.

    auto& getEntity(EntityIndex mI) noexcept
    {
        assert(sizeNext > mI);
        return entities[mI];
    }

    const auto& getEntity(EntityIndex mI) const noexcept
    {
        assert(sizeNext > mI);
        return entities[mI];
    }

public:
    // We'll need to immediately grow upon construction.
    Manager()
    {
        growTo(100);
    }

    // Let's also start defining the basic user interface that will
    // interact with entities via `EntityIndex`.

    // These methods are pretty self-explanatory.

    auto isAlive(EntityIndex mI) const noexcept
    {
        return getEntity(mI).alive;
    }

    void kill(EntityIndex mI) noexcept
    {
        getEntity(mI).alive = false;
    }

    // Adding, removing and checking tags consists in simple bitset
    // operations.

    template<typename T>
    auto hasTag(EntityIndex mI) const noexcept
    {
        static_assert(Settings::template isTag<T>(), "");
        return getEntity(mI).bitset[Settings::template tagBit<T>()];
    }

    template<typename T>
    void addTag(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isTag<T>(), "");
        getEntity(mI).bitset[Settings::template tagBit<T>()] = true;
    }

    template<typename T>
    void delTag(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isTag<T>(), "");
        getEntity(mI).bitset[Settings::template tagBit<T>()] = false;
    }

    // Checking and removing components are simple bitset operations,
    // as well. `addComponent` and `getComponent` will be implemented
    // after coding the component data storage.

    template<typename T>
    auto hasComponent(EntityIndex mI) const noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");
        return getEntity(mI).bitset
            [Settings::template componentBit<T>()];
    }

    template<typename T, typename... TArgs>
    void addComponent(EntityIndex mI, TArgs&&... mXs) noexcept;

    template<typename T>
    auto& getComponent(EntityIndex mI) noexcept;

    template<typename T>
    void delComponent(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");
        getEntity(mI).bitset
            [Settings::template componentBit<T>()] = false;
    }

    // Let's now implement a method that will allow the user to create
    // new entities. It will return an `EntityIndex`.

    auto createIndex()
    {
        // Grow (reallocate) if memory is not available.
        growIfNeeded();

        // Get first available metadata index.
        EntityIndex freeIndex(sizeNext++);

        // We assert the entity is marked as dead.
        assert(!isAlive(freeIndex));

        // Initialize metadata for the new entity.
        auto& e(entities[freeIndex]);
        e.alive = true;
        e.bitset.reset();

        // Return the index of the newly created entity.
        return freeIndex;
    }

    // We'll also need a method to clear the manager.

    void clear() noexcept
    {
        for(auto i(0u); i < capacity; ++i)
        {
            auto& e(entities[i]);

            e.dataIndex = i;
            e.bitset.reset();
            e.alive = false;
        }

        size = sizeNext = 0;
    }

    // Now we'll implement the `refresh()` method.
    // It will consist in a "interface" function calling an inner 
    // implementation function if required.

    // The algorithm is quite simple but requires some explanation.

    void refresh() noexcept
    {
        // If no new entities have been created, set `size` to `0` 
        // and exit early.
        if(sizeNext == 0)
        {
            size = 0;
            return;
        }

        // Otherwise, get the new `size` by calling `refreshImpl()`.
        // After refreshing, `size` will equal `sizeNext`.
        // The final value for these variables will be calculated
        // by re-arranging entity metadata in the `entities` vector.
        size = sizeNext = refreshImpl();
    }

    // The `refreshImpl()` method contains the core refresh algorithm.
    // It should efficiently:
    // * Move all alive entities towards the beginning of the vector.
    // * Move all dead entities towards the end of the vector.
    // * Eventually invalidate all dead entities' handles and refresh
    //   alive entities' handles.
    // * Calculate and return the new "real" count of alive entities.

private:
    auto refreshImpl() noexcept
    {
        // The algorithm is implemented using two indices.
        // * `iD` looks for dead entities, starting from the left.
        // * `iA` looks for alive entities, starting from the right.

        // Newly created entities will always be at the end of the
        // vector.

        // Alive entities found on the right will be swapped with
        // dead entities found on the left - this will re-arrange
        // the entities as intended.

        // Particular care must be taken to avoid off-by-one errors!

        // The function will return the number of alive entities,
        // which is one-past the index of the last alive entity.
        EntityIndex iD{0}, iA{sizeNext - 1};

        // {image: p4_d2}

        while(true)
        {
            // Find first dead entity from the left.
            for(; true; ++iD)
            {
                // We have to immediately check if we have gone
                // through the `iA` index. If that's the case, there
                // are no more dead entities in the vector and we can
                // return `iD` as our result.
                if(iD > iA) return iD;

                // If we found a dead entity, we break out of this
                // inner for-loop.
                if(!entities[iD].alive) break;
            }

            // Find first alive entity from the right.
            for(; true; --iA)
            {
                // In the case of alive entities, we have to perform
                // the checks done above in the reverse order.

                // If we found an alive entity, we immediately break
                // out of this inner for-loop.
                if(entities[iA].alive) break;

                // Otherwise, we acknowledge this is an entity that
                // has been killed since last refresh.
                // We will invalidate its handle here later.

                // If we reached `iD` or gone through it, we are
                // certain there are no more alive entities in the
                // entity metadata vector, and we can return `iD`.
                if(iA <= iD) return iD;
            }

            // If we arrived here, we have found two entities that
            // need to be swapped.

            // `iA` points to an alive entity, towards the right of
            // the vector.
            assert(entities[iA].alive);

            // `iD` points to a dead entity, towards the left of the
            // vector.
            assert(!entities[iD].alive);

            // Therefore, we swap them to arrange all alive entities
            // towards the left.
            std::swap(entities[iA], entities[iD]);

            // After swapping, we will eventually need to refresh
            // the alive entity's handle and invalidate the dead
            // swapped entity's handle.

            // Move both "iterator" indices.
            ++iD; --iA;
        }

        return iD;
    }

// Lastly, some methods returing useful information.
public:
    auto getEntityCount() const noexcept
    {
        return size;
    }

    auto getCapacity() const noexcept
    {
        return capacity;
    }

    // Let's also create a debugging function that will
    // print the state of the entity metadata.

    auto printState(std::ostream& mOSS) const
    {
        mOSS << "\nsize: " << size
             << "\nsizeNext: " << sizeNext
             << "\ncapacity: " << capacity
             << "\n";

        for(auto i(0u); i < sizeNext; ++i)
        {
            auto& e(entities[i]);
            mOSS << (e.alive ? "A" : "D");
        }

        mOSS << "\n\n";
    }
};
}

// We're still missing quite a few things:
// * Handles.
// * Components.
// * Entity/signature matching.

// Before implementing those things, let's run some tests:
namespace tests
{
struct C0 { };
struct C1 { };
struct C2 { };
struct C3 { };
struct C4 { };
using MyComponentList = ecs::ComponentList<C0, C1, C2, C3, C4>;

struct T0 { };
struct T1 { };
struct T2 { };
using MyTagList = ecs::TagList<T0, T1, T2>;

using S0 = ecs::Signature<>;
using S1 = ecs::Signature<C0, C1>;
using S2 = ecs::Signature<C0, C4, T0>;
using S3 = ecs::Signature<C1, T0, C3, T2>;
using MySignatureList = ecs::SignatureList<S0, S1, S2, S3>;

using MySettings =
    ecs::Settings<MyComponentList, MyTagList, MySignatureList>;

using MyManager = ecs::Manager<MySettings>;

void runtimeTests()
{
    MyManager mgr;

    std::cout << "After manager instantiated\n";
    mgr.printState(std::cout);

    assert(mgr.getEntityCount() == 0);

    auto i0 = mgr.createIndex();

    std::cout << "After entity 0 created\n";
    mgr.printState(std::cout);

    auto i1 = mgr.createIndex();

    std::cout << "After entity 1 created\n";
    mgr.printState(std::cout);

    assert(!mgr.hasTag<T0>(i0));
    mgr.addTag<T0>(i0);
    assert(mgr.hasTag<T0>(i0));

    assert(!mgr.hasTag<T1>(i1));
    mgr.addTag<T1>(i1);
    assert(mgr.hasTag<T1>(i1));

    assert(mgr.hasTag<T0>(i0));
    mgr.delTag<T0>(i0);
    assert(!mgr.hasTag<T0>(i0));

    // Notice how the "entity count" stays to zero until we call the
    // `refresh()` method.
    assert(mgr.getEntityCount() == 0);

    mgr.refresh();

    assert(!mgr.hasTag<T0>(i0));
    assert(mgr.hasTag<T1>(i1));

    std::cout << "After refresh\n";
    mgr.printState(std::cout);

    assert(mgr.getEntityCount() == 2);

    mgr.clear();

    std::cout << "After clear\n";
    mgr.printState(std::cout);

    assert(mgr.getEntityCount() == 0);

    for(auto i(0u); i < 20; ++i)
    {
        for(auto j(0u); j < 5; ++j)
            if(ssvu::getRndI(0, 100) < 50)
                mgr.createIndex();

        for(auto j(0u); j < mgr.getEntityCount(); ++j)
            if(ssvu::getRndI(0, 100) < 50)
                mgr.kill(ecs::EntityIndex(j));

        std::cout << "Before test iteration " << i << " refresh\n";
        mgr.printState(std::cout);
        std::cout << "\n";

        mgr.refresh();

        std::cout << "After test iteration " << i << " refresh\n";
        mgr.printState(std::cout);
        std::cout << "\n";
    }
}
}

namespace example
{
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