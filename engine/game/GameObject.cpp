//
// @author 帽子屋小姐
// @date 2026/10/9
//

#include "GameObject.hpp"
#include<algorithm>
#include<cfloat>
namespace engine {

    void GameObject::Reset() {
        update_list_prev = nullptr;
        update_list_next = nullptr;
        group_list_next = nullptr;
        group_list_prev = nullptr;

        status = GameObjectStatus::Free;

        x = y = 0.0;
        last_x = last_y = 0.0;
        vx =  vy = 0.0;
        rot = 0.0;
        omega = 0.0;
        ax = ay = 0.0;
        maxv = DBL_MAX;
        max_vx = max_vy = DBL_MAX;

        rect = false;
        rect_a = rect_b = 0.0;
        col_r = 0.0;

        hscale = vscale = 1.0;

        layer = 0.0;
        group = 0;
        colli = true;
        boundcheck = true;
        hide = false;
        navi = false;

        ani_timer = 0;
        timer = 0;
        image  = nullptr;
    }

    void GameObject::UpdateMovement() noexcept {
        if (!ignore_pause && pause > 0 ) {
            --pause;
            return;
        }
        if (!resolve_move) {
            vx += ax;
            vy += ay;
            if (maxv <= DBL_MIN) {
                vx = vy  =0.0;
            }
            else if (const double cur_v = calculateSpeed(); cur_v > maxv ) {
                auto const cur_d = calculateSpeedDirection();
                vx = maxv * std::cos(cur_d);
                vy = maxv * std::sin(cur_d);
            }
            vx = std::clamp(vx , -max_vx ,max_vx);
            vy = std::clamp(vy ,-max_vy ,max_vy);

            x += vx;
            y += vy;
        }
        else {
            if (last_xy_touched) {
                vx = x - last_x ;
                vy = y - last_y ;
            }
            else {
                vx = 0.0;
                vy = 0.0;
            }
        }
        rot += omega;
        if (navi && last_xy_touched) {
            auto const ddx = x -last_x;
            auto const ddy = y -last_y;
            if (std::abs(ddx) > DBL_MIN || std::abs(ddy) > DBL_MIN) {
                rot = std::atan2 (ddy ,ddx);
            }
        }
    }
    void GameObject::UpdateLast() noexcept {
        if (!last_xy_touched) {
            dx = 0.0;
            dy = 0.0;
        } else {
            dx = x - last_x;
            dy = y - last_y;
        }
        last_xy_touched = true;
        last_x =  x;
        last_y = y;
        ++ timer;
        ++ ani_timer;
    }
} // engine