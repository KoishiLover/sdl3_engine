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
        //World 定义在 engine_utils.hpp，和 TextRender 共用
        std::array<World ,8> m_worlds;
        int m_world_id = -1;
        int m_logic_w = 0;
        int m_logic_h = 0;
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

        ///@brief 切换当前渲染的世界：-1 表示普通渲染模式，其余取值需在 [0, m_worlds.size()) 内
        inline void SetWorldID(int id) noexcept {
            if (id < -1 || id >= static_cast<int>(m_worlds.size())) return;
            m_world_id = id;
        }
        ///@brief 注册/设置一个世界区域的边界：id 需在 [0, m_worlds.size()) 内
        inline void AddWorld(int id ,int left,int right,int top,int bottom) noexcept {
            if (id < 0 || id >= static_cast<int>(m_worlds.size())) return;
            m_worlds[id].left = left;
            m_worlds[id].right = right;
            m_worlds[id].top = top;
            m_worlds[id].bottom = bottom;
        }

    };
}
#endif //SDL3_ENGINE_TEXTURE_HPP
