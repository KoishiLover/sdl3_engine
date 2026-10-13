//
// @author 帽子屋小姐
// @date 2026/10/9
//

#include "GameObject.hpp"
#include<cfloat>
namespace engine {
    void GameObject::Reset() {
        //对象池索引 / 全局唯一标识 / 状态
        id = static_cast<Uint64>(-1);
        uid = 0;
        status = GameObjectStatus::Free;

        //更新链表结点
        update_list_next = nullptr;
        update_list_prev = nullptr;
        //碰撞组链表结点
        group_list_next = nullptr;
        group_list_prev = nullptr;

        //位置坐标
        x = y = 0.0;
        //速度分量
        vx = vy = 0.0;
        //加速度分量
        ax = ay = 0.0;
        //角度，角度增量
        rot = omega = 0.0;
        //判定圆半径
        col_r = 0.0;
        //矩形碰撞判定
        rect_a = rect_b = 0.0;

        //图层
        layer = 0.0;
        //所属碰撞组
        group = 0;

        //图像资源
        img = "";

        //计时器，动画计时器
        timer = 0;
        ani_timer = 0;

        //方向是否根据运动自动调整
        navi = false;
        //上一帧是否正确获取坐标
        last_xy = false;
        //是否进行出界检测
        bound = false;
        //是否启动碰撞检测
        colli = true;
        //是否进行渲染
        hide = false;
        //是否为矩形判定
        rect = false;
        //是否无视特殊暂停
        ignore_special_pause = false;
        //是否由脚本直接改坐标（resolve_move）
        resolve_move = false;

        //上一帧坐标
        last_x = last_y = 0.0;

        //回调函数集
        callbacks.reset();

        //Lua 脚本实例表
        lObject = sol::object(sol::nil);
    }

    void GameObject::updateXY()  noexcept {
        if (! resolve_move) {
            vx += ax ;
            vy += ay;

            x += vx;
            y += vy;
        } else {
            if (last_xy) {
                vx = x - last_x;
                vy = y - last_y;
            } else {
                vx = vy = 0.0;
            }
        }
        rot += omega;
        if (navi && last_xy) {
            auto const dx = x - last_x;
            auto const dy = y - last_y;
            if (std::abs(dx) > DBL_MIN && std::abs(dy) > DBL_MIN) {
                rot = std::atan2(dy,dx);
            }
        }
    }

    void GameObject::updateLast() noexcept {
        last_xy = true;
        last_x = x;
        last_y = y;
        ++timer;
        ++ani_timer;
    }

} // engine