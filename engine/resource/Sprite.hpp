//
// @author 帽子屋小姐
// @date 2026/10/4
//
#ifndef SDL3_ENGINE_GRAPHIC_HPP
#define SDL3_ENGINE_GRAPHIC_HPP
#include<SDL3_image/SDL_image.h>

namespace engine {

    class Sprite {
    public:
        Sprite(float x,float y ,float w, float h,SDL_Texture* tex) noexcept:
            m_src_x(x),m_src_y(y),m_w(w),m_h(h),m_tex(tex) {
            m_color= {.r=1.0,.g =1.0,.b=1.0,.a=1.0};
            m_blend_mode = SDL_BLENDMODE_NONE;
            m_center_x= 0.0;
            m_center_y= 0.0;
        }
        ~Sprite() = default;
        float m_src_x;
        float m_src_y;
        float m_w;
        float m_h;
        float m_center_x;
        float m_center_y;
        bool m_update_flag =false;
        SDL_Texture* m_tex;
        SDL_FColor m_color;
        SDL_BlendMode m_blend_mode;
    };
}
#endif //SDL3_ENGINE_GRAPHIC_HPP
