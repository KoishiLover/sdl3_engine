//
// @author 帽子屋小姐
// @date 2026/10/9
//

#include "GameObject.hpp"
#include<cfloat>
namespace engine {
    void GameObject::Reset() {
        update_list_next = update_list_prev = nullptr;
        group_list_next = group_list_prev = nullptr;

        x = y = 0.0;
        vx = vy = 0.0;
        ax = ay = 0.0;
        rot = omega = 0.0;
        layer = 0.0;
        group = 0;
        last_x = last_y = 0.0;
        col_r = rect_a = rect_b = 0.0;

        navi = false ;
        colli = true;
        last_xy = false;
        rect = false;
        ignore_special_pause = false;
        resolve_move = false;
        hide = false;
        bound = false;

        img = "";

        ani_timer = 0;
        timer = 0;

        callbacks.reset();

        uid = 0;

        status = GameObjectStatus::Free;
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