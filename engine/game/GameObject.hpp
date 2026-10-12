//
// @author 帽子屋小姐
// @date 2026/10/9
//

#ifndef HATTA_ENGINE_GAMEOBJECT_HPP
#define HATTA_ENGINE_GAMEOBJECT_HPP

#include<SDL3/SDL_stdinc.h>
#include<sol/sol.hpp>
#include<cmath>
#include<string>

namespace engine {
    class Image;

    using Callback = sol::protected_function;

    struct GameObjectCallbacks {
        Callback init;
        Callback beforeUpdate;
        Callback update;
        Callback afterUpdate;
        Callback render;
        Callback collision;
        Callback kill;
        void reset() {
            init.reset();
            beforeUpdate.reset();
            update.reset();
            afterUpdate.reset();
            render.reset();
            collision.reset();
            kill.reset();
        }
    };
    enum class GameObjectStatus {
        Free ,
        Active,
        Killed,
        Destroyed
    };
    struct GameObject {
        //在对象池中的索引，全局对象唯一标识
        Uint64 id;
        Uint64 uid;

        GameObjectStatus status;

        //更新链表结点
        GameObject* update_list_next;
        GameObject* update_list_prev;
        //碰撞组链表结点
        GameObject* group_list_next;
        GameObject* group_list_prev;

        //位置坐标
        double x,y;
        //速度分量
        double vx , vy;
        // 加速度分类
        double ax , ay;
        //角度 ,角度增量
        double rot , omega;
        //判断圆的半径
        double col_r;
        //矩形碰撞判定
        double rect_a , rect_b;

        //图层
        double layer;
        //所属的碰撞组别
        Uint8 group;

        //图像资源
        std::string img;
        //计时器，动画计时器
        Uint64 timer;
        Uint64 ani_timer;

        //方向是否根据运动自动方向调整
        bool navi ;
        //上一帧是否正确获取坐标(即游戏对象是否为该帧生成)
        bool last_xy;
        //是否进行出界检测
        bool bound;
        // 是否启动碰撞检测
        bool colli;
        //是否进行渲染
        bool hide;
        //是否为矩形判定
        bool rect;
        //是否无视特殊暂停
        bool ignore_special_pause;
        //
        bool resolve_move;

        double last_x ,last_y; //[不可见]
        //回调函数集
        GameObjectCallbacks callbacks;

        //接口

        //设置位置
        inline void setPos(double _x,double _y) noexcept{ x = _x; y = _y;}
        //设置速度
        inline void setV (double v , double ang) noexcept {
            vx = v * std::cos(ang) ; vy = v * std::sin(ang);
        }
        //设置速度分量
        inline void setV2(double _vx,double _vy) noexcept{vx = _vx ; vy = _vy;}
        //设置加速度
        inline void setA(double _a , double ang) noexcept {
            ax = _a * std::cos(ang) ; ay = _a * std::sin(ang);
        }
        //设置加速度分量
        inline void setA2(double _ax ,double _ay) noexcept {ax = _ax ; ay = _ay;}
        //设置图层
        inline void setLayer(double _layer) noexcept { layer = _layer; }
        //设置碰撞组
        inline void setGroup(Uint8 _group) noexcept { group = _group; }
        //设置碰撞矩阵
        inline void setRect(double _a , double _b , bool _rect) noexcept {
            rect_a = _a ; rect_b = _b; rect = _rect;
            if (!rect) { col_r = std::min(_a , _b);}
        }
        //内部调用
        void Reset();
        [[nodiscard]]inline double calculateSpeed() const noexcept { return std::hypot(vx,vy); }
        [[nodiscard]]inline double calculateSpeedDirection() const noexcept {
            if (abs(vx) > std::numeric_limits<double>::min() && abs(vy) > std::numeric_limits<double>::min()) {
                return std::atan2(vy,vx);
            }
            return rot;
        }
        void updateXY() noexcept;
        void updateLast() noexcept;
        //void changeResource();
    };


} // engine

#endif //HATTA_ENGINE_GAMEOBJECT_HPP
