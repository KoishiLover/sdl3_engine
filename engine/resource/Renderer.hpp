//
// @author 帽子屋小姐
// @date 2026/10/4
// @desc 纹理资源管理类
#ifndef SDL3_ENGINE_TEXTURE_HPP
#define SDL3_ENGINE_TEXTURE_HPP
#include<unordered_map>
#include<string>
#include<string_view>
#include<memory>
#include<set>
#include<array>
#include <SDL3/SDL_render.h>
#include"Sprite.hpp"
#include"../engine_utils.hpp"
struct SDL_Texture;
struct SDL_Renderer;



namespace engine {
    class Sprite;
    struct TextureDeleter {
        void operator()(SDL_Texture* tex)const {
            if(tex != nullptr) {
                SDL_DestroyTexture(tex);
            }
        }
    };

    using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;


    enum class BlendMode :Uint32 {
        None = SDL_BLENDMODE_NONE,
        Blend = SDL_BLENDMODE_BLEND,
        Add = SDL_BLENDMODE_ADD,
        Mod = SDL_BLENDMODE_MOD,
        Mul = SDL_BLENDMODE_MUL,
    };

    class Renderer {
    public:
        Renderer() = default;
        ~Renderer() = default;
        bool Init(SDL_Window* window);
        [[nodiscard]]SDL_Renderer* GetRenderer()const noexcept;
        void Destroy();
    private:
        std::unordered_map<std::string , TexturePtr,StringHash ,StringEqual> m_textures;
        std::unordered_map<std::string , Sprite ,StringHash,StringEqual> m_images;
        std::unordered_map<std::string ,std::set<std::string>> m_copy_map;
        SDL_Renderer* m_renderer = nullptr;
        struct World {
            float left,right;
            float top,bottom;
        };
        std::array<World ,4> m_worlds;
        int m_world_id = -1;
        int m_logic_w;
        int m_logic_h;
        float m_scale = 1.0f;
    public:
        //接口
        void LoadTexture(std::string_view path,std::string_view name);
        void LoadImage(std::string_view name , std::string_view tex , float x,float y ,float w,float h);
        void DrawImage(std::string_view img , float x,float y,float rot ,float scale);
        void SetImageState(std::string_view img,BlendMode mode ,float r,float g,float b,float a);
        void SetImageScale(float scale);
        void SetImageCenter(std::string_view img ,float x,float y);
        void SetLogicalPresentMode(int w, int h);

    };
}
#endif //SDL3_ENGINE_TEXTURE_HPP
