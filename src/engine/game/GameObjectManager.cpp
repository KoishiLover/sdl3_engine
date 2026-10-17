//
// @author 帽子屋小姐
// @date 2026/10/9
//

#include "GameObjectManager.hpp"
#include "../engine_log.hpp"
#include <cmath>
namespace engine {
    namespace {
        ///@brief 两个对象是否重叠：双方都用矩形判定时做 AABB，否则一律按圆判定
        bool objectsOverlap(const GameObject* a ,const GameObject* b) noexcept {
            const double dx = a->x - b->x;
            const double dy = a->y - b->y;
            if (a->rect && b->rect) {
                return std::abs(dx) <= (a->rect_a + b->rect_a)
                    && std::abs(dy) <= (a->rect_b + b->rect_b);
            }
            const double r = a->col_r + b->col_r;
            return (dx * dx + dy * dy) <= (r * r);
        }
    }

    void GameObjectManager::Init() noexcept{
        if (!m_initialized) {
            m_initialized = true;
            resetGameObjectsList();
        }
    }
    void GameObjectManager::SetSpecialPause(Uint64 time) noexcept {
        m_special_pause = time;
        m_next_special_pause = time;
    }
    Uint64 GameObjectManager::GetSpecialPause() const noexcept {
        return m_special_pause;
    }
    Uint64 GameObjectManager::UpdateNextSpecialPause() {
        m_special_pause = m_next_special_pause;
        if (m_next_special_pause > 0) {
            --m_next_special_pause;
        }
        return m_special_pause;
    }
    void GameObjectManager::reset() noexcept {
        m_bound_l = m_bound_r =m_bound_t = m_bound_b =0.0;
        m_special_pause = m_next_special_pause = 0;
        for (auto p = m_update_list.getFirst(); p != nullptr;) {
            p = freeGameObject(p);
        }
        resetGameObjectsList();
        m_objects_pool.reset();
        m_memory.release();
    }

    void GameObjectManager::resetGameObjectsList() noexcept {
        m_update_list.clear();
        m_render_set.clear();
        for (auto& list:m_group_list) {
            list.clear();
        }
        m_cache_pairs.clear();
    }
    GameObject* GameObjectManager::freeGameObjectWithCallback(GameObject* const object) noexcept {
        if (object->callbacks.kill.valid()) {
            auto r = object->callbacks.kill(object);
            if (!r.valid()) {
                sol::error err = r;
                EngineLogger::err("[lua]  调用回调函数 Kill 时出错 : {}",err.what());
            }
        }
        auto const next = m_update_list.remove(object);
        m_render_set.erase(object);
        m_group_list[object->group].remove(object);
        object->status = GameObjectStatus::Free;
        m_objects_pool.free(object->id);
        object->Reset();
        return next;
    }
    GameObject* GameObjectManager::freeGameObject(GameObject* const object) noexcept {
        auto const next = m_update_list.remove(object);
        m_render_set.erase(object);
        m_group_list[object->group].remove(object);
        object->status = GameObjectStatus::Free;
        m_objects_pool.free(object->id);
        object->Reset();
        return next;
    }

    void GameObjectManager::beforeUpdateCallback() {

        for (auto p = m_update_list.getFirst(); p != nullptr;p = p->update_list_next) {
            if (!isValid(p)) {
                continue;
            }
            if (  p->callbacks.beforeUpdate.valid()) {
                auto r = p->callbacks.beforeUpdate(p);
                if (!r.valid()) {
                    sol::error err = r;
                    EngineLogger::err("[lua] 调用回调函数 beforeUpdateCallback 时出错 : {}",err.what());
                }
            }

        }
    }
    void GameObjectManager::afterUpdateCallback() {

        for (auto p = m_update_list.getFirst(); p != nullptr;p = p->update_list_next) {
            if (!isValid(p)) {
                continue;
            }
            if (p->callbacks.afterUpdate.valid()) {
                auto r = p->callbacks.afterUpdate(p);
                if (!r.valid()) {
                    sol::error err = r;
                    EngineLogger::err("[lua] 调用回调函数 afterUpdateCallback 时出错 : {}",err.what());
                }
            }

        }
    }
    void GameObjectManager::updateCallback() {

        for (auto p = m_update_list.getFirst(); p != nullptr;p = p->update_list_next) {
            if (!isValid(p)) {
                continue;
            }
            if (p->callbacks.update.valid()) {
                auto r = p->callbacks.update(p);
                if (!r.valid()) {
                    sol::error err = r;
                    EngineLogger::err("[lua]  调用回调函数 updateCallback 时出错 : {}",err.what());
                }
            }
        }
    }

    void GameObjectManager::UpdateXY() noexcept {
        //每帧递减暂停计数，并据此冻结运动（暂停只限制运动，不限制逻辑回调）
        const auto pause = UpdateNextSpecialPause();
        for (auto p = m_update_list.getFirst(); p != nullptr;p = p->update_list_next) {
            if (!isValid(p) || (pause && !p->ignore_special_pause)) {
                continue;
            }
            p->updateXY();
        }
    }
    void GameObjectManager::UpdateLast() noexcept {
        //暂停期间不推进「上一帧坐标 / 计时器」，让运动恢复后 resolve_move 的位移仍正确
        const auto pause = GetSpecialPause();
        for (auto p = m_update_list.getFirst(); p != nullptr;) {
            switch (p->status) {
                case GameObjectStatus::Killed:
                    p = freeGameObjectWithCallback(p);
                    break;
                case GameObjectStatus::Destroyed:
                    p =freeGameObject(p);
                    break;
                case GameObjectStatus::Active:
                    if (!pause || p->ignore_special_pause) {
                        p->updateLast();
                    }
                    p = p->update_list_next;
                    break;
                case GameObjectStatus::Free:
                    EngineLogger::warn("[lua]  意外的情况:对标记为“Free”的GameObject执行UpdateLast操作");
                    p =p->update_list_next;
                    break;
            }
        }
    }

    void GameObjectManager::Render() {
        for (const auto& p : m_render_set) {
            if (!isValid(p) || p->hide ) {
                continue;
            }
            if (p->callbacks.render.valid()) {
                auto r = p->callbacks.render(p);
                if (!r.valid()) {
                    sol::error err = r;
                    EngineLogger::err("[lua] 调用回调函数Render时出错  : {}" ,err.what());
                }
            }
        }
    }

    void GameObjectManager::BoundCheck() {
        for (GameObject* p = m_update_list.getFirst(); p != nullptr;p = p->update_list_next) {
            if (!isValid(p) || !p->bound) {
                continue;
            }
            if (!isInBound(p)) {
                if (p->callbacks.kill.valid()) {
                    auto r = p->callbacks.kill(p);
                    if (!r.valid()) {
                        sol::error err = r;
                        EngineLogger::err("[lua]  调用回调函数Kill时出错 : {}",err.what());
                    }
                }
                p->status = GameObjectStatus::Destroyed;
            }
        }
    }

    void GameObjectManager::CollisionCheck(const Uint8 group1 ,const Uint8 group2) noexcept {
        m_cache_pairs.clear();
        if (group1 >= OBJECT_GROUP_SIZE || group2 >= OBJECT_GROUP_SIZE) {
            return;
        }
        //判定阶段只收集命中对，不执行回调：避免回调里增删对象破坏遍历
        const bool self = (group1 == group2);
        for (auto p1 = m_group_list[group1].getFirst(); p1 != nullptr; p1 = p1->group_list_next) {
            if (!isValid(p1) || !p1->colli) {
                continue;
            }
            //组内自撞时从 p1 的下一个开始，保证每一对只判定一次
            for (auto p2 = self ? p1->group_list_next : m_group_list[group2].getFirst();
                 p2 != nullptr; p2 = p2->group_list_next) {
                if (!isValid(p2) || !p2->colli) {
                    continue;
                }
                if (objectsOverlap(p1 ,p2)) {
                    m_cache_pairs.push_back(CollisionPairs{p1 ,p2});
                }
            }
        }
    }
    void GameObjectManager::CollisionCallback() {
        for (const auto& [object1 ,object2] : m_cache_pairs) {
            if (isValid(object1) && object1->callbacks.collision.valid()) {
                auto r = object1->callbacks.collision(object1 ,object2);
                if (!r.valid()) {
                    sol::error err = r;
                    EngineLogger::err("[lua] 调用回调函数Collision时出错 : {}",err.what());
                }
            }
            if (isValid(object2) && object2->callbacks.collision.valid()) {
                auto r = object2->callbacks.collision(object2 ,object1);
                if (!r.valid()) {
                    sol::error err = r;
                    EngineLogger::err("[lua] 调用回调函数Collision时出错 : {}",err.what());
                }
            }
        }
        m_cache_pairs.clear();
    }

    size_t GameObjectManager::Count() const noexcept {
        size_t n = 0;
        for (auto p = m_update_list.getFirst(); p != nullptr; p = p->update_list_next) {
            if (isValid(p)) {
                ++n;
            }
        }
        return n;
    }
    void GameObjectManager::Clear() {
        for (auto p = m_update_list.getFirst(); p != nullptr;) {
            p = freeGameObjectWithCallback(p);
        }
    }

    void GameObjectManager::setGroup(GameObject *object, Uint8 group) {
        if (!isValid(object) || group >= OBJECT_GROUP_SIZE||object->group == group) { return; }
        m_group_list[object->group].remove(object);
        object->group = group;
        m_group_list[group].add(object);
    }

    void GameObjectManager::setLayer(GameObject *object, double layer) {
        if (!isValid(object)|| object->layer == layer) { return; }
        m_render_set.erase(object);
        object->layer = layer;
        m_render_set.insert(object);
    }

    GameObject *GameObjectManager::New() {
        size_t id ;
        m_objects_pool.alloc(id);
        if (id == static_cast<size_t>(-1)) {
            EngineLogger::err("[lua]  对象池已满，创建失败");
            return nullptr;
        }
        GameObject* object = m_objects_pool.object(id);
        object->Reset();
        object->id = id;
        object->uid = ++m_uid;
        object->status = GameObjectStatus::Active;
        m_update_list.add(object);
        m_render_set.insert(object);
        m_group_list[0].add(object);

        return object;
    }
}
