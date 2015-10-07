// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <memory>
#include "../Other/Other.hpp"

// Features still missing:
// * Component storage.
// * Entity/signature matching.
// * Handles.

// In this code segment we'll implement component storage,
// entity/signature matching and entity iteration.

namespace ecs
{
ECS_STRONG_TYPEDEF(std::size_t, DataIndex);
ECS_STRONG_TYPEDEF(std::size_t, EntityIndex);

namespace Impl
{
template<typename TSettings>
struct Entity
{
    using Settings = TSettings;
    using Bitset = typename Settings::Bitset;

    DataIndex dataIndex;
    Bitset bitset;
    bool alive;
};

// Let's implement the `ComponentStorage` class.
// It will be instantiated inside the manager.

template<typename TSettings>
class ComponentStorage
{
private:
    // Type aliases.
    using Settings = TSettings;
    using ComponentList = typename Settings::ComponentList;

    // We want to have a single `std::vector` for every
    // component type.
    
    // We know the types of the components at compile-time.
    // Therefore, we can use `std::tuple`.
    
    template<typename... Ts>
    using TupleOfVectors = std::tuple<std::vector<Ts>...>;

    // We need to "unpack" the contents of `ComponentList` in
    // `TupleOfVectors`. We can do that using `MPL::Rename`.
    MPL::Rename<TupleOfVectors, ComponentList> vectors;

    // That's it!
    // We have separate contiguous storage for all component
    // types.

    // All that's left is defining a public interface for the
    // component storage.

    // We will want to:
    // * Grow every vector.
    // * Get a component of a specific type via `DataIndex`.

public:
    void grow(std::size_t mNewCapacity)
    {
        // Hmm... we need to iterate over every tuple element.
        // Compile-time recursion? Nah.

        Utils::forTuple([this, mNewCapacity](auto& v)
        {
            v.resize(mNewCapacity);
        }, vectors);
    }

    template<typename T>
    auto& getComponent(DataIndex mI) noexcept
    {
        // We need to get the correct vector, depending on `T`.
        // Compile-time recursion? Nah.

        // Let's use C++14's `std::get` instead!

        return std::get<std::vector<T>>(vectors)[mI];
    }
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

    template<typename T>
    const auto& getSignatureBitset() const noexcept
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

template<typename TSettings>
class Manager
{
private:
    using Settings = TSettings;
    using ThisType = Manager<Settings>;
    using Bitset = typename Settings::Bitset;
    using Entity = Impl::Entity<Settings>;
    using SignatureBitsetsStorage =
        Impl::SignatureBitsetsStorage<Settings>;
    using ComponentStorage = Impl::ComponentStorage<Settings>;

    std::size_t capacity{0}, size{0}, sizeNext{0};
    std::vector<Entity> entities;
    SignatureBitsetsStorage signatureBitsets;

    // Let's instantiate our component storage.
    ComponentStorage components;

    void growTo(std::size_t mNewCapacity)
    {
        assert(mNewCapacity > capacity);

        entities.resize(mNewCapacity);

        // Remember we need to grow the component storage now.
        components.grow(mNewCapacity);

        for(auto i(capacity); i < mNewCapacity; ++i)
        {
            auto& e(entities[i]);

            e.dataIndex = i;
            e.bitset.reset();
            e.alive = false;
        }

        capacity = mNewCapacity;
    }

    void growIfNeeded()
    {
        if(capacity > sizeNext) return;
        growTo((capacity + 10) * 2);
    }

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
    Manager() { growTo(100); }

    auto isAlive(EntityIndex mI) const noexcept
    {
        return getEntity(mI).alive;
    }

    void kill(EntityIndex mI) noexcept
    {
        getEntity(mI).alive = false;
    }

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

    template<typename T>
    auto hasComponent(EntityIndex mI) const noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");
        return getEntity(mI).bitset
            [Settings::template componentBit<T>()];
    }

    // `addComponent` will not only set the correct component bit,
    // but also construct `T` by perfect-forwarding user-passed args.
    // It will return a reference to the newly constructed component.
    template<typename T, typename... TArgs>
    auto& addComponent(EntityIndex mI, TArgs&&... mXs) noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");

        auto& e(getEntity(mI));
        e.bitset[Settings::template componentBit<T>()] = true;

        // `FWD(x)` is a macro that expands to
        // `::std::forward<decltype(x)>(x)`.
        
        auto& c(components.template getComponent<T>(e.dataIndex));
        new (&c) T(FWD(mXs)...);
        return c;
    }

    // `getComponent` will simply return a reference to the component,
    // after asserting its existance.
    template<typename T>
    auto& getComponent(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");
        assert(hasComponent<T>(mI));

        return components.
            template getComponent<T>(getEntity(mI).dataIndex);
    }

    template<typename T>
    void delComponent(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");
        getEntity(mI).bitset
            [Settings::template componentBit<T>()] = false;
    }

    auto createIndex()
    {
        growIfNeeded();
        EntityIndex freeIndex(sizeNext++);

        assert(!isAlive(freeIndex));
        auto& e(entities[freeIndex]);
        e.alive = true;
        e.bitset.reset();

        return freeIndex;
    }

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

    void refresh() noexcept
    {
        if(sizeNext == 0)
        {
            size = 0;
            return;
        }

        size = sizeNext = refreshImpl();
    }

    // Let's now deal with entity/signature matching and entity
    // iteration.

    // `matchesSignature` will return `true` if the entity at
    // index `mI` "matches" the signature of type `T`.
    template<typename T>
    auto matchesSignature(EntityIndex mI) const noexcept
    {
        // {image: p5_d0}

        static_assert(Settings::template isSignature<T>(), "");

        const auto& entityBitset(getEntity(mI).bitset);
        const auto& signatureBitset(signatureBitsets.
            template getSignatureBitset<T>());

        return (signatureBitset & entityBitset) == signatureBitset;
    }

    // We will need to iterate over:
    // * All alive entities.
    // * All alive entities matching a particular signature.

    // I like using a more "functional approach" for custom iteration
    // functions. Instead of returns some iterators, I find it much
    // cleaner to pass a function to an iteration method.

    template<typename TF>
    void forEntities(TF&& mFunction)
    {
        for(EntityIndex i{0}; i < size; ++i)
            mFunction(i);
    }

    template<typename T, typename TF>
    void forEntitiesMatching(TF&& mFunction)
    {
        static_assert(Settings::template isSignature<T>(), "");

        forEntities([this, &mFunction](auto i)
        {
            if(matchesSignature<T>(i))
            {
                // If an entity matches a specific signature, we
                // know the component types that the entity will
                // have.

                // So, we'll expand references to those component
                // types directly in the function call, saving 
                // unnecessary boilerplate user code.

                expandSignatureCall<T>(i, mFunction);
            }
        });
    }

private:
    template<typename... Ts>
    struct ExpandCallHelper;

    template<typename T, typename TF>
    void expandSignatureCall(EntityIndex mI, TF&& mFunction)
    {
        static_assert(Settings::template isSignature<T>(), "");

        // What we need to do:
        // * Put all signature component types in a compile-time
        //   type list.
        // * Expand that list into a call to `mFunction`, passing
        //   a reference to every component.

        // List of signature component types.
        using RequiredComponents = typename Settings::
            SignatureBitsets::template SignatureComponents<T>;

        // To actually perform the expansion, we'll use a variadic
        // inner class, so we can make use of the intuitive `...`
        // ellipsis operator.

        // Let's "rename" `TypeList` to `ExpandCallHelper`, to
        // pass the component types to the helper class.
        using Helper = MPL::Rename
        <
            ExpandCallHelper, RequiredComponents
        >;

        // And... done.
        Helper::call(mI, *this, mFunction);
    }

    // `ExpandCallHelper` is an inner class taking a variadic number
    // of component types.

    template<typename... Ts>
    struct ExpandCallHelper
    {
        // It contains a single static `call` function, which takes
        // the index of the entity, a reference to the caller manager,
        // and the function to call.

        // The `call` function will expand the component types into
        // component references to the correct entity, thanks to the
        // ellipsis operator.

        template<typename TF>
        static void call(EntityIndex mI, ThisType& mMgr,
            TF&& mFunction)
        {
            auto di(mMgr.getEntity(mI).dataIndex);

            // Call `mFunction`...
            mFunction
            (
                // ...with the entity index as the first argument...
                mI,

                // ...and with `N` `getComponent` calls, where `N` is
                // the number of component types in the `Ts...` pack.
                mMgr.components.template getComponent<Ts>(di)...

                // The `...` above will expand the `Ts` type pack,
                // and basically repeat the lines of code above for
                // every component type.
            );

            // Here's an example result call of an entity with index
            // `0` and components `C0`, `C1` and `C2`:
            //
            //     mFunction
            //     (
            //         0,
            //
            //         mMgr.components.template getComponent<C0>(di),
            //         mMgr.components.template getComponent<C1>(di),
            //         mMgr.components.template getComponent<C2>(di)
            //     );
            //
        }
    };

private:
    auto refreshImpl() noexcept
    {
        EntityIndex iD{0}, iA{sizeNext - 1};

        while(true)
        {
            for(; true; ++iD)
            {
                if(iD > iA) return iD;
                if(!entities[iD].alive) break;
            }

            for(; true; --iA)
            {
                if(entities[iA].alive) break;
                if(iA <= iD) return iD;
            }

            assert(entities[iA].alive);
            assert(!entities[iD].alive);

            std::swap(entities[iA], entities[iD]);

            ++iD; --iA;
        }

        return iD;
    }

public:
    auto getEntityCount() const noexcept
    {
        return size;
    }

    auto getCapacity() const noexcept
    {
        return capacity;
    }

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

// Let's test our work so far... with a particle simulation.

namespace example
{
// Component definitions:
struct CPosition { Vec2f value; };
struct CVelocity { Vec2f value; };
struct CAcceleration { Vec2f value; };
struct CLife { float value; };

struct CRender
{
    sf::RectangleShape shape;

    CRender()
    {
        shape.setFillColor(sf::Color::Red);
        shape.setSize(Vec2f{5.f, 5.f});
    }
};

using MyComponents = ecs::ComponentList
<
    CPosition, CVelocity, CAcceleration, CRender, CLife
>;

// We won't need any tags for this example.
using MyTags = ecs::TagList<>;

// Signature definitions:
using SApplyVelocity = ecs::Signature<CPosition, CVelocity>;
using SApplyAcceleration = ecs::Signature<CVelocity, CAcceleration>;
using SRender = ecs::Signature<CPosition, CRender>;
using SLife = ecs::Signature<CLife>;
using SGrow = ecs::Signature<CRender, CLife>;

using MySignatures = ecs::SignatureList
<
    SApplyVelocity, SApplyAcceleration, SRender, SLife, SGrow
>;

using MySettings = ecs::Settings
<
    MyComponents, MyTags, MySignatures
>;

using MyManager = ecs::Manager<MySettings>;

struct Game : Boilerplate::TestApp
{
    MyManager mgr;

    Game(ssvs::GameWindow& mX) : Boilerplate::TestApp{mX}
    {
        onTxtInfoUpdate += [this](auto& oss, FT)
        {
            oss << "Entities: " << mgr.getEntityCount() << "\n";
        };
    }

    void update(FT mFT) override
    {
        // Particle creation loop:
        for(auto i(0u); i < 40; ++i)
        {
            // Create an entity an get its index.
            auto e(mgr.createIndex());

            auto setRndVec2([](auto& mVec, auto mX)
            {
                mVec.x = ssvu::getRndR(-mX, mX);
                mVec.y = ssvu::getRndR(-mX, mX);
            });

            // Add position, velocity, acceleration, life,
            // and shape components with random values.

            auto& pos(mgr.addComponent<CPosition>(e).value);
            setRndVec2(pos, 100.f);

            auto& vel(mgr.addComponent<CVelocity>(e).value);
            setRndVec2(vel, 4.f);

            auto& acc(mgr.addComponent<CAcceleration>(e).value);
            setRndVec2(acc, 0.4f);

            auto& life(mgr.addComponent<CLife>(e).value);
            life = ssvu::getRndR(10.f, 130.f);

            auto& shape(mgr.addComponent<CRender>(e).shape);
            shape.setFillColor(sf::Color
            (
                ssvu::getRndI(150, 255),
                ssvu::getRndI(30, 70),
                ssvu::getRndI(30, 70),
                255
            ));
        }

        // Match entities that have both position and velocity,
        // then add velocity to their position.
        mgr.forEntitiesMatching<SApplyVelocity>
        ([mFT](auto, auto& cPosition, auto& cVelocity)
        {   
            // The components are "automatically" passed to the
            // lambda as parameters. The unused parameter is the
            // index of the entity. 

            cPosition.value += cVelocity.value * mFT;
        });

        mgr.forEntitiesMatching<SApplyAcceleration>
        ([mFT](auto, auto& cVelocity, auto& cAcceleration)
        {
            cVelocity.value += cAcceleration.value * mFT;
        });

        mgr.forEntitiesMatching<SRender>
        ([](auto, auto& cPosition, auto& cRender)
        {
            auto& s(cRender.shape);
            s.setPosition(cPosition.value);
        });

        mgr.forEntitiesMatching<SLife>
        ([mFT, this](auto e, auto& cLife)
        {
            cLife.value -= mFT;

            if(cLife.value <= 0)
                mgr.kill(e);
        });

        mgr.forEntitiesMatching<SGrow>
        ([](auto, auto& cRender, auto& cLife)
        {
            auto l(0.8f + (cLife.value * 0.3f));
            cRender.shape.setSize(Vec2f{l, l});
        });

        mgr.refresh();
    }

    void draw() override
    {
        mgr.forEntitiesMatching<SRender>
        ([this](auto, auto&, auto& cRender)
        {
            // NOTE: the explicit `this->` is required because of
            // a gcc bug.
            // See: http://stackoverflow.com/questions/32097759

            this->render(cRender.shape);
        });
    }
};
}

int main()
{
    Boilerplate::AppRunner<example::Game>{"ECS", 320, 200};
    return 0;
}

// In the next segment, we'll implement handles.