//
// @author 帽子屋小姐
// @date 2026/10/9
//

#ifndef HATTA_ENGINE_GAMEOBJECT_HPP
#define HATTA_ENGINE_GAMEOBJECT_HPP

#include<SDL3/SDL_stdinc.h>
#include<sol/sol.hpp>
#include<limits>
#include<vector>

namespace engine {

    class Sprite;
    class GameObject;

    enum class GameObjectStatus {
        Free,
        Active,
        Destroyed,
        Killed
    };
    using callBack = sol::protected_function;
    struct GameObjectCallbacks {
        callBack onCreateCallback;
        std::vector<callBack>  beforeUpdateCallbacks;
        callBack onUpdateCallback;
        std::vector<callBack>  afterUpdateCallbacks;
        callBack onRenderCallback;
        callBack onColliCallbacks;
        callBack onDestroyCallbacks;
    };

    class GameObject {
    public:
        //链表部分
        GameObject* update_list_prev;
        GameObject* update_list_next;
        GameObject* group_list_prev;
        GameObject* group_list_next;

        GameObject() = default;
        ~GameObject() = default;
    //GameObject 属性
        double x ;
        double y ;
        double vx ;
        double vy ;
        double ax  ;
        double ay ;
        double rot;
        double omega;
        double rect_a ;
        double rect_b ;
        double col_r ;
        double hscale ;
        double vscale ;

        double maxv ;
        double max_vx,max_vy;

        uint8_t hide: 1;  //是否渲染
        uint8_t colli : 1  ;  // 是否启用碰撞检测
        uint8_t rect : 1 ;   //是否为矩形碰撞
        uint8_t navi : 1 ;   //渲染角度是否随着速度角度变化
        uint8_t boundcheck : 1;// 是否开启边界检测
        uint8_t last_xy_touched : 1;
        uint8_t resolve_move : 1;
        uint8_t ignore_pause : 1;

        Uint64 timer ;
        Uint64 ani_timer ;
        Uint64 pause;

        double layer ;   //图层
        Uint8 group ;  // 所属的碰撞组

        Sprite* image ;

        GameObjectStatus status ;


        Uint64 uid ;
    private:

        double last_x ;
        double last_y ;
        double dx ;
        double dy ;

    public:
        //回调函数
        GameObjectCallbacks callbacks ;
        void registerBeforeUpdateCallback(const callBack& call) {
            callbacks.beforeUpdateCallbacks.push_back(call);
        }
        void registerAfterUpdateCallback(const callBack& call) {
            callbacks.afterUpdateCallbacks.push_back(call);
        }

        //接口
        void UpdateMovement() noexcept ;
        void UpdateLast() noexcept;
        void setGroup(Uint8 group);
        void setLayer(double layer);

        //内部调用
        void Reset();
        [[nodiscard]] double calculateSpeed()const noexcept {
            return std::hypot(vx,vy);
        }
        [[nodiscard]] double calculateSpeedDirection()const noexcept {
            if (abs(vx) > std::numeric_limits<double>::min() && abs(vy) > std::numeric_limits<double>::min()) {
                return std::atan2(vy,vx);
            }
            return rot;
        }
        void setSpeed(double const speed)noexcept {
            if (auto const cur = calculateSpeed() ; cur > std::numeric_limits<double>::min()) {
                auto const a3 = speed / cur;
                vx *= a3;
                vy *= a3;
            }
            else {
                vx = std::cos(rot) * speed;
                vy = std::sin(rot) * speed;
            }
        }
        void setSpeedDirection(double const direction) noexcept {
            if (auto const speed = calculateSpeed() ; speed > std::numeric_limits<double>::min()) {
                vx = speed * std::cos(direction);
                vy = speed * std::sin(direction);
            } else {
                rot = direction;
            }
        }
    };
} // engine

#endif //HATTA_ENGINE_GAMEOBJECT_HPP
