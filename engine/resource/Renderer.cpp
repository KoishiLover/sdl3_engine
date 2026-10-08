//
// @author 帽子屋小姐
// @date 2026/10/5
//
#include "Renderer.hpp"

#include <filesystem>

#include "image.hpp"
#include "../engine_log.hpp"
#include<SDL3_image/SDL_image.h>

namespace engine {
    bool Renderer::Init(SDL_Window* window) {
        using log = EngineLogger;
        if (!window) {
            log::err("[engine] 渲染器初始化失败,窗口句柄为nullptr");
            return false;
        }
        m_renderer = SDL_CreateRenderer(window ,nullptr);
        if (!m_renderer) {
            log::err("[engine] 创建渲染器时出错:{}",SDL_GetError());
            return false;
        }
        return true;
    }
    SDL_Renderer* Renderer::GetRenderer()const noexcept {
        return m_renderer;
    }
    void Renderer::LoadTexture(std::string_view name, std::string_view path) {
        using log = EngineLogger;
        auto tex = IMG_LoadTexture(m_renderer , path.data());
        if (!tex) {
            log::err("[lua]调用函数LoadTexture()失败 , Error:{}",SDL_GetError());
            return;
        }
        if (m_textures.contains(name)) {
            log::warn("[lua] 纹理:{},已经存在,加载取消",name);
        }
        m_textures[std::string(name)] = TexturePtr(tex);
        log::info("[lua] 纹理资源 [{}] 创建成功",name);
    }
    void Renderer::LoadImage(std::string_view img, std::string_view tex,float x,float y ,float w,float h) {
        using log = EngineLogger;
        auto _tex = m_textures.find(tex);
        if (_tex == m_textures.end() ) {
            log::err("[lua] 调用函数 LoadImage 时出错 : 纹理[{}]不存在",tex);
            return;
        }
        if (m_images.contains(img)) {
            log::warn("[lua] 图片资源 [{}]已存在，加载取消",img);
        }
        m_images.insert_or_assign(std::string(img), Image(x,y,w,h,(_tex->second.get())));
        log::info("[lua] 图片资源 [{}] 创建成功",img);
    }
    void Renderer::SetImageState(std::string_view img ,BlendMode mode ,float r,float g,float b,float a) {
        using log = EngineLogger;
        SDL_FColor col= {.r = r /255.0f , .g = g / 255.0f , .b = b / 255.0f , .a = a/255.0f};
        auto it = m_images.find(img);
        if (it == m_images.end()) {
            log::err("[lua] 调用函数 SetImageState 时失败,图片 [{}]不存在",img);
            return;
        }
        (it->second).m_color = col;
        (it->second).m_blend_mode = static_cast<SDL_BlendMode>(mode);
        (it->second).m_update_flag = true;
    }
    void Renderer::DrawImage(std::string_view img ,float x,float y,float rot,float scale) {
        using log = EngineLogger;
        auto it = m_images.find(img);
        if (it == m_images.end()) {
            log::err("[lua] 调用函数 DrawImage 时出错,图像 [{}] 不存在",img);
            return;
        }
        if (!(it->second).m_tex) {
            log::err("[lua] 调用函数 DrawImage 时出错,图像[{}] 对应的纹理资源不存在",img);
            return;
        }
        auto& _img = it->second;
        //获取图像的中心坐标,相对于左上角
        float cx = _img.m_center_x *scale ,cy = _img.m_center_y *scale;
        //获取图像在纹理图像中的宽高
        float sw = _img.m_w , sh = _img.m_h;
        //获取图像在纹理中的坐标，相对于左上角
        float sx = _img.m_src_x ,sy = _img.m_src_y;
        SDL_FRect src,dst;
        if (m_world_id == - 1) {
            // 普通渲染模式 ，以窗口左上角为(0,0)向右向下为正方向
            src = {.x = sx ,.y = sy ,.w = sw, .h = sh};
            dst = {.x = x - cx ,.y = y -cy ,.w =sw*scale ,.h =sh *scale };

        } else {
            //世界渲染 ，渲染中心为世界的中心，向上向右为正方向
            //获取世界的左右上下边界
            auto [wl ,wr,wt,wb] = m_worlds[m_world_id];
            float wx  = (wl+wr)*0.5f,wy =(wt+wb)*0.5f;
            float lx = (x-wx)+m_logic_w*0.5f ,ly =m_logic_h*0.5f -(y-wy);
            src ={.x =sx ,.y =sy ,.w =sw ,.h =sh};
            dst = {.x=lx -cx ,.y =ly -cy ,.w =sw *scale ,.h =sh*scale};
        }
        auto [r,g,b,a] = _img.m_color;
        if (_img.m_update_flag) {
            if (!SDL_SetTextureColorModFloat(_img.m_tex,r,g,b)) {
                log::err("[lua] 调用函数 DrawImage 时出错:{}",SDL_GetError());
                return;
            }
            if (!SDL_SetTextureAlphaMod(_img.m_tex,a)) {
                log::err("[lua] 调用函数 DrawImage 时出错:{}",SDL_GetError());
                return;
            }
            if (_img.m_blend_mode!=SDL_BLENDMODE_NONE) {
                if (!SDL_SetTextureBlendMode(_img.m_tex , _img.m_blend_mode)) {
                    log::err("[lua] 调用函数 DrawImage 时出错:{}",SDL_GetError());
                    return;
                }
            }
            _img.m_update_flag =false;
        }
        SDL_FPoint rotc {.x =cx ,.y =cy};
        if (!SDL_RenderTextureRotated(m_renderer,_img.m_tex , &src,&dst,rot,&rotc,SDL_FLIP_NONE)) {
            log::err("[lua] 调用函数 DrawImage 时出错:{}",SDL_GetError());
            return;
        }
        if (_img.m_blend_mode!=SDL_BLENDMODE_NONE) {
            if (!SDL_SetTextureBlendMode(_img.m_tex , SDL_BLENDMODE_NONE)) {
                log::err("[lua] 调用函数 DrawImage 时出错:{}",SDL_GetError());
            }
        }
    }

    void Renderer::SetLogicalPresentMode(int w, int h) {
        using log = EngineLogger;
        if (!SDL_SetRenderLogicalPresentation(m_renderer , w, h,SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
            log::err("[engine] 调用函数 SetLogicalPresentMode 时出错:{}",SDL_GetError());
            m_logic_w = w;
            m_logic_h = h;
        }
    }

    void Renderer::SetImageScale(float scale) {
        m_scale = scale;
    }
    void Renderer::SetImageCenter(std::string_view img,float x,float y) {
        auto it = m_images.find(img);
        if (it == m_images.end()) {
            EngineLogger::err("[lua] 调用函数 SetImageCenter 时出错: 图片[{}] 不存在",img);
            return;
        }
        it->second.m_center_x = x;
        it->second.m_center_y = y;
    }
    void Renderer::Destroy() {
        for (auto& it :m_textures) {
            it.second.reset();
        }
        m_images.clear();
        m_copy_map.clear();
        SDL_DestroyRenderer(m_renderer);
        EngineLogger::info("[engine] SDL_Renderer 销毁");
    }

}
