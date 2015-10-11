// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <memory>
#include "../Other/Other.hpp"

// In this segment we'll implement the simple shoot'em'up game using
// our new component-based entity system.

namespace ecs
{
ECS_STRONG_TYPEDEF(std::size_t, DataIndex);
ECS_STRONG_TYPEDEF(std::size_t, EntityIndex);
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
    HandleDataIndex handleDataIndex;
    Bitset bitset;
    bool alive;
};

struct HandleData
{
    EntityIndex entityIndex;
    Counter counter;
};

struct Handle
{
    HandleDataIndex handleDataIndex;
    Counter counter;
};

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
    std::vector<HandleData> handleData;

    void growTo(std::size_t mNewCapacity)
    {
        assert(mNewCapacity > capacity);

        entities.resize(mNewCapacity);
        components.grow(mNewCapacity);
        handleData.resize(mNewCapacity);

        for(auto i(capacity); i < mNewCapacity; ++i)
        {
            auto& e(entities[i]);
            auto& h(handleData[i]);

            e.dataIndex = i;
            e.bitset.reset();
            e.alive = false;
            e.handleDataIndex = i;

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

    auto& getHandleData(HandleDataIndex mI) noexcept
    {
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

    auto isHandleValid(const Handle& mX) const noexcept
    {
        return mX.counter == getHandleData(mX).counter;
    }

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

    auto createHandle()
    {
        auto freeIndex(createIndex());
        assert(isAlive(freeIndex));

        auto& e(entities[freeIndex]);
        auto& hd(handleData[e.handleDataIndex]);
        hd.entityIndex = freeIndex;

        Handle h;
        h.handleDataIndex = e.handleDataIndex;
        h.counter = hd.counter;
        assert(isHandleValid(h));

        return h;
    }

    void clear() noexcept
    {
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
    void invalidateHandle(EntityIndex mX) noexcept
    {
        auto& hd(handleData[entities[mX].handleDataIndex]);
        ++hd.counter;
    }

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
            }

            for(; true; --iA)
            {
                if(entities[iA].alive) break;
                invalidateHandle(iA);
                if(iA <= iD) return iD;
            }

            assert(entities[iA].alive);
            assert(!entities[iD].alive);

            std::swap(entities[iA], entities[iD]);

            refreshHandle(iD);

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

// This was our original game hierarchy:
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

// Let's transform it into something "component-oriented".

// The game objects we're gonna have are:
// * Player
// * Boss
// * Bullet
// * PlayerBullet

// Some considerations:
//
// * All of the objects will have a position.
//
// * All of the objects will have a velocity.
//
// * Bullets and player bullets will have acceleration.
//
// * Bullets and player bullets will get destroyed automatically
//   after some time.
//
// * The player and the boss will be rendered using rectangles.
//
// * The bullets and the player bullets will be rendered using
//   circles.
//
// * The boss will be damaged by player bullets.
//
// * The player will be damaged by bullets.
//
// * The boss will horizontally move towards the player.
//

// We can take care of the first four consideration by using
// `CPosition`, `CVelocity`, `CAcceleration` and `CLife` components.
// We'll need a signature to apply velocity, one to apply
// acceleration, one to continuously decrement life.

// We can deal with the rendering by adding `CShapeRectangle` and
// `CShapeCircle` components.
// We'll "attach" shapes to objects by using signatures that require
// a render component and a position component.

// For simplicity, we'll use a `CCircleHitbox` component to implement
// collision. We'll assume all our game objects' hitboxes are circles.

// We'll give every game object its own tag - this way we'll be able
// to match specific game object in signatures and use "nested loops"
// to make objects interact.

// The player keyboard controls and the boss AI will be also handled
// thanks to tags.

namespace example
{
// The component types we previously defined for the
// particle simulation will be useful.
struct CPosition { Vec2f value; };
struct CVelocity { Vec2f value; };
struct CAcceleration { Vec2f value; };

// "Life" will always automatically decrement.
struct CLife { float value; };

// "Health" will decrement when an entity "takes damage".
struct CHealth { float value; };

// A "cooldown" is a timer for entity actions. It will be used to
// periodically have the player and the boss shoot bullets.
struct CCooldown { float value, maxValue; };

// The rendering components will be simple wrappers
// for SFML shape classes.
struct CShapeRectangle { sf::RectangleShape shape; };
struct CShapeCircle { sf::CircleShape shape; };

// The collision component will simply store the
// effective radius of the object.
struct CCircleHitbox { float radius; };

using MyComponents = ecs::ComponentList
<
    CPosition, CVelocity, CAcceleration, CLife, CHealth, CCooldown,
    CShapeRectangle, CShapeCircle,
    CCircleHitbox
>;

// We'll need tags for every game object in this situation,
// as all of them have special interactions between each other.
struct TPlayer { };
struct TBoss { };
struct TBullet { };
struct TPlayerBullet { };

using MyTags = ecs::TagList
<
    TPlayer, TBoss, TBullet, TPlayerBullet
>;

using SApplyVelocity = ecs::Signature<CPosition, CVelocity>;
using SApplyAcceleration = ecs::Signature<CVelocity, CAcceleration>;
using SLife = ecs::Signature<CLife>;
using SHealth = ecs::Signature<CHealth>;

using SRenderRectangle = ecs::Signature<CPosition, CShapeRectangle>;
using SRenderCircle = ecs::Signature<CPosition, CShapeCircle>;

using SPlayer = ecs::Signature
    <TPlayer, CPosition, CVelocity, CCircleHitbox, CCooldown>;

using SBoss = ecs::Signature
    <TBoss, CPosition, CVelocity, CCircleHitbox, CHealth, CCooldown>;

// Bullet signatures only differ because of their tags.
using SBullet = ecs::Signature
    <TBullet, CPosition, CCircleHitbox>;

using SPlayerBullet = ecs::Signature
    <TPlayerBullet, CPosition, CCircleHitbox>;

using MySignatures = ecs::SignatureList
<
    SApplyVelocity, SApplyAcceleration, SLife, SHealth,
    SRenderRectangle, SRenderCircle,
    SPlayer, SBoss, SBullet, SPlayerBullet
>;

using MySettings = ecs::Settings
<
    MyComponents, MyTags, MySignatures
>;

using MyManager = ecs::Manager<MySettings>;

struct Game : Boilerplate::TestApp
{
    MyManager mgr;

    // Entity creation during "updates" can cause the internal manager
    // storage to reallocate. Therefore, we use a vector of generic
    // type-erased functions to store our entity creation actions and
    // execute them after the "update" and before the "refresh".
    std::vector<std::function<void()>> beforeRefresh;

    auto mkPlayer()
    {
        auto e(mgr.createIndex());
        mgr.addTag<TPlayer>(e);

        auto& pos(mgr.addComponent<CPosition>(e).value);
        auto& vel(mgr.addComponent<CVelocity>(e).value);
        auto& hitbox(mgr.addComponent<CCircleHitbox>(e).radius);
        auto& shape(mgr.addComponent<CShapeRectangle>(e).shape);
        auto& cooldown(mgr.addComponent<CCooldown>(e));

        pos = {320.f / 2.f, 240.f - 70.f};
        vel = {0.f, 0.f};
        hitbox = 2.5f;
        shape.setFillColor(sf::Color::Green);
        Vec2f size{4.f, 6.5f};
        shape.setSize(size);
        shape.setOrigin(size / 2.f);
        cooldown.value = cooldown.maxValue = 1.6f;

        return e;
    }

    auto mkBoss()
    {
        auto e(mgr.createIndex());
        mgr.addTag<TBoss>(e);

        auto& pos(mgr.addComponent<CPosition>(e).value);
        auto& vel(mgr.addComponent<CVelocity>(e).value);
        auto& hitbox(mgr.addComponent<CCircleHitbox>(e).radius);
        auto& shape(mgr.addComponent<CShapeRectangle>(e).shape);
        auto& health(mgr.addComponent<CHealth>(e).value);
        auto& cooldown(mgr.addComponent<CCooldown>(e));

        pos = {320.f / 2.f, 40.f};
        vel = {0.f, 0.f};
        hitbox = 5.5f;
        shape.setFillColor(sf::Color::Magenta);
        Vec2f size{6.f, 9.5f};
        shape.setSize(size);
        shape.setOrigin(size / 2.f);
        health = 100.f;
        cooldown.value = cooldown.maxValue = 70.f;

        return e;
    }

    auto mkBullet(const Vec2f& mPos)
    {
        auto e(mgr.createIndex());
        mgr.addTag<TBullet>(e);

        auto& pos(mgr.addComponent<CPosition>(e).value);
        auto& vel(mgr.addComponent<CVelocity>(e).value);
        auto& acc(mgr.addComponent<CAcceleration>(e).value);
        auto& hitbox(mgr.addComponent<CCircleHitbox>(e).radius);
        auto& shape(mgr.addComponent<CShapeCircle>(e).shape);
        auto& life(mgr.addComponent<CLife>(e).value);

        pos = mPos;
        vel = {0.f, 0.f};
        acc = {ssvu::getRndR(-0.01f, 0.01f),
            ssvu::getRndR(-0.01f, 0.02f)};
        hitbox = 0.5f;
        shape.setFillColor(sf::Color::Red);
        shape.setRadius(1.1f);
        life = 500.f;

        return e;
    }

    auto mkPlayerBullet(const Vec2f& mPos)
    {
        auto e(mgr.createIndex());
        mgr.addTag<TPlayerBullet>(e);

        auto& pos(mgr.addComponent<CPosition>(e).value);
        auto& vel(mgr.addComponent<CVelocity>(e).value);
        auto& acc(mgr.addComponent<CAcceleration>(e).value);
        auto& hitbox(mgr.addComponent<CCircleHitbox>(e).radius);
        auto& shape(mgr.addComponent<CShapeCircle>(e).shape);
        auto& life(mgr.addComponent<CLife>(e).value);

        pos = mPos;
        vel = {0.f, -6.f};
        acc = {0.f, 0.f};
        hitbox = 0.5f;
        shape.setFillColor(sf::Color::Cyan);
        shape.setRadius(1.1f);
        life = 200.f;

        return e;
    }

    Game(ssvs::GameWindow& mX) : Boilerplate::TestApp{mX}
    {
        onTxtInfoUpdate += [this](auto& oss, FT)
        {
            oss << "Entities: " << mgr.getEntityCount() << "\n";
        };

        // The game begins by creating a boss and a player.
        mkBoss();
        mkPlayer();
    }

    // Helper function that checks if two circles intersect.
    auto isCollision
    (
        const CPosition& mP0, const CCircleHitbox& mC0,
        const CPosition& mP1, const CCircleHitbox& mC1
    )
    {
        auto dist(ssvs::getDistEuclidean(mP0.value, mP1.value));
        return dist <= (mC0.radius + mC1.radius);
    }

    // Helper function that queues a specific action if an entity is
    // ready to perform it. Used for bullet spawning.
    template<typename TFunction>
    void cooldownAction(FT mFT, CCooldown& mCD, TFunction&& mFunction)
    {
        mCD.value -= mFT;
        if(mCD.value > 0.f) return;

        beforeRefresh.emplace_back(mFunction);
        mCD.value = mCD.maxValue;
    }

    // Helper function that manages input for movement and shooting.
    void updatePlayerInput(FT mFT, CPosition& pPos, CVelocity& pVel,
        CCooldown& pCD)
    {
        constexpr float speed{2.f};
        Vec2f xyInput;
        bool shoot;

        using SFK = sf::Keyboard;

        // Get horizontal movement.
        if(SFK::isKeyPressed(SFK::Key::Left)) xyInput.x = -1;
        else if(SFK::isKeyPressed(SFK::Key::Right)) xyInput.x = 1;

        // Get vertical movement.
        if(SFK::isKeyPressed(SFK::Key::Up)) xyInput.y = -1;
        else if(SFK::isKeyPressed(SFK::Key::Down)) xyInput.y = 1;

        // Get shooting state.
        shoot = SFK::isKeyPressed(SFK::Key::Z);

        // Calculate velocity vector.
        auto radians(ssvs::getRad(xyInput));
        pVel.value = ssvs::getVecFromRad(radians)
            * (speed * ssvs::getMag(xyInput));

        // Enqueue shooting action.
        cooldownAction(mFT, pCD, [this, shoot, pPos]
        {
            if(!shoot) return;
            this->mkPlayerBullet(pPos.value);
        });
    }

    void update(FT mFT) override
    {
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

        mgr.forEntitiesMatching<SRenderRectangle>
        ([](auto, auto& cPosition, auto& cShapeRectangle)
        {
            cShapeRectangle.shape.setPosition(cPosition.value);
        });

        mgr.forEntitiesMatching<SRenderCircle>
        ([](auto, auto& cPosition, auto& cShapeCircle)
        {
            cShapeCircle.shape.setPosition(cPosition.value);
        });

        mgr.forEntitiesMatching<SLife>
        ([mFT, this](auto e, auto& cLife)
        {
            cLife.value -= mFT;
            if(cLife.value <= 0) mgr.kill(e);
        });

        mgr.forEntitiesMatching<SHealth>
        ([mFT, this](auto e, auto& cHealth)
        {
            if(cHealth.value <= 0) mgr.kill(e);
        });

        // Player logic.
        mgr.forEntitiesMatching<SPlayer>
        ([this, mFT](auto pI, auto& pPos, auto& pVel, auto& pHitbox,
            auto& pCD)
        {
            this->updatePlayerInput(mFT, pPos, pVel, pCD);

            // Player/Boss logic.
            // If there is a player and a boss, the boss will move
            // towards the player and try to shoot.
            mgr.forEntitiesMatching<SBoss>
            ([this, mFT, &pPos](auto, auto& bssPos, auto& bssVel,
                auto&, auto&, auto& bssCD)
            {
                constexpr auto speed(0.7f);
                auto left(pPos.value.x < bssPos.value.x);
                bssVel.value.x = left ? -speed : speed;

                this->cooldownAction(mFT, bssCD, [this, bssPos]
                {
                    for(auto i(0u); i < 70; ++i)
                        this->mkBullet(bssPos.value);
                });
            });

            // Player/Bullet logic.
            // Checks collisions between the player and enemy bullets.
            mgr.forEntitiesMatching<SBullet>
            ([this, pI, &pPos, &pHitbox](auto, auto& bltPos,
                auto& bltHitbox)
            {
                if(this->isCollision(pPos, pHitbox, bltPos, bltHitbox))
                    mgr.kill(pI);
            });
        });
    
        // PlayerBullet logic.
        mgr.forEntitiesMatching<SPlayerBullet>
        ([this](auto pbI, auto& pbPos, auto& pbHitbox)
        {   
            // PlayerBullet/Boss logic.
            // Checks collisions between the boss and player bullets.
            mgr.forEntitiesMatching<SBoss>
            ([this, pbI, &pbPos, &pbHitbox](auto, auto& bssPos, auto&,
                auto& bssHitbox, auto& bssHealth, auto&)
            {
                if(this->isCollision(pbPos, pbHitbox,
                    bssPos, bssHitbox))
                {
                    bssHealth.value -= 1;
                    mgr.kill(pbI);
                }
            });
        });

        for(auto& f : beforeRefresh) f();
        beforeRefresh.clear();

        mgr.refresh();
    }

    void draw() override
    {
        mgr.forEntitiesMatching<SRenderRectangle>
        ([this](auto, auto&, auto& cRender)
        {
            this->render(cRender.shape);
        });

        mgr.forEntitiesMatching<SRenderCircle>
        ([this](auto, auto&, auto& cRender)
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

// That's it!

// The implementation above is a simple example of component-based
// design for games and applications.

// Let's go back to the slides for some "food for thought".