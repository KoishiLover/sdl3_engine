//
// @author 帽子屋小姐
// @date 2026/10/13
// @desc 游戏对象系统的 Lua 绑定
//       GameObject usertype + 自定义 __index/__newindex
//       C++ 字段用编译期字段表 + switch 分发直接读写（绕开 sol2 对 usertype<T*> 的 property 绑定 bug），
//       脚本字段落实例表，方法走 sol2
//

#include "GameObjectManager.hpp"
#include "../engine_log.hpp"
#include "../lua_bind.hpp"

#include <sol/sol.hpp>
#include <vector>
#include <string_view>
#include <type_traits>

namespace engine {
namespace {
    ///@brief 从 userdata（栈下标 1）取出 GameObject*
    GameObject* GetRaw(lua_State* L) noexcept {
        return *static_cast<GameObject**>(lua_touserdata(L, 1));
    }

    // ==== C++ 字段访问：编译期编号 + 函数指针（零 std::function、零哈希）====
    ///@brief 读字段：把值 push 到 L 栈顶
    using FieldGetter = void (*)(GameObject&, lua_State*);
    ///@brief 写字段：从 L 的 value_idx 读值并写入
    using FieldSetter = void (*)(GameObject&, lua_State*, int);

    ///@brief 成员指针作非类型模板参数，生成无捕获 getter（bool 与数值分开处理）
    template<auto Member>
    void getField(GameObject& g, lua_State* L) {
        using T = std::remove_cvref_t<decltype(g.*Member)>;
        if constexpr (std::is_same_v<T, bool>) {
            lua_pushboolean(L, g.*Member);
        } else {
            lua_pushnumber(L, static_cast<lua_Number>(g.*Member));
        }
    }

    ///@brief 对应 setter
    template<auto Member>
    void setField(GameObject& g, lua_State* L, int value_idx) {
        using T = std::remove_cvref_t<decltype(g.*Member)>;
        if constexpr (std::is_same_v<T, bool>) {
            g.*Member = (lua_toboolean(L, value_idx) != 0);
        } else {
            g.*Member = static_cast<T>(lua_tonumber(L, value_idx));
        }
    }

    ///@brief 字段编号，顺序必须与 kFields 一致
    enum class FieldId : uint8_t {
        X, Y, VX, VY, AX, AY, ROT, OMEGA, COL_R, RECT_A, RECT_B,
        NAVI, BOUND, COLLI, HIDE, RECT, IGNORE_SPECIAL_PAUSE, RESOLVE_MOVE,
        NONE
    };

    struct FieldAccess {
        FieldGetter get;
        FieldSetter set;
    };

    ///@brief 编译期字段表，按下标 O(1) 取函数指针
    constexpr FieldAccess kFields[] = {
        { &getField<&GameObject::x>,      &setField<&GameObject::x> },
        { &getField<&GameObject::y>,      &setField<&GameObject::y> },
        { &getField<&GameObject::vx>,     &setField<&GameObject::vx> },
        { &getField<&GameObject::vy>,     &setField<&GameObject::vy> },
        { &getField<&GameObject::ax>,     &setField<&GameObject::ax> },
        { &getField<&GameObject::ay>,     &setField<&GameObject::ay> },
        { &getField<&GameObject::rot>,    &setField<&GameObject::rot> },
        { &getField<&GameObject::omega>,  &setField<&GameObject::omega> },
        { &getField<&GameObject::col_r>,  &setField<&GameObject::col_r> },
        { &getField<&GameObject::rect_a>, &setField<&GameObject::rect_a> },
        { &getField<&GameObject::rect_b>, &setField<&GameObject::rect_b> },
        { &getField<&GameObject::navi>,   &setField<&GameObject::navi> },
        { &getField<&GameObject::bound>,  &setField<&GameObject::bound> },
        { &getField<&GameObject::colli>,  &setField<&GameObject::colli> },
        { &getField<&GameObject::hide>,   &setField<&GameObject::hide> },
        { &getField<&GameObject::rect>,   &setField<&GameObject::rect> },
        { &getField<&GameObject::ignore_special_pause>, &setField<&GameObject::ignore_special_pause> },
        { &getField<&GameObject::resolve_move>,         &setField<&GameObject::resolve_move> },
    };

    ///@brief 字符串 → 字段编号（长度 + 字符分发，替代哈希）
    ///@note layer / group 不在此列：需同步管理器，用 SetLayer/SetGroup 方法
    FieldId resolveField(std::string_view key) noexcept {
        switch (key.size()) {
            case 1:
                if (key[0] == 'x') return FieldId::X;
                if (key[0] == 'y') return FieldId::Y;
                return FieldId::NONE;
            case 2:
                switch (key[0]) {
                    case 'v':
                        if (key[1] == 'x') return FieldId::VX;
                        if (key[1] == 'y') return FieldId::VY;
                        return FieldId::NONE;
                    case 'a':
                        if (key[1] == 'x') return FieldId::AX;
                        if (key[1] == 'y') return FieldId::AY;
                        return FieldId::NONE;
                    default:
                        return FieldId::NONE;
                }
            case 3:
                if (key == "rot") return FieldId::ROT;
                return FieldId::NONE;
            case 4:
                if (key == "navi") return FieldId::NAVI;
                if (key == "hide") return FieldId::HIDE;
                if (key == "rect") return FieldId::RECT;
                return FieldId::NONE;
            case 5:
                if (key == "omega") return FieldId::OMEGA;
                if (key == "col_r") return FieldId::COL_R;
                if (key == "bound") return FieldId::BOUND;
                if (key == "colli") return FieldId::COLLI;
                return FieldId::NONE;
            case 6:
                if (key == "rect_a") return FieldId::RECT_A;
                if (key == "rect_b") return FieldId::RECT_B;
                return FieldId::NONE;
            case 12:
                if (key == "resolve_move") return FieldId::RESOLVE_MOVE;
                return FieldId::NONE;
            case 20:
                if (key == "ignore_special_pause") return FieldId::IGNORE_SPECIAL_PAUSE;
                return FieldId::NONE;
            default:
                return FieldId::NONE;
        }
    }

    ///@brief __index：先查 C++ 字段，再查脚本实例表（沿 metatable 链），最后回落 sol2 方法
    ///@note upvalue1 = sol2 原始 __index
    int ObjectIndex(lua_State* L) {
        auto* go = GetRaw(L);

        // 1. C++ 字段
        size_t key_len = 0;
        const char* key = lua_tolstring(L, 2, &key_len);
        if (key != nullptr) {
            FieldId id = resolveField(std::string_view(key, key_len));
            if (id != FieldId::NONE) {
                kFields[static_cast<size_t>(id)].get(*go, L);
                return 1;
            }
        }

        // 2. 脚本实例表（普通索引，沿 instance → class，能查到类方法）
        if (go->lObject.valid() && go->lObject.is<sol::table>()) {
            go->lObject.push();
            lua_pushvalue(L, 2);
            lua_gettable(L, -2);
            if (!lua_isnil(L, -1)) {
                return 1;
            }
            lua_pop(L, 2);
        }

        // 3. sol2 方法（SetPos / GetSpeed / SetGroup ...）
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushvalue(L, 1);
        lua_pushvalue(L, 2);
        lua_call(L, 2, 1);
        return 1;
    }

    ///@brief __newindex：先写 C++ 字段，否则落脚本实例表
    int ObjectNewIndex(lua_State* L) {
        auto* go = GetRaw(L);

        // 1. C++ 字段
        size_t key_len = 0;
        const char* key = lua_tolstring(L, 2, &key_len);
        if (key != nullptr) {
            FieldId id = resolveField(std::string_view(key, key_len));
            if (id != FieldId::NONE) {
                kFields[static_cast<size_t>(id)].set(*go, L, 3);
                return 0;
            }
        }

        // 2. 脚本实例表（self.speed 这类）
        if (go->lObject.valid() && go->lObject.is<sol::table>()) {
            go->lObject.push();
            lua_pushvalue(L, 2);
            lua_pushvalue(L, 3);
            lua_rawset(L, -3);
            lua_pop(L, 1);
        }
        return 0;
    }
}

void LuaWrapper::GameObjectBind(sol::state& lua) {
    auto& manager = GameObjectManager::getInstance();

    // 挂载到 hatta 命名空间（hatta.Object / hatta.GameObject）
    sol::table hatta = lua["hatta"];

    // 1. 注册 usertype（只绑方法；字段由 __index/__newindex 处理）
    //    对象由对象池管理，禁用默认构造/析构的 automagic，避免 GC 时销毁池对象
    sol::automagic_enrollments enrollments;
    enrollments.destructor = false;
    enrollments.default_constructor = false;
    auto go = hatta.new_usertype<GameObject*>("GameObject", enrollments);

    // —— 方法（指针 usertype 上不能用成员函数指针，统一用 lambda 接收 GameObject&）——
    go.set("SetPos", [](GameObject& g, double x, double y) { g.setPos(x, y); });
    go.set("SetV",   [](GameObject& g, double v, double ang) { g.setV(v, ang); });
    go.set("SetV2",  [](GameObject& g, double vx, double vy) { g.setV2(vx, vy); });
    go.set("SetA",   [](GameObject& g, double a, double ang) { g.setA(a, ang); });
    go.set("SetA2",  [](GameObject& g, double ax, double ay) { g.setA2(ax, ay); });
    go.set("SetRect",[](GameObject& g, double a, double b, bool r) { g.setRect(a, b, r); });
    go.set("GetSpeed",     [](GameObject& g) { return g.calculateSpeed(); });
    go.set("GetDirection", [](GameObject& g) { return g.calculateSpeedDirection(); });
    go.set("GetId",   [](GameObject& g) { return g.id; });
    go.set("GetUid",  [](GameObject& g) { return g.uid; });
    go.set("GetPos",  [](GameObject& g) { return std::vector<double>{g.x, g.y}; });
    go.set("GetTimer",    [](GameObject& g) { return g.timer; });
    go.set("GetAniTimer", [](GameObject& g) { return g.ani_timer; });

    // —— 组/图层必须走管理器（同步链表/渲染集）——
    go.set("SetGroup", [&manager](GameObject& g, Uint8 grp) { manager.setGroup(&g, grp); });
    go.set("GetGroup", [](GameObject& g) { return g.group; });
    go.set("SetLayer", [&manager](GameObject& g, double l)  { manager.setLayer(&g, l); });
    go.set("GetLayer", [](GameObject& g) { return g.layer; });

    // —— 销毁：打标记，由 UpdateLast 统一回收 ——
    go.set("Destroy", [](GameObject& g) { g.status = GameObjectStatus::Destroyed; });
    go.set("Kill",    [](GameObject& g) { g.status = GameObjectStatus::Killed; });
    go.set("IsAlive", [](GameObject& g) { return g.status == GameObjectStatus::Active; });

    // 2. 安装自定义 __index / __newindex
    lua_State* L = lua.lua_state();
    const std::string meta = sol::usertype_traits<GameObject*>::metatable();
    luaL_getmetatable(L, meta.c_str());
    const int mt = lua_gettop(L);

    // __index 需要 sol2 原始 __index 作为回落（查方法）
    lua_getfield(L, mt, "__index");
    lua_pushcclosure(L, &ObjectIndex, 1);
    lua_setfield(L, mt, "__index");

    // __newindex 不需要 sol2 原始（字段自己处理，脚本字段落实例表）
    lua_pushcclosure(L, &ObjectNewIndex, 0);
    lua_setfield(L, mt, "__newindex");

    lua_pop(L, 1);

    // 3. hatta.Object.New(class, args...)
    sol::table object_api = lua.create_table();
    hatta.set("Object", object_api);
    object_api.set_function("New",
        [&lua, &manager](sol::table klass, sol::variadic_args args) -> GameObject* {
            sol::object init_obj = klass["init"];
            if (!init_obj.is<sol::function>()) {
                EngineLogger::err("[lua] Object.New 失败：类必须实现 init");
                return nullptr;
            }

            GameObject* obj = manager.New();
            if (!obj) {
                return nullptr;
            }

            // 实例表：metatable = 脚本类（脚本字段与方法写/查这里）
            sol::table instance = lua.create_table();
            instance[sol::metatable_key] = klass;
            obj->lObject = instance;

            // 收集类方法
            obj->callbacks.init         = klass["init"];
            obj->callbacks.update       = klass["update"];
            obj->callbacks.render       = klass["render"];
            obj->callbacks.beforeUpdate = klass["beforeUpdate"];
            obj->callbacks.afterUpdate  = klass["afterUpdate"];
            obj->callbacks.collision    = klass["collision"];
            obj->callbacks.kill         = klass["kill"];

            // 直接调用 init(self, args...)
            sol::protected_function init = init_obj;
            auto result = init(obj, sol::as_args(args));
            if (!result.valid()) {
                sol::error e = result;
                EngineLogger::err("[lua] init 执行失败: {}", e.what());
                obj->status = GameObjectStatus::Destroyed;
                return nullptr;
            }
            return obj;
        });
}

} // namespace engine
