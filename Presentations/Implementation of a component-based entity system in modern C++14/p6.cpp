// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <memory>
#include "../Other/Other.hpp"

// In this code segment we'll implement handles.

// Handles create an additional layer of indirection between the
// user and a specific entity.

// They can be used to keep track of a particular entity, to check
// whether or not it has been destroyed, or to access it after the
// manager has been refreshed.

// Handles are going to be implemented with an additional auxiliary
// contiguous container that will be as big as the entity metadata
// container.

// Every element of the handle data container will have an index
// pointing to the correct entity (this index will be updated on
// `refresh()` calls) and a validity counter that will make sure
// that the pointed entity is the intended one.

// The counter is required because an entity may take the place of
// another entity after a `refresh()`.

// {image: p6_d0}

namespace ecs
{
ECS_STRONG_TYPEDEF(std::size_t, DataIndex);
ECS_STRONG_TYPEDEF(std::size_t, EntityIndex);

// We're going to need two additional strong typedefs:
// * One for indices of handle data (auxiliary array indices).
// * One for the type of handle counter.

ECS_STRONG_TYPEDEF(std::size_t, HandleDataIndex);
ECS_STRONG_TYPEDEF(int, Counter);

namespace Impl
{
template<typename TSettings>
struct Entity
{
    using Settings = TSettings;
    using Bitset = typename Settings::Bitset;

    DataIndex dataIndex;

    // Entities must be aware of what handle is pointing
    // to them, so that the handle can be updated if the
    // entity gets moved around after a `refresh()`.
    HandleDataIndex handleDataIndex;

    Bitset bitset;
    bool alive;
};

// Our "handle data" class will be a simple pair
// of `EntityIndex` and `Counter`.
struct HandleData
{
    EntityIndex entityIndex;
    Counter counter;
};

// And our actual `Handle` class will be a simple pair
// of `HandleDataIndex` and `Counter`.
struct Handle
{
    HandleDataIndex handleDataIndex;
    Counter counter;
};

// To access an entity through an handle, these steps
// will occur:
//
// 1. The corresponding `HandleData` is retrieved,
//    using the `handleDataIndex` stored in the handle.
//
// 2. If the handle's counter does not match the handle
//    data's counter, then the handle is not valid anymore.
//
// 3. Otherwise, the entity will be retrieved through the
//    handle data's `entityIndex` member.
//

template<typename TSettings>
class ComponentStorage
{
    private:
        using Settings = TSettings;
        using ComponentList = typename Settings::ComponentList;

        template<typename... Ts>
        using TupleOfVectors = std::tuple<std::vector<Ts>...>;

        MPL::Rename<TupleOfVectors, ComponentList> vectors;

    public:
        void grow(std::size_t mNewCapacity)
        {
            Utils::forTuple([this, mNewCapacity](auto& v)
            {
                v.resize(mNewCapacity);
            }, vectors);
        }

        template<typename T>
        auto& getComponent(DataIndex mI) noexcept
        {
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
    using HandleData = Impl::HandleData;

public:
    using Handle = Impl::Handle;

private:
    using SignatureBitsetsStorage =
        Impl::SignatureBitsetsStorage<Settings>;
    using ComponentStorage = Impl::ComponentStorage<Settings>;

    std::size_t capacity{0}, size{0}, sizeNext{0};
    std::vector<Entity> entities;
    SignatureBitsetsStorage signatureBitsets;
    ComponentStorage components;

    // Handle data will be stored in a vector, like entities.
    std::vector<HandleData> handleData;

    void growTo(std::size_t mNewCapacity)
    {
        assert(mNewCapacity > capacity);

        entities.resize(mNewCapacity);
        components.grow(mNewCapacity);

        // Do not forget to grow the new container.
        handleData.resize(mNewCapacity);

        for(auto i(capacity); i < mNewCapacity; ++i)
        {
            auto& e(entities[i]);
            auto& h(handleData[i]);

            e.dataIndex = i;
            e.bitset.reset();
            e.alive = false;

            // New entities will need to know what their
            // handle is. During initialization, it will
            // be the handle "directly below them".
            e.handleDataIndex = i;

            // New handle data instances will have to
            // be initialized with a value for their counter
            // and the index of the entity they're pointing
            // at (which will be the one "directly on top of
            // them", at that point in time).

            // Set handledata values.
            h.counter = 0;
            h.entityIndex = i;
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

    // We'll need some getters for `handleData`.
    // We can get `HandleData` instances in three ways:
    // * Through an `HandleDataIndex`.
    // * Through an `EntityIndex` -> `HandleDataIndex`.
    // * Through an `Handle` -> `HandleDataIndex`.

    // Some code repetition is necessary...

    auto& getHandleData(HandleDataIndex mI) noexcept
    {
        // The handle for an entity does not necessarily have to
        // be before the `sizeNext` index.
        assert(handleData.size() > mI);
        return handleData[mI];
    }
    const auto& getHandleData(HandleDataIndex mI) const noexcept
    {
        assert(handleData.size() > mI);
        return handleData[mI];
    }

    auto& getHandleData(EntityIndex mI) noexcept
    {
        return getHandleData(getEntity(mI).handleDataIndex);
    }
    const auto& getHandleData(EntityIndex mI) const noexcept
    {
        return getHandleData(getEntity(mI).handleDataIndex);
    }

    auto& getHandleData(const Handle& mX) noexcept
    {
        return getHandleData(mX.handleDataIndex);
    }
    const auto& getHandleData(const Handle& mX) const noexcept
    {
        return getHandleData(mX.handleDataIndex);
    }

public:
    Manager() { growTo(100); }

    // How to check if an handle is valid?
    // Comparing its counter to the corresponding handle data
    // counter is enough.
    auto isHandleValid(const Handle& mX) const noexcept
    {
        return mX.counter == getHandleData(mX).counter;
    }

    // All methods that we previously could call with `EntityIndex`
    // should also be possible to call using handles.

    // Let's create a method that returns an `EntityIndex` from
    // an handle valid to aid us.

    auto getEntityIndex(const Handle& mX) const noexcept
    {
        assert(isHandleValid(mX));
        return getHandleData(mX).entityIndex;
    }

    auto isAlive(EntityIndex mI) const noexcept
    {
        return getEntity(mI).alive;
    }
    auto isAlive(const Handle& mX) const noexcept
    {
        return isAlive(getEntityIndex(mX));
    }

    void kill(EntityIndex mI) noexcept
    {
        getEntity(mI).alive = false;
    }
    void kill(const Handle& mX) noexcept
    {
        kill(getEntityIndex(mX));
    }

    template<typename T>
    auto hasTag(EntityIndex mI) const noexcept
    {
        static_assert(Settings::template isTag<T>(), "");
        return getEntity(mI).bitset[Settings::template tagBit<T>()];
    }
    template<typename T>
    auto hasTag(const Handle& mX) const noexcept
    {
        return hasTag<T>(getEntityIndex(mX));
    }

    template<typename T>
    void addTag(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isTag<T>(), "");
        getEntity(mI).bitset[Settings::template tagBit<T>()] = true;
    }
    template<typename T>
    void addTag(const Handle& mX) noexcept
    {
        return addTag<T>(getEntityIndex(mX));
    }

    template<typename T>
    void delTag(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isTag<T>(), "");
        getEntity(mI).bitset[Settings::template tagBit<T>()] = false;
    }
    template<typename T>
    auto delTag(const Handle& mX) noexcept
    {
        return delTag<T>(getEntityIndex(mX));
    }

    template<typename T>
    auto hasComponent(EntityIndex mI) const noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");
        return getEntity(mI).bitset
            [Settings::template componentBit<T>()];
    }
    template<typename T>
    auto hasComponent(const Handle& mX) const noexcept
    {
        return hasComponent<T>(getEntityIndex(mX));
    }

    template<typename T, typename... TArgs>
    auto& addComponent(EntityIndex mI, TArgs&&... mXs) noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");

        auto& e(getEntity(mI));
        e.bitset[Settings::template componentBit<T>()] = true;

        auto& c(components.template getComponent<T>(e.dataIndex));
        new (&c) T(FWD(mXs)...);
        return c;
    }
    template<typename T, typename... TArgs>
    auto& addComponent(const Handle& mX, TArgs&&... mXs) noexcept
    {
        return addComponent<T>(getEntityIndex(mX), FWD(mXs)...);
    }

    // `getComponent` will simply return a reference to the
    // component, after asserting its existance.
    template<typename T>
    auto& getComponent(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");
        assert(hasComponent<T>(mI));

        return components.
            template getComponent<T>(getEntity(mI).dataIndex);
    }
    template<typename T>
    auto& getComponent(const Handle& mX) noexcept
    {
        return getComponent<T>(getEntityIndex(mX));
    }

    template<typename T>
    void delComponent(EntityIndex mI) noexcept
    {
        static_assert(Settings::template isComponent<T>(), "");
        getEntity(mI).bitset
            [Settings::template componentBit<T>()] = false;
    }
    template<typename T>
    void delComponent(const Handle& mX) noexcept
    {
        return delComponent<T>(getEntityIndex(mX));
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

    // If the user does not need to track a specific entity,
    // `createIndex()` can be used.

    // Otherwise, we'll need to create a new method that
    // returns an handle.

    auto createHandle()
    {
        // Let's start by creating an entity with
        // `createIndex()`, and storing the result.
        auto freeIndex(createIndex());
        assert(isAlive(freeIndex));

        // We'll need to "match" the new entity
        // and the new handle together.
        auto& e(entities[freeIndex]);
        auto& hd(handleData[e.handleDataIndex]);

        // Let's update the entity's corresponding
        // handle data to point to the new index.
        hd.entityIndex = freeIndex;

        // Initialize a valid entity handle.
        Handle h;

        // The handle will point to the entity's
        // handle data...
        h.handleDataIndex = e.handleDataIndex;

        // ...and its validity counter will be set
        // to the handle data's current counter.
        h.counter = hd.counter;

        // Assert entity handle validity.
        assert(isHandleValid(h));

        // Return a copy of the entity handle.
        return h;
    }

    void clear() noexcept
    {
        // Let's re-initialize handles during `clear()`.

        for(auto i(0u); i < capacity; ++i)
        {
            auto& e(entities[i]);
            auto& hd(handleData[i]);

            e.dataIndex = i;
            e.bitset.reset();
            e.alive = false;
            e.handleDataIndex = i;

            hd.counter = 0;
            hd.entityIndex = i;
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

    template<typename T>
    auto matchesSignature(EntityIndex mI) const noexcept
    {
        static_assert(Settings::template isSignature<T>(), "");

        const auto& entityBitset(getEntity(mI).bitset);
        const auto& signatureBitset(signatureBitsets.
            template getSignatureBitset<T>());

        return (signatureBitset & entityBitset) == signatureBitset;
    }

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
                expandSignatureCall<T>(i, mFunction);
        });
    }

private:
    template<typename... Ts>
    struct ExpandCallHelper;

    template<typename T, typename TF>
    void expandSignatureCall(EntityIndex mI, TF&& mFunction)
    {
        static_assert(Settings::template isSignature<T>(), "");

        using RequiredComponents = typename Settings::
            SignatureBitsets::template SignatureComponents<T>;

        using Helper = MPL::Rename
        <
            ExpandCallHelper, RequiredComponents
        >;

        Helper::call(mI, *this, mFunction);
    }

    template<typename... Ts>
    struct ExpandCallHelper
    {
        template<typename TF>
        static void call(EntityIndex mI, ThisType& mMgr,
            TF&& mFunction)
        {
            auto di(mMgr.getEntity(mI).dataIndex);

            mFunction
            (
                mI,
                mMgr.components.template getComponent<Ts>(di)...
            );
        }
    };

private:
    // We'll need to do something when iterating over dead
    // entities during `refreshImpl()` to make sure their
    // handles get invalidated.

    // Invalidating an handle is as simple as incrementing
    // its counter.
    void invalidateHandle(EntityIndex mX) noexcept
    {
        auto& hd(handleData[entities[mX].handleDataIndex]);
        ++hd.counter;
    }

    // We'll also need that swapped alive entities' handles
    // point to the new correct indices.

    // It is sufficient to get the handle data from the entity,
    // after it has been swapped, and update its entity index
    // with the new one.
    void refreshHandle(EntityIndex mX) noexcept
    {
        auto& hd(handleData[entities[mX].handleDataIndex]);
        hd.entityIndex = mX;
    }

    auto refreshImpl() noexcept
    {
        EntityIndex iD{0}, iA{sizeNext - 1};

        while(true)
        {
            for(; true; ++iD)
            {
                if(iD > iA) return iD;
                if(!entities[iD].alive) break;

                // There is no need to invalidate or refresh
                // handles of untouched alive entities.
            }

            for(; true; --iA)
            {
                if(entities[iA].alive) break;

                // New dead entities on the right need to be
                // invalidated. Their handle index doesn't need
                // to be changed.
                invalidateHandle(iA);

                if(iA <= iD) return iD;
            }

            assert(entities[iA].alive);
            assert(!entities[iD].alive);

            std::swap(entities[iA], entities[iD]);

            // After swap, the alive entity's handle must be
            // refreshed, but not invalidated.
            refreshHandle(iD);

            // After swap, the dead entity's handle must be
            // both refreshed and invalidated.
            invalidateHandle(iA);
            refreshHandle(iA);

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

namespace example
{
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

using MyTags = ecs::TagList<>;

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

    // Let's test handles by keeping track of particles with even
    // index.
    std::vector<MyManager::Handle> trackedParticles;

    Game(ssvs::GameWindow& mX) : Boilerplate::TestApp{mX}
    {
        onTxtInfoUpdate += [this](auto& oss, FT)
        {
            oss << "Entities: " << mgr.getEntityCount() << "\n";
            oss << "Tracked: " << trackedParticles.size() << "\n";
        };
    }

    void update(FT mFT) override
    {
        for(auto i(0u); i < 40; ++i)
        {
            auto e(mgr.createHandle());

            auto setRndVec2([](auto& mVec, auto mX)
            {
                mVec.x = ssvu::getRndR(-mX, mX);
                mVec.y = ssvu::getRndR(-mX, mX);
            });

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

            if(i % 2 == 0)
            {
                trackedParticles.emplace_back(e);
            }
        }

        mgr.forEntitiesMatching<SApplyVelocity>
        ([mFT](auto, auto& cPosition, auto& cVelocity)
        {
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
            {
                mgr.kill(e);
            }
        });

        mgr.forEntitiesMatching<SGrow>
        ([](auto, auto& cRender, auto& cLife)
        {
            auto l(0.8f + (cLife.value * 0.3f));
            cRender.shape.setSize(Vec2f{l, l});
        });

        // "Erase-remove_if" all invalid handles.
        trackedParticles.erase
        (
            std::remove_if
            (
                std::begin(trackedParticles),
                std::end(trackedParticles),
                [this](auto& h){ return !mgr.isHandleValid(h); }
            ),
            std::end(trackedParticles)
        );

        // Make all tracked particles move towards east.
        for(auto& h : trackedParticles)
        {
            auto& vel(mgr.getComponent<CVelocity>(h).value);
            auto& acc(mgr.getComponent<CAcceleration>(h).value);

            vel.x = 30.f;
            vel.y = acc.x = acc.y = 0.f;
        }

        mgr.refresh();
    }

    void draw() override
    {
        mgr.forEntitiesMatching<SRender>([this](auto, auto&,
            auto& cRender)
        {
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

// In the next code segment we'll re-implement our simple shoot-em-up
// game using our entity-component system.