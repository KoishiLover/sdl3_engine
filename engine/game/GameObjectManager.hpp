//
// @author 帽子屋小姐
// @date 2026/10/9
//

#ifndef HATTA_ENGINE_GAMEOBJECTMANAGER_HPP
#define HATTA_ENGINE_GAMEOBJECTMANAGER_HPP

#include<SDL3/SDL_stdinc.h>
#include<array>
#include<set>
#include"GameObject.hpp"

#define OBJECT_POOL_SIZE  16384 // 对象池最大容量
#define OBJECT_GROUP_SIZE 16    // 碰撞组数

namespace engine {
    //当图层一致时比较 全局唯一标识，即后生成的object先绘制
    struct GameObjectLayerCompare {
        bool operator()(GameObject* const& left , GameObject* const& right)const noexcept {
            if (left->layer != right->layer) {
                return left->layer < right->layer;
            }
            return left->uid < right->uid;
        }
    };
    //游戏对象更新对象的链表指针分配器
    struct GameObjectUpdateListFiledAccessor {
        static GameObject* getPrevious(GameObject const* const object) noexcept {
            return object->update_list_prev;
        }
        static GameObject* getNext(GameObject const* const object) noexcept {
            return object->update_list_next;
        }
        static void setNext(GameObject *const object, GameObject* value)noexcept {
            object->update_list_next = value;
        }
    };
    //游戏对象碰撞组的链表指针分配器
    struct GameObjectGroupListFiledAccessor {
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
    template<typename FiledAccessor>
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
                FiledAccessor::setPrevious(object, nullptr);
                FiledAccessor::setNext(object, nullptr);
            }
            else {
                auto const last = m_last;
                FiledAccessor::setPrevious(object ,last);
                FiledAccessor::setNext(last ,object);
                m_last = object;
            }
        }
        GameObject* remove(GameObject* object) noexcept {
            assert(object != nullptr);
            assert(!empty());
            if (m_first == object && m_last == object) {
                m_first = nullptr;
                m_last = nullptr;
                assert(FiledAccessor::getPrevious(object) == nullptr);
                assert(FiledAccessor::getNext(object) == nullptr);
                return nullptr;
            }
            auto const prev = FiledAccessor::getPrevious(object);
            auto const next = FiledAccessor::getNext(object);
            if (prev != nullptr) {
                FiledAccessor::setNext(prev , next);
            }
            if (next != nullptr) {
                FiledAccessor::setPrevious(next,prev);
            }
            FiledAccessor::setPrevious(object , nullptr);
            FiledAccessor::setNext(object , nullptr);
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
                object = FiledAccessor::getNext(object);
                FiledAccessor::setNext(current , nullptr);
                FiledAccessor::setPrevious(current,nullptr);
            }
        }
    };

    using GameObjectUpdateList = GameObjectList<GameObjectUpdateListFiledAccessor>;
    using GameObjectGroupList = GameObjectList<GameObjectGroupListFiledAccessor>;

    class GameObjectManager {

    public:
        static Uint64 unique_id;

        std::array<GameObject* ,OBJECT_POOL_SIZE> m_GameObjectsPool;
        std::set<GameObject*,GameObjectLayerCompare> m_RenderGroup;
        std::array<GameObject* ,OBJECT_GROUP_SIZE> m_Groups;

    public:
        GameObjectManager();
        GameObjectManager(const GameObjectManager&) = delete;
        GameObjectManager& operator=(const GameObjectManager&) = delete;
        ~GameObjectManager();

    };
}

#endif //HATTA_ENGINE_GAMEOBJECTMANAGER_HPP
