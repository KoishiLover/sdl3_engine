//
// @author 帽子屋小姐
// @date 2026/10/9
//

#ifndef HATTA_ENGINE_GAMEOBJECTMANAGER_HPP
#define HATTA_ENGINE_GAMEOBJECTMANAGER_HPP

#include<SDL3/SDL_stdinc.h>
#include<array>
#include<cassert>
#include <memory_resource>
#include<set>
#include"GameObject.hpp"
#include<vector>
#define OBJECT_POOL_SIZE  16384 // 对象池最大容量
#define OBJECT_GROUP_SIZE 16    // 碰撞组数

namespace engine {
    struct GameObjectLayerCompare {
        bool operator()(const GameObject* l ,const GameObject* r) const{
            if (l->layer != r->layer) {
                return l->layer < r->layer;
            }
            return l->uid < r->uid;
        }
    };
        //游戏对象更新对象的链表指针分配器
    struct GameObjectUpdateListFieldAccessor {
        static GameObject* getPrevious(GameObject const* const object) noexcept {
            return object->update_list_prev;
        }
        static GameObject* getNext(GameObject const* const object) noexcept {
            return object->update_list_next;
        }
        static void setNext(GameObject *const object, GameObject* value)noexcept {
            object->update_list_next = value;
        }
        static void setPrevious(GameObject *const object, GameObject* value)noexcept {
            object->update_list_prev = value;
        }
    };
    //游戏对象碰撞组的链表指针分配器
    struct GameObjectGroupListFieldAccessor {
        static GameObject* getPrevious(GameObject const* const object)noexcept {
            return object->group_list_prev;
        }
        static GameObject* getNext(GameObject const* const object) noexcept {
            return object->group_list_next;
        }
        static void setNext(GameObject *const object, GameObject* value)noexcept {
            object->group_list_next = value;
        }
        static void setPrevious(GameObject* const object , GameObject* value)noexcept {
            object->group_list_prev = value;
        }
    };
    //游戏对象的管理类
    template<typename FieldAccessor>
    class GameObjectList {
    private:
        GameObject* m_first {};
        GameObject* m_last {};
    public:
        [[nodiscard]] bool empty()const noexcept {
            return m_first == nullptr && m_last == nullptr;
        }
        void add(GameObject* object)noexcept {
            assert(object != nullptr);
            if (empty()) {
                m_first = object;
                m_last = object;
                FieldAccessor::setPrevious(object, nullptr);
                FieldAccessor::setNext(object, nullptr);
            }
            else {
                auto const last = m_last;
                FieldAccessor::setPrevious(object ,last);
                FieldAccessor::setNext(last ,object);
                m_last = object;
            }
        }
        GameObject* remove(GameObject* object) noexcept {
            assert(object != nullptr);
            assert(!empty());
            if (m_first == object && m_last == object) {
                m_first = nullptr;
                m_last = nullptr;
                assert(FieldAccessor::getPrevious(object) == nullptr);
                assert(FieldAccessor::getNext(object) == nullptr);
                return nullptr;
            }
            auto const prev = FieldAccessor::getPrevious(object);
            auto const next = FieldAccessor::getNext(object);
            if (prev != nullptr) {
                FieldAccessor::setNext(prev , next);
            }
            if (next != nullptr) {
                FieldAccessor::setPrevious(next,prev);
            }
            FieldAccessor::setPrevious(object , nullptr);
            FieldAccessor::setNext(object , nullptr);
            if (m_first == object) {
                m_first = next ;
            }
            if (m_last == object) {
                m_last = prev;
            }
            return next;
        }
        [[nodiscard]] GameObject* getFirst()const noexcept {
            return m_first;
        }
        void clear() noexcept {
            GameObject* object = m_first;
            while (object) {
                GameObject* const current =object;
                object = FieldAccessor::getNext(object);
                FieldAccessor::setNext(current , nullptr);
                FieldAccessor::setPrevious(current,nullptr);
            }
            m_first = m_last = nullptr;
        }
    };
    using GameObjectUpdateList = GameObjectList<GameObjectUpdateListFieldAccessor>;
    using GameObjectGroupList = GameObjectList<GameObjectGroupListFieldAccessor>;

    template<typename T , size_t Size>
    class GameObjectPool {
        T m_pool[Size];
        bool used_ids[Size];
        size_t free_ids[Size];
        size_t free_count;
    public:
        void alloc(size_t& id)  noexcept {
            if (free_count > 0) {
                --free_count;
                id = free_ids[free_count];
                used_ids[id] = true;
                return;
            }
            id = static_cast<size_t>(-1);
        }
        bool free(size_t id ) noexcept {
            if (id >= Size || !used_ids[id]) {
                return false;
            }
            used_ids[id] = false;
            free_ids[free_count] = id;
            ++free_count;
            return true;
        }
        T* object(size_t id) {
            if (id >= Size || !used_ids[id]) {
                return nullptr;
            }
            return &m_pool[id];
        }
        void reset()noexcept {
            for (size_t i = 0; i < Size; ++i) {
                free_ids[i] = Size-1-i;
            }
            free_count = Size;
            for (size_t i = 0; i < Size; ++i) {
                used_ids[i] = false;
            }
        }

        GameObjectPool() noexcept {reset();}
        ~GameObjectPool()  = default;
        GameObjectPool(GameObjectPool const&) = delete;
        GameObjectPool& operator=(GameObjectPool const&) = delete;
        GameObjectPool(GameObjectPool&&) = delete;
        GameObjectPool& operator=(GameObjectPool&&) = delete;
    };

    class GameObjectManager {
    private:
        //==== 数据成员（全部私有，仅通过公开接口访问）====

        ///@brief 对象全局唯一标识计数器（创建对象时自增并赋给 uid）
        Uint64 m_uid = 0;

        ///@brief pmr 内存池：渲染集节点与碰撞缓存共用，避免频繁堆分配
        std::pmr::unsynchronized_pool_resource m_memory;

        ///@brief 有序渲染集（按 layer，再按 uid）
        std::pmr::set<GameObject* ,GameObjectLayerCompare> m_render_set{&m_memory};

        ///@brief 碰撞组链表（组 = 会互相碰撞的一层）
        std::array<GameObjectGroupList ,OBJECT_GROUP_SIZE> m_group_list;

        ///@brief 更新链表（遍历顺序即更新顺序）
        GameObjectUpdateList m_update_list;

        ///@brief 对象池
        GameObjectPool<GameObject , OBJECT_POOL_SIZE> m_objects_pool;

        //舞台边界（y 轴向上：下界 <= 上界）
        double m_bound_l = 0.0 , m_bound_r = 0.0 , m_bound_t = 0.0 , m_bound_b = 0.0;

        ///@brief 碰撞判定收集的命中对（判定与回调分离，避免回调重入破坏遍历）
        struct CollisionPairs {
            GameObject* object1;
            GameObject* object2;
        };
        std::pmr::vector<CollisionPairs> m_cache_pairs{&m_memory};

        //特殊暂停计数
        Uint64 m_special_pause = 0;
        Uint64 m_next_special_pause = 0;

        bool m_initialized = false;

        //==== 内部实现（私有）====

        ///@brief 清空更新链表 / 渲染集 / 碰撞组链表 / 碰撞缓存
        void resetGameObjectsList() noexcept;
        ///@brief 执行 kill 回调并回收对象
        GameObject* freeGameObjectWithCallback(GameObject* const object) noexcept;
        ///@brief 直接回收对象（不执行回调）
        GameObject* freeGameObject(GameObject* const object) noexcept;
        ///@brief 每帧递减暂停计数，返回本帧剩余暂停帧数（仅在 UpdateXY 中调用一次）
        [[nodiscard]] Uint64 UpdateNextSpecialPause();

    public:
        //==== 生命周期 ====
        GameObjectManager() = default;
        GameObjectManager(GameObjectManager const&) = delete;
        GameObjectManager& operator=(GameObjectManager const&) = delete;
        GameObjectManager(GameObjectManager&&) = delete;
        GameObjectManager& operator=(GameObjectManager&&) = delete;
        ~GameObjectManager() = default;

        static GameObjectManager& getInstance() noexcept {
            static GameObjectManager instance;
            return instance;
        }

        ///@brief 初始化（可重复调用）
        void Init() noexcept;
        ///@brief 清空所有对象并重置整个系统（关卡切换用）
        void reset() noexcept;

        //==== 查询 ====
        ///@brief 对象指针是否仍然存活
        static bool isValid(const GameObject* object) noexcept {
            return object != nullptr && object->status == GameObjectStatus::Active;
        }

        //==== 舞台边界 ====
        void SetBound(double l ,double r,double t,double b)  {
            assert(l <= r && b <= t);
            m_bound_l = l;
            m_bound_r = r;
            m_bound_t = t;
            m_bound_b = b;
        }
        [[nodiscard]] bool isInBound(const GameObject* object) const noexcept {
            return  object->x <= m_bound_r
                    && object->x >= m_bound_l
                    && object->y <= m_bound_t
                    && object->y >= m_bound_b;
        }

        //==== 暂停 ====
        ///@brief 请求特殊暂停 frames 帧
        void SetSpecialPause(Uint64 time) noexcept;
        ///@brief 当前剩余暂停帧数
        [[nodiscard]] Uint64 GetSpecialPause() const noexcept;

        //==== 对象管理 ====
        void setGroup(GameObject* object , Uint8 group);
        void setLayer(GameObject* object , double layer);

        //==== 帧驱动（引擎每帧按顺序调用）====
        //执行所有 beforeUpdate 回调
        void beforeUpdateCallback();
        //执行所有 update 回调
        void updateCallback();
        //执行所有 afterUpdate 回调
        void afterUpdateCallback();
        //更新所有坐标
        void UpdateXY()noexcept;
        //更新所有上一帧信息以及计时器，并回收被标记的对象
        void UpdateLast()noexcept;
        //渲染所有对象
        void Render();
        //出界检测
        void BoundCheck();

        //==== 碰撞 ====
        //对两个组别进行碰撞检测
        void CollisionCheck(const Uint8 group1 , const Uint8 group2 ) noexcept;
        //执行碰撞回调
        void CollisionCallback();
    };
#define IMGR GameObjectManager::getInstance()
}

#endif //HATTA_ENGINE_GAMEOBJECTMANAGER_HPP
