# Hatta Engine 架构评审

> 评审对象：`D:\Program Files\CLion\Project\sdl3_engine`
> 规模：26 个源文件 / 约 3270 行 C++；Lua 侧 13 个脚本
> 技术栈：C++23 · SDL3 + SDL3_image / mixer / ttf · sol2 + LuaJIT · spdlog · vcpkg(`x64-windows-static`)
> 方法：逐文件通读 + 实机运行验证；所有结论标注 `文件:行号` 以便复核。
> 本文按 **架构 / 性能 / 资源管理 / API 设计 / 正确性 / 工程化** 六个维度列出问题，最后给出优先级路线图。

---

## 0. 总体评价

**先说做得好的地方（避免只看到问题）**

| 子系统 | 评价 |
|---|---|
| 游戏对象系统 | 对象池 + 侵入式双向链表 + `pmr` 有序渲染集，更新/渲染遍历**零堆分配**，增删 O(1)、顺序稳定。思路是对的 |
| 音频 `Audio` | 32 条 SE 轨道 + 空闲/占用队列 + 抢占最早播放者；`Init/Destroy` 幂等；音量做了 NaN 钳制。是全部模块里**工程质量最高的** |
| Lua `Class` / `Scene` / `Task` | 纯 Lua 搭了类系统、场景栈、协程任务表，分层清晰，脚本侧代码干净 |
| 文档 | `doc/*.lua` 用 LuaLS 注解写成，字段/参数/注意事项齐全。这个习惯反过来还帮你发现了 API 与实现不一致的问题 |
| 一处细节 | `enemy_bullet` 只实现 `init/render`、不实现 `update`，从而避开了每帧一次 Lua 调用 —— 这个性能直觉很准 |

**核心问题可以归纳成五句话**

1. **没有时间步长（delta time）** —— 逻辑按帧推进，帧率一变游戏速度就变。
2. **错误策略一刀切** —— `EngineLogger::err` 会「弹消息框 + 结束进程」，却被用在"图片名打错"这类可恢复场景上。
3. **有若干"假 API"** —— `SetVsync` / `SetWindowed` / `SetImageScale` 只写不读，调用后毫无效果，也不报错。
4. **根目录不在版本控制里，且全新 clone 无法构建**。
5. **运行时数据（脚本/资源/文档）没有部署规则**，全靠手工拷贝。
6. **`IsKeyDown` 的"本帧刚按下"边缘检测在对外层失效** —— 它被转发成了 `GetKeyState`（见 5.8）。

---

## 1. 架构

### 1.1 全局单例 + 宏，耦合度高

**证据**

- `Engine::GetInstance()` 是函数内静态单例，`#define IENGINE engine::Engine::GetInstance()`（`engine_frame.h:117`）
- 另一个独立单例 `GameObjectManager`，`#define IMGR GameObjectManager::getInstance()`（`GameObjectManager.hpp:314`）
- 所有 Lua 绑定直接写 `IENGINE.xxx()`（`lua_bind.cpp` 全篇）

**影响**：无法构造两个实例做单元测试；初始化顺序靠隐式约定；头文件互相包含，改一处重编半个工程。

**建议**：保留单例作为门面（Facade），但让绑定层通过参数注入 `Engine&`，而不是内部再 `GetInstance()`。
中期可考虑 ServiceLocator，但优先级低于 1.2 / 1.4。

### 1.2 窗口在 `GameInit()` 之后才显示 —— 时序反了

**证据**（`engine_frame.cpp:135-136`）

```cpp
GameInit();                                   // ← 此时窗口还是 HIDDEN
SDL_ShowWindow(m_window.GetWindow());
```

**影响**：脚本在 `GameInit` 里调 `SetResolution` / `SetLogicalPresentSize` 时，渲染器输出尺寸尚未确定。
之前"逻辑呈现没生效、画面缩在左上角"那类问题就源自这里。

**建议**：`SDL_ShowWindow` 提到 `GameInit()` 之前；或在 `SetResolution` 后补 `SDL_SyncWindow()` 再重设逻辑呈现。

### 1.3 事件系统几乎是空的

**证据**（`engine_frame.cpp:82-91`）

```cpp
while (SDL_PollEvent(&event)) {
    switch (event.type) {
        case SDL_EVENT_QUIT: m_status = EngineStatus::OnDestroy; break;
    }
}
```

- **不处理** `WINDOW_RESIZED` / `WINDOW_PIXEL_SIZE_CHANGED` / 焦点 / 鼠标 / 手柄 / 文本输入
- 为此预留的 `m_resized` **只写不读**（`engine_frame.h:35`，`engine_frame.cpp:294,300`）
- `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)`（`engine_frame.cpp:35`）**没有 `SDL_INIT_GAMEPAD`**

**建议**：至少处理 `WINDOW_RESIZED` → 校正逻辑呈现；把 `m_resized` 用起来或删掉。

### 1.4 没有时间步长，游戏速度与帧率绑定

**证据**

- `FrameController::GetDeltaTime()` **只有声明，没有定义**（`frame_controller.hpp:31`，`.cpp` 里搜不到）
  —— 现在不报错只因为没人调用它，**一旦有人调用就是链接错误**
- `m_dt` 算了但从不读（`frame_controller.cpp:85`）
- 对象位移是"每帧加一次速度"（`GameObject::updateXY()` 里 `x += vx`）

**影响**：**架构级缺陷**。任何"60fps 下调好的手感"在掉帧或换机时都会变；
子弹速度、动画、计时器全部与帧率耦合；`TaskManager:wait(n)` 也是"等 n 帧"，同样受影响。

**建议**

1. 补上 `GetDeltaTime()` 定义并把 `dt` 暴露给 Lua
2. 逻辑层乘 `dt`：`x += vx * dt`（速度单位改成"每秒"）
3. 或改**固定步长 + 累加器**（2D 游戏最稳的方案，也让碰撞表现可复现）：
   `accumulator += dt; while (accumulator >= STEP) { FixedUpdate(); accumulator -= STEP; }`
4. 协程 `wait(n)` 改成 `wait(seconds)`，或明确文档化为"等 n 帧"

### 1.5 对象系统（设计不错，但有隐患）

```
m_memory (pmr 池) ──┬── m_render_set  (有序集，按 layer→uid)
                    └── m_cache_pairs (碰撞命中对缓存)
m_group_list[16]    碰撞组链表
m_update_list       更新链表（遍历顺序 = 更新顺序）
m_objects_pool<GameObject, 32768>
```
（`GameObjectManager.hpp:184-197`）

| 隐患 | 证据 | 影响 |
|---|---|---|
| 池固定 32768，`GameObject` 约 300–450B | `GameObjectManager.hpp:16,130` | **常驻 10–14 MB**；首次使用时 32768 个对象全部构造 |
| 记账用两个 32768 元素数组 | `GameObjectManager.hpp:131-132` | `bool[32768]` + `size_t[32768]` ≈ 288 KB，可用位图 + 动态数组 |
| 分配顺序**倒序** | `free_ids[i] = Size-1-i`（`:159-167`） | 首个对象拿到 id=32767，与按 uid 排序的渲染集一起看，访存局部性差 |
| 碰撞 O(n²) | `CollisionCheck()` | 同组对象一多就雪崩 |
| 世界表固定 8 个 | `Renderer.hpp:58` | 不够用要改代码重编 |

**建议**：池大小可配置/按需增长；`free_ids` 用 `std::vector<size_t>`；碰撞加空间划分（网格/四叉树）；世界表用 `std::vector<World>`。

---

## 2. 性能

### 2.1 每帧最大开销是 **Lua 回调**，不是 C++

每个对象每帧要经历：C++ 遍历链表 → 压入 userdata → `lua_pcall` → Lua 里按 `__index` 链查
`self.x` → 返回。1000 个带 `update` 的对象 = 1000 次跨语言调用。

**证据**：`GameObjectManager::updateCallback()`（`GameObjectManager.cpp:121-135`）

**建议**

- 只有真正需要每帧逻辑的对象才绑定 `update`（`enemy_bullet` 已经这么做了 ✅）
- 大量同质对象（子弹）考虑**批量脚本**：一个 Lua 函数一次更新一批
- 真正热点（碰撞、位移）下沉到 C++，Lua 只做表现

### 2.2 `__index` 是三级查找链（取舍合理，但要知道代价）

每读一次 `self.x`：① C++ 字段表（长度分支 + 逐字符比较，`lua_object_bind.cpp:150-165`）
→ ② 实例表 `lua_gettable`（沿 metatable 到类表）→ ③ sol2 兜底。

换来"脚本字段与 C++ 字段共存"，**这个取舍是对的**；只要知道热循环里少碰对象字段即可。

### 2.3 渲染没有批处理，每精灵一次 draw call

**证据**（`Renderer.cpp:121-129`）

```cpp
SDL_RenderTextureRotated(m_renderer, _img.m_tex, &src, &dst, rot, &rotc, SDL_FLIP_NONE);
if (_img.m_blend_mode != SDL_BLENDMODE_NONE)
    SDL_SetTextureBlendMode(_img.m_tex, SDL_BLENDMODE_NONE);   // 画完又切回去
```

- 即使 `rot == 0` 也走 `Rotated` 版本（比 `SDL_RenderTexture` 慢）
- **每精灵切换两次混合模式**（设置 + 复位），破坏状态排序的可能
- 同一纹理被多个 Sprite 共享时，各 Sprite 各自记脏（`m_update_flag`，`Renderer.cpp:67/103-118`），
  同一纹理被反复重设 —— 100 个精灵/帧 ≈ 数百次多余 SDL 状态调用

**建议**：① `rot==0` 走 `SDL_RenderTexture`；② 混合同步改成"值变了才设"（缓存 last_applied）；
③ 去掉绘制后的强制复位；④ 中长期按纹理分组批处理。

### 2.4 `GetFPS()` 是**1 秒平均**，对实时掉帧不敏感

**证据**：`frame_controller.cpp:95-102`，每秒才刷新 `m_average_fps`。

**影响**：HUD 帧率滞后；写性能探针时要意识到这点（你那个"帧率波动探针"抓到的就是这种阶梯值）。

**建议**：另加"最近 N 帧滑动平均"或"上一帧耗时(ms)"，两个指标一起看。

### 2.5 零散小成本

| 项 | 证据 | 说明 |
|---|---|---|
| `Object.Count()` 是 O(n) | `GameObjectManager.cpp` `Count()` | 被放在**每帧** `SetTitle` 里（脚本侧），对象多时纯浪费 |
| 每帧改窗口标题 | `main.lua` `GameUpdate` | 触发系统调用 + 窗口重绘；调试可以，发布应去掉 |
| `m_frame_duration[0]` 从没更新 | `frame_controller.cpp:47-50,86` | 统计字段不一致 |
| `m_other_duration[1]` 先累加后又被整体覆盖 | `frame_controller.cpp:57,67,73,89` | 白算一遍 |
| `PlaySE` 每次全量轮询 `m_busy_queue` | `Audio.cpp:305-314` | 音效密集时每帧 32 次 `MIX_TrackPlaying`；建议改回调 |

### 2.6 可观的体积/时间成本

- exe **19.8 MB**（全静态 + 无 LTO，见 6.6）
- `vcpkg.json:8` 给 sdl3-image 开了 `avif` feature，但资源里**一张 avif 都没有**，
  为此装进了 aom/dav1d/libavif/libyuv 一大串（安装日志可见）
- `demo/packages/resources/Musics/图书室之梦.ogg`（8.1 MB）**没有任何脚本引用**（demo 里
  `LoadMusic/PlayMusic` 零调用）

---

## 3. 资源管理

### 3.1 【严重】重名加载会**覆盖**，导致悬垂指针

**证据**（`Renderer.cpp:33-42`）

```cpp
auto tex = IMG_LoadTexture(m_renderer, path.data());   // 先解码
if (m_textures.contains(name)) {
    log::warn("[lua] 纹理:{},已经存在,加载取消", name);  // ← 说"取消"
}
m_textures[std::string(name)] = TexturePtr(tex);         // ← 但实际覆盖了
```

**影响**：`Sprite` 里存的是**裸 `SDL_Texture*`**（`Sprite.hpp:28`，非拥有）。
重名 `LoadTexture` 时旧纹理被销毁，所有引用它的 `Sprite` 立刻**悬垂** → 下一帧 `DrawImage`
就是 use-after-free。而 `DrawImage` 的 `if (!m_tex)` 检查**挡不住已释放的非空指针**（`Renderer.cpp:76`）。

`LoadImage` 同样（`Renderer.cpp:51-54`，说"加载取消"实际 `insert_or_assign`）。

**建议**

```cpp
if (m_textures.contains(name)) { log::warn("纹理 [{}] 已存在，忽略", name); return; }
m_textures.emplace(std::string(name), TexturePtr(tex));
```

并把判断提到 `IMG_LoadTexture` **之前**（重复加载不必白读一次磁盘 —— `TextRender::LoadTTF`
就是先判重后开文件，可直接照抄 `TextRender.cpp:80-82`）；`Sprite` 的纹理改 `shared_ptr` 从根上消灭悬垂。

### 3.2 【严重】"资源找不到"直接**弹框退出进程**

**证据**：`Renderer.cpp` 全篇用 `log::err` 处理资源问题（`:48, :62, :73, :77`），而
`EngineLogger::err` 的语义是（`engine_log.cpp:40-46`）：

```cpp
spdlog::error(...);
SDL_ShowSimpleMessageBox(...);   // 阻塞弹框
GetFatalHandler()();             // → OnDestroy() → exit(EXIT_FAILURE)
```

**影响**：脚本里一个图片名拼错 → 弹框 + 整个游戏退出。而 `DrawImage` 在每帧路径上，
开发迭代摩擦极大。

**建议**：区分两类错误 ——

- **编程错误**（状态机非法、不可能分支）→ `err` 快速失败 ✅ 保留
- **资源缺失/脚本笔误**（可恢复）→ `warn` + 返回缺省值（画个洋红占位图）

### 3.3 【严重】`Engine::Init` 失败路径不清理已建资源

**证据**（`engine_frame.cpp:41-75`）：窗口失败、渲染器失败、文本失败、音频失败、Lua 失败、
Input 失败各自 `return false`，**全程不调用 `ReleaseResources()`**；
而 `Renderer` / `Audio` 的析构都是 `= default`（`Renderer.hpp:45`、`Audio.hpp:28`）——
**没有 RAII**，失败路径必然泄漏 SDL_Renderer、MIX_Mixer/轨道/已加载音频、纹理。

**建议**：给 `Renderer::~Renderer() { Destroy(); }`、`Audio::~Audio() { Destroy(); }`（配合幂等 Destroy），
或在 `Init` 里改成统一的 `goto fail` 收尾。

### 3.4 【严重】`LoadLuaScripts` 用 `err` 报告**成功**，且失败分支不可达

**证据**（`engine_frame.cpp:170-186`）

```cpp
if (std::filesystem::create_directories(src, ec)) {   // 返回 true = 创建成功
    log::info("...不存在,尝试创建...");
    if (ec) { log::err("...创建时失败..."); return false; }   // ← 永远进不来
    log::err("...已创建", src);                               // ← 成功却用 err！
}
```

两个 bug：

1. `create_directories` 返回 `true` 表示**创建成功**，失败时返回 `false` 并设置 `ec`。
   所以 `if (ec)` 嵌在 `if (created)` 里 → **创建失败被静默忽略**
2. 成功消息用了 `err` → **首次运行（目录不存在）会弹框退出**

**建议**

```cpp
std::error_code ec;
std::filesystem::create_directories(src, ec);
if (ec) { log::err("创建脚本目录失败: {}", ec.message()); return false; }
log::info("脚本目录就绪: {}", src);
```

### 3.5 把非 NUL 结尾的 `string_view` 交给 C API

**证据**：`Renderer.cpp:33` `IMG_LoadTexture(m_renderer, path.data())`、
`Audio.cpp:139` / `:267` `MIX_LoadAudio(m_mixer, path.data(), true)`。
这两个 SDL API 都要 `const char*`，而 `std::string_view::data()` **不保证** NUL 结尾。
当前调用链恰好来自 Lua 字符串（sol2 取 `string_view` 走 `lua_tolstring`，底层有 NUL），
所以**今天不出事**；一旦有人传子串就是越界读。

**建议**：接口收 `const std::string&`，或内部 `std::string file(path); ... file.c_str()`
（`TextRender.cpp:87-88` 已经这么写了 ✅）。

### 3.6 没有"卸载"接口 —— 资源只增不减

`Renderer` / `Audio` / `TextRender` 都只有 `Load*`，没有 `Unload*`，
也没有 `HasTexture(name)` 之类的查询。场景/关卡切换无法释放上一关的资源，内存单调增长。

**建议**：补 `UnloadTexture(name)` / `ClearUnused()`；更好的是给资源加引用计数或按场景分组释放。

### 3.7 `Renderer::Destroy` 不置空、不幂等

**证据**（`Renderer.cpp:154-162`）：`SDL_DestroyRenderer(m_renderer)` 后没有 `m_renderer = nullptr`，
也没有 `if (!m_renderer) return;` 守卫（对比 `Audio::Destroy` 在 `Audio.cpp:99` 做了置空）。
`Engine::Destroy()`（`engine_frame.cpp:92-94`）与 `OnExit/OnDestroy` 都可能走到 `ReleaseResources()`。

### 3.8 已暂停的 SE 轨道永不回收

**证据**（`Audio.cpp:309-312`）回收条件只有 `MIX_TrackPlaying(id)`。而
`MIX_TrackPlaying` 对**暂停中**的轨道返回真（作者自己在 `Audio.cpp:202` 就用了
`!playing && !paused` 才判"不在播放"）。若某条 SE 轨道进入 paused，它永远留在 `m_busy_queue`，
长跑后 32 条轨道可用性持续退化。

**建议**：回收条件加 `|| MIX_TrackPaused(id)`，或改用 `MIX_SetTrackStoppedCallback` 事件驱动。

### 3.9 `AcquireSETrack` 潜在空队列 UB

**证据**（`Audio.cpp:283`）：`m_free_queue` 空时直接 `m_busy_queue.front()`；
若 `m_busy_queue` 也为空即 UB。当前 `PlaySE` 的 `m_mixer` 守卫（`Audio.cpp:296`）挡住了
`Destroy()` 后的路径，**暂时不可达**，但这是"靠不变量侥幸正确"的典型，建议加兜底分支。

### 3.10 销毁路径本身是干净的（值得肯定）

`Engine::ReleaseResources()` 顺序正确（`engine_frame.cpp:107-113`）：

```
IMGR.reset() → m_text.Destroy() → m_audio.Destroy() → m_renderer.Destroy() → m_window.Destroy()
```

先释放持有 Lua 引用的对象，再依次拆"引用了 renderer 的"文本引擎、音频、渲染器、窗口。
`TextRender::Destroy` / `Audio::Destroy` 都做到了**幂等**，`TextRender` 还用 `m_ttf_owner`
正确区分了"TTF_Init 是谁调的"（`TextRender.cpp:30,58-61`）—— 这个模式值得推广到 `Audio`。

---

## 4. API 设计

### 4.1 【严重】三个"假 API"：调用成功，什么也没发生

| API | 证据 | 实际行为 |
|---|---|---|
| `SetVsync(bool)` | `engine_window.cpp:49-51` 写入 `m_vsync`，全文**再无读取** | 完全无效（真正的 vsync 在 `Renderer::Init` 硬编码 `SDL_SetRenderVSync(m_renderer, 0)`，`Renderer.cpp:25`）|
| `SetWindowed(bool)` | `engine_window.cpp:46-48` 写入 `m_windowed`，**再无读取** | 完全无效 |
| `SetImageScale(float)` | `Renderer.cpp:142-144` 写入 `m_scale`，**再无读取** | 完全无效（`DrawImage` 用的是每帧传入的形参 `scale`）；`doc/render.lua:55` 自己也标了"TODO(未实现)" |

**影响**：调用者以为生效了，实际没有，且不报错不提示 —— **静默失效是最难查的 bug**。
（你踩过一次同类问题：`self.layer = 100` 静默变成脚本字段，C++ 里 layer 还是 0。）

**建议**：要么实现，要么删除。保留占位就至少 `log::warn("尚未实现")`。

### 4.2 【严重】`LoadTexture` 的形参名三层互相矛盾（已误导过一次评审）

```cpp
Renderer.hpp:62     void LoadTexture(std::string_view path, std::string_view name);   // 声明
Renderer.cpp:31     void Renderer::LoadTexture(std::string_view name, std::string_view path) {  // 定义！名字反了
lua_bind.hpp:23     static void LW_LoadTexture(std::string_view path,std::string_view name);
engine_frame.h:85   void LoadTexture(std::string_view path,std::string_view name);
```

**当前能正常工作，纯属"双重否定"**：Lua 传的是 `(名字, 路径)`（`doc/render.lua:18` 与所有脚本都是），
`LW_` 的形参名叫 `(path, name)` 正好把值"翻译"反，`Engine` 原样透传，
到 `Renderer` 定义处形参叫 `(name, path)` 又反一次 → `IMG_LoadTexture(..., path.data())` 拿到真路径。

**风险**：任何人只要"顺手把名字改对"，整条纹理链立刻失效，而且是**运行期静默失败**
（表现为"纹理不存在"），不是编译错误。本次评审就有一位评审者据此误判为"纹理全部加载失败"。

**建议**：全链路统一 `(name, path)`，并让三处的形参名与头文件逐字一致。

### 4.3 同一含义的参数类型不统一

| 维度 | 不一致之处 |
|---|---|
| 颜色 | `SetImageState(..., float r,g,b,a)`（0–255 的 float，`Renderer.cpp:59`）vs `SetTextColor(Uint8 r,g,b,a)`；且前者**不钳制**（`SetImageState("img",1,300,0,0,255)` 会得到 `r≈1.18` 的超范围 `SDL_FColor`），而 `Audio::ClampVolume`（`Audio.cpp:16-21`）却做了 NaN/范围钳制 |
| 世界坐标 | `AddWorld / AddTextWorld(int,int,int,int)` 用 `int`，内部 `World` 是 `float`、对象坐标是 `double` |
| 尺寸 | `SetResolution(Uint64,Uint64)`（`engine_frame.h:79`）vs `LW_SetResolution(int,int)`（`lua_bind.hpp:19`）——窄化 |
| 按键 | `GetKeyState(int)` / `IsKeyDown(int)`（`engine_frame.h:107-108`），实际是 `SDL_Scancode`，无范围校验 |
| 帧率 | `SetFPS(Uint64)`；`m_target_delay = 1000000000 / m_target_fps`（`frame_controller.cpp:24`）用 `int` 字面量 |
| BGM 循环点 | `Uint32 start/end`（秒，`Audio.hpp:20-24`）→ 无法表达 0.5s；`start==0` 与 `end==0` 共用哨兵语义 |

**建议**：颜色统一 `Uint8`（并在入口钳制）；世界坐标统一 `double`；按键用 `SDL_Scancode`；
循环点改 `float` 或毫秒 `Sint64`，非法输入**拒绝**而不是悄悄改写用户意图
（现在 `end<=start` 会被静默改成 `end=0`，`Audio.cpp:134-138`）。

### 4.4 `LoadImage` 的参数名像目标位置，其实是源矩形

**证据**：`Renderer.hpp:63` `LoadImage(name, tex, float x, float y, float w, float h)`
—— 这里的 `x/y/w/h` 是**纹理内的源矩形**（`Sprite::m_src_x / m_w`），
名字却和 `DrawImage(img, x, y, ...)` 的目标坐标混淆。

**建议**：改名 `src_x, src_y, src_w, src_h`（`Sprite` 内部已经是 `m_src_x` 了，只是接口没跟上）。

### 4.5 `Destroy()` 的语义 = "异常退出进程"

**证据**：`engine_frame.cpp:92-94` `void Engine::Destroy() { OnDestroy(); }`，而 `OnDestroy` 会 `exit(EXIT_FAILURE)`。
名字像"析构/清理"，实际是带错误码终止进程。

**建议**：改名 `Abort()` / `FatalExit()`，或让它只清理、由调用方决定退出码。

### 4.6 脚本每帧回调出错只 `warn`，然后每帧继续

**证据**（`engine_frame.cpp:258-272`）：`GameUpdate` / `GameRender` 出错只 `log::warn`，不中断。
脚本一半初始化失败后游戏会以"半坏"状态继续跑，日志被同一个错误刷屏，真正原因被淹没。

**建议**：错误**计数 + 限流**（同一错误只记前 3 次），持续 N 帧后升级为致命错误或暂停到标题页。

### 4.7 命名与风格

- `FindAudio` 独有一个 `caller` 参数用于拼日志（`Audio.cpp:115-123`），其余接口把函数名硬编码 → 新增接口容易忘传
- 整个 `Audio` API **没有一个返回 `bool`**，而 `Audio::Init/CreateTracks` 又是 `bool` —— 内外不一致
- `Audio.hpp` 大面积用 `std::string_view`（`:57,66,69-75`）却只 include 了 `<string>`，
  靠 `engine_utils.hpp:10` 传递包含才编过 → 应显式 include

---

## 5. 正确性（本次开发中真实踩到的坑）

这一节最有价值 —— 都是"看起来能跑、特定条件下炸"的类型。

### 5.1 【已修】Lua 协程里访问对象 → 访问违例（0xC0000005）

**根因**：`lua_object_bind.cpp` 用 `go->lObject.push()` 压栈，它会压到 `lObject`
**自己记录的那个 `lua_State`**（主线程），而 `__index` / `__newindex` 收到的 `L` 是**协程栈**
→ 两个栈错位 → 随后的 `lua_gettable(L, -2)` 读到垃圾栈槽。

**修法**：改 `sol::stack::push(L, go->lObject)`（注册表引用是全线程共享的）。

**教训**：**跨 Lua/C++ 边界的代码必须用"当前传入的 `lua_State`"**，
不能依赖 sol2 对象内部记住的 state。协程是这条规则的高发区。

### 5.2 【已修】`sol::usertype<T*>` 的析构器拒绝工作

**现象**：GC 时报 `ERROR in finalizer: cannot call the destructor for ''`。

**根因**：sol2 的 `make_destructor<T>()` 对**裸指针类型**一律注册 `cannot_destroy`
（`sol/stack_core.hpp:1303-1324`：unique 类型 → `unique_destroy`；非指针 → 正常析构；**指针 → `cannot_destroy`**）。
`enrollments.destructor = false` **拦不住它** —— `usertype_storage` 注册 `__gc` 时根本不看这个标志。

**修法**：覆盖 metatable 的 `__gc` 为空实现（对象归对象池所有，Lua 不拥有）。

**教训**：绑 `T*` 给 sol2 时必须明确"谁拥有对象"；池化对象要自己接管 `__gc`。

### 5.3 【已修】`windows.h` 的宏破坏成员函数名

**现象**：引入 spdlog 后编译失败 `error C2039: "LoadImageA": 不是 "engine::Engine" 的成员`。

**根因**：`windows.h` 把 `LoadImage` / `DrawText` 定义成宏（→ `LoadImageA` / `DrawTextA`）。

**修法**：在引入 `windows.h` 的 TU 里 `#undef LoadImage` / `#undef DrawText`
（已加在 `engine_frame.cpp:15-20` 并写明原因）。

**教训**：成员名避开 Windows 的 `A/W` 宏（`LoadImage`、`DrawText`、`CreateWindow`、`GetObject`、
`SetWindowLong`…），或坚持 `WIN32_LEAN_AND_MEAN` + `#undef`。

### 5.4 【已修】自定义 `__index` 把 sol2 的 `__index` 当函数调用

**根因**：安装自定义 `__index` 时捕获了 sol2 原来的 `__index`，假设它是**函数**用 `lua_call`；
实际它是**一张方法表** → `attempt to call a table value`。
（最迷惑的是栈回溯指向 `__index` 自身而不是调用点。）

**修法**：先 `lua_isfunction` 判断，函数走 `lua_call`，表走 `lua_gettable`。

**教训**：劫持元方法时不要假设被劫持对象的类型，先探测。

### 5.5 【未修】`task.lua` 的错误路径依赖没打开的 `debug` 库

**证据**（`task.lua:48`）：`msg = msg .. debug.traceback(task)`。
而 `lua_bind.cpp:126-130` 只打开了 `base/package/coroutine/string/table/math/io/os/jit/ffi`，
**没有 `debug`**。任务一旦抛错，错误处理器自己会再抛
"attempt to index a nil value (global 'debug')"，把原始错误盖掉。

**建议**：`open_libraries` 加上 `sol::lib::debug`，或改用不依赖 debug 库的 traceback 写法。

### 5.6 【未修】未初始化的成员变量

| 成员 | 证据 | 说明 |
|---|---|---|
| `EngineWindow::m_vsync` / `m_windowed` | `engine_window.hpp:28-29`（无初值）| 读取即 UB（目前没人读，所以没暴露）|
| `FrameController::m_target_fps` / `m_start_time` / 各 `std::array` | `frame_controller.hpp:57-72`（多数无初值）| 靠"单例在 BSS 里被零初始化"侥幸正确；`Init()` 里 `if (!m_target_fps)` 正是在读不确定值 |
| `Sprite::m_tex` | `Sprite.hpp:28`（无 `= nullptr`）| 靠构造函数初始化列表兜住，建议补默认值 |

**建议**：全部加默认成员初始化器。这类问题平时不炸，一旦对象从静态存储改成堆/栈就立刻 UB。

### 5.7 【未修·严重】`Engine::IsKeyDown` 转发错函数 —— 边缘检测对外完全失效

**证据**（`engine_frame.cpp:388-393`）

```cpp
bool Engine::GetKeyState(const int key) const noexcept { return m_input.GetKeyState(key); }
bool Engine::IsKeyDown(const int key) const noexcept {
    return m_input.GetKeyState(key);        // ← 应该是 m_input.IsKeyDown(key)
}
```

而 `Input::IsKeyDown` 的实现**是对的**（`Input.cpp:29-31`：`m_keys[k] && !m_last_keys[k]`），
`UpdateLastInput()` 也在每帧被调用（`engine_frame.cpp:98`）—— 但**从来没人用它的结果**。

**影响**

- Lua 的 `IsKeyDown` 和 `GetKeyState` 变成**同一个函数**（按住期间一直为真）
- 菜单里按住方向键会瞬间滑到底；`IsKeyDown(KEY.Z)` 这类"按一次触发一次"的逻辑会**每帧触发**
  （这也解释了之前调试场景切换时，`GameScene` 的 tasks 里莫名多出一个任务：Z 键按住的那几帧
  每次 `Update` 都执行了一次 `changeScene`）
- `m_last_keys` / `UpdateLastInput()` 沦为死代码

**建议**：改一行 —— `return m_input.IsKeyDown(key);`

### 5.8 【未修】输入下标来自脚本且零校验 —— 脚本可触发越界读

**证据**（`Input.cpp:25-31`）

```cpp
bool Input::GetKeyState(int keycode) const noexcept { return m_keys[keycode]; }   // 无范围检查
```

- `m_keys` 指向 SDL 内部数组，长度由 `SDL_GetKeyboardState(&m_length)` 给出（当前 512）
- 而 `m_last_keys` 是**硬编码** `std::array<bool,512>`（`Input.hpp:19`），与真实长度可能不一致
- `m_length` 声明为 `int` 且**未初始化**（`Input.hpp:17`）；`Init()` 失败或未调用时，
  `UpdateLastInput()` 会以垃圾长度循环（`Input.cpp:34`）
- 语义是 **scancode**（`doc/key.lua:1`），但只要脚本写 `IsKeyDown(0x40000050)`（SDLK_LEFT 的键码）
  或随手一个 `999999`，就是越界读；未 `Init` 时 `m_keys` 为 `nullptr`，直接解引用

**建议**：入口加 `if (keycode < 0 || keycode >= m_length) return false;`；
`m_last_keys` 改成与 `m_length` 等长的 `std::vector<bool>`（或 `SDL_SCANCODE_COUNT`）；
`m_length` 给初值 0；`Init` 失败时保持 `m_keys = nullptr` 且所有读取先判空。

### 5.9 【未修】帧率节流尾部交给会**忙等**的 `SDL_DelayPrecise`

**证据**：`frame_controller.cpp:79-83`（`SDL_DelayPrecise(m_target_delay - elapsed)`）。
SDL 文档对 `SDL_DelayPrecise` 明说"必要时会忙等（busy waiting if necessary）"。
再叠加 `SetFPS` **只钳下界、不钳上界**（`:30-35`，且 Lua 可达 `lua_bind.cpp:10`）：

- 目标帧率低于显示器刷新率时，尾部自旋白白烧 CPU
- `SetFPS(1000000000)` → `m_target_delay = 0` → 完全不限帧
- 而 vsync 又被硬编码关闭（`Renderer.cpp:25`），没有"交给 vsync"这条退路

**建议**：`SetFPS` 加上限（如 1000）；节流优先用 vsync；
必须精确睡眠时用 `SDL_Delay` 粗睡 + 极短自旋（≤1ms），别把整段空闲都交给 `DelayPrecise`。

### 5.10 【未修】异常穿过 `noexcept` → `std::terminate`

**证据**

- `engine_frame.h:78-82` 一排 `noexcept`，其中 `SetTitle` → `engine_window.cpp:52-53`
  `m_title = title;`（`std::string` 赋值可抛 `bad_alloc`；传 `nullptr` 构造 `std::string` 是 UB）
- `engine_log.hpp:30-42` 的模板直接 `std::vformat`，**没有 try/catch** →
  格式串非法会抛 `std::format_error`，而多数调用点在 `noexcept` 函数里 → 直接 terminate

**建议**：真需要 `noexcept` 就在函数体内 try/catch 并降级；`SetTitle(nullptr)` 判空；
日志模板把 `vformat` 包起来，格式化失败就退化成原样输出。

### 5.11 【未修】`m_window_start_time` 没在 `Init` 里初始化 → 首秒 FPS 是假值

**证据**：`frame_controller.cpp:13-27` 设了 `m_start_time` 和各 duration，**唯独漏了
`m_window_start_time`**（`frame_controller.hpp:63` 初值 0）；随后 `:95-102` 用 `now - m_window_start_time`。
`SDL_GetTicksNS()` 是"自 SDL 初始化起"的纳秒数 → 若引擎初始化耗时超过 1 秒，
第一秒的 `GetFPS()` 会给出一个明显偏低的假值（脚本若把它当时间源就会出错）。

**建议**：`Init()` 里补 `m_window_start_time = SDL_GetTicksNS(); m_frame_count = 0;`

### 5.12 【未修】输入快照时机导致渲染阶段读到假值 + 亚帧按键丢失

**证据**：`UpdateLastInput()` 在 `OnUpdate()` **末尾**（`engine_frame.cpp:96-99`），
而主循环是 `Update → Render`（`:137-150`）→ 修好 5.7 之后，在 `GameRender` 里调 `IsKeyDown`
会恒为 `false`；同时事件循环只处理 `QUIT`（`:82-91`），**键盘事件被整个丢弃** →
一帧内"按下又松开"（亚帧点击）完全捕捉不到。

**建议**：`UpdateLastInput()` 挪到 `OnUpdate` 开头（先快照上一帧再处理输入），
或改成事件驱动（在 `SDL_PollEvent` 里累积 pressed/released 集合）。

### 5.13 【未修】关机路径的收尾问题

| 问题 | 证据 | 说明 |
|---|---|---|
| `main` 控制流含糊 | `main.cpp:4-12` | `Init()` 失败后调 `Destroy()`（其实是 `OnDestroy → exit(1)`），其后的 `Run()` 与 `return 0` 语义上都到不了，**失败原因被丢弃** |
| 从不调用 `SDL_Quit()` | `engine_frame.cpp` 全文搜不到 | SDL 子系统靠进程退出兜底 |
| `~Engine` 不释放资源 | `engine_frame.h:29`（`= default`） | 资源只能靠显式 `ReleaseResources()` |
| `EngineWindow::Destroy` 不幂等不置空 | `engine_window.cpp:63-66` | 而释放路径有三个入口（`OnExit` / `OnDestroy` / `main`），二次释放是 UB |
| `Input::Init` 失败次序 | `Input.cpp:14-20` | 失败早退时跳过 `m_last_keys` 清零（影响小，但契约不清晰） |

**建议**：`main` 改成 `if (!instance.Init()) return EXIT_FAILURE; ... return EXIT_SUCCESS;`；
显式 `SDL_Quit()`；`~Engine` 调用幂等的 `ReleaseResources()`；所有 `Destroy` 加幂等守卫 + 置空。

---

## 6. 工程化（这一节的问题最"要命"，因为它决定别人能不能跑起来）

### 6.1 【严重】根目录**不在版本控制里**

**证据**：根 `D:\...\sdl3_engine\.git` 不存在；只有 `src\.git`（remote = `github.com/KoishiLover/sdl3_engine.git`）
和 `external\luajit2\.git`；`.idea\vcs.xml:5` 只把 `src` 映射为 Git。

**影响**：根 `CMakeLists.txt` / `CMakePresets.json` / `vcpkg.json` / `doc/` / `demo/` / `userdata/`
**全部没有历史**，无法 review、无法回滚、无法 CI；而根 `.gitignore` 的规则对 `src` 仓库完全不生效
（`src` 内没有 `.gitignore`）。将来若在根 `git init`，两个嵌套 `.git` 会被当成 embedded repo。

**建议**：确定唯一仓库根（把根 CMake/presets/vcpkg 纳入 `src` 仓库并上移，或删掉 `src/.git` 后在根 init）；
把 `luajit2` 改成 `.gitmodules` 子模块，或在根仓库内 vendor。

### 6.2 【严重】全新 clone **无法构建**

**证据**：`external/CMakeLists.txt:3-7` 把 `LUAJIT_LIB` 硬编码成
`${CMAKE_CURRENT_SOURCE_DIR}/luajit2/src/lua51.lib`；而 `.gitignore:73` 忽略整个 `external/luajit2/`、
`:34` 忽略 `*.lib`。`external/luajit2/src/` 下是**手工产物**（lua51.lib 9.8MB、luajit.exe、pdb、exp），
没有任何构建规则生成它，也没有 commit hash（用的是滚动分支）。

**影响**：任何人 clone 后 configure 或 link 必然失败；LuaJIT 版本随本机目录滚动，**产物不可复现**；
Debug 也只能链 Release 版静态库（CRT 不匹配风险）。`external/CMakeLists.txt` 也没做
`IMPORTED_CONFIGURATIONS` / `EXISTS` 校验。

**建议**：用 `FetchContent` / `ExternalProject` 固定 LuaJIT commit 本地编译（或改用 vcpkg 的 luajit port）；
至少在 CMake 里对 `lua51.lib` 缺失做 `FATAL_ERROR` 并给出获取说明。

### 6.3 【严重】运行时数据没有部署规则

**证据**：`src/CMakeLists.txt:28-33` **只** POST_BUILD 拷贝 `doc/`；
而运行时需要 `packages/scripts/`、`packages/resources/`（`engine_frame.cpp:227,230`）
和 `userdata/log/`（`engine_log.cpp:22`）。`bin/Release` 下的 `packages/`、`userdata/`
是**手工拷进去的**，CMake 里没有任何规则。

**影响**：干净构建产出的 exe 因缺 `packages/` **直接初始化失败**；构建产物目录变成"半手工装配区"，
无法区分产物与数据，清理/分发不可靠。

**建议**：加自定义目标把 `demo/packages` 同步到 `$<TARGET_FILE_DIR>`（用文件级 `DEPENDS` 避免每次全量拷贝）；
`userdata` 由程序自行创建。

### 6.4 【严重】`src/` 无法作为独立工程配置

**证据**：`src/CMakeLists.txt:30` 用 `${CMAKE_SOURCE_DIR}/doc`，而 `doc/` 只存在于上层；
`src/CMakePresets.json`、`src/vcpkg.json` 的存在说明 `src/` 被设计成可独立配置的根
（`src` 仓库 `git ls-files` 共 29 个文件，无 `doc/` 条目）。

**影响**：单独 checkout 上游仓库后构建必然失败（POST_BUILD 命令失败 → 目标失败），上游不可能通过 CI。
（实测 `cmake -E copy_directory <不存在的源>` 返回 exit=1。）

**建议**：把 `doc/` 纳入源码仓库，或拷贝前 `if(EXISTS ...)`；并明确项目根唯一（删掉 `src` 下的重复 presets/vcpkg）。

### 6.5 【严重】资源路径大小写不一致 + 两份资源树内容漂移

**证据**：脚本写大写 `Images`（`MainScene.lua:1,4,7`、`GameScene.lua:1`、`player.lua:2`、`enemy_bullet.lua:2`），
实际部署目录是小写 `bin/Release/packages/resources/images`。
更麻烦的是两份资源**内容不同**：`demo/packages/resources/Images` 只有 5 个 png
（background/exit/player/start/uibg），部署目录有 7 个（多了 bullet/loli）。
即 **`enemy_bullet.lua` 要加载的 `bullet.png` 在 demo 里根本不存在**，demo 无法自洽运行。

**影响**：Windows 大小写不敏感掩盖了问题；迁移 Linux/macOS 时全部贴图加载失败 ——
而且因为 3.2 的错误策略，**会直接弹框退出**，排查成本极高。

**建议**：统一小写目录名并把脚本改成 `packages/resources/images/`；
让 `demo/packages` 成为唯一资源源、由 CMake 同步到输出目录，删掉手工维护的副本。

### 6.6 构建配置的其它问题

| 问题 | 证据 | 影响 |
|---|---|---|
| 无警告等级 | `CMakeLists.txt:7-9` 只有 `/utf-8`；全仓库无 `/W4`、`-Wall -Wextra` | 23 个文件在默认 `/W1` 下开发；**本次评审发现的"只写不读成员""隐式窄化"编译器本来就能报** |
| 未关编译器扩展 | `CMakeLists.txt:4` 设了 C++23 但无 `CMAKE_CXX_EXTENSIONS OFF` | GCC/Clang 下静默走 `gnu++23` |
| 无非 MSVC 分支 | `if(MSVC)` 无 else | 源码含大量中文注释/字面量，非 MSVC 工具链缺 `/utf-8` 对应项 |
| `/MT` 与 triplet 拆两处、无校验 | `CMakeLists.txt:3` vs `CMakePresets.json:17` | 绕过 preset 直接 `cmake -B build` → vcpkg 回落 `/MD` → LNK2038，报错晦涩 |
| 未声明 include 目录 | `src/CMakeLists.txt` 全文无 `target_include_directories`；靠 8 处 `../` 相对包含（`GameObjectManager.cpp:7`、`Audio.hpp:16`、`lua_object_bind.cpp:11-12`、`TextRender.hpp:22` …） | 任何目录重排即大面积断链；下游 target 无法复用头文件 |
| `CMAKE_EXPORT_COMPILE_COMMANDS=ON` 配 VS 生成器 | `CMakePresets.json:12,18` | VS 生成器不产出该文件 → clangd/clang-tidy 拿不到编译数据库 |
| 无 LTO | 全仓库无 `INTERPROCEDURAL_OPTIMIZATION` | Release 下 19.8MB 全静态 exe 无跨 TU 内联/去重 |
| CMake 版本下限钉到 patch | `CMakeLists.txt:1` `3.30.9` | 自带略低版本 CMake 的 IDE/发行版直接拒绝配置 |
| 三套构建树并存 | `cmake-build-release-visual-studio/`、`build/vs2022`（presets）、磁盘上还有第三套 | 磁盘浪费 + "哪套权威"不明 |
| 二进制资源无 LFS | `Musics/图书室之梦.ogg` 8.1MB、`jnbbh-Bold.ttf` 5.2MB、`background.png` 3.0MB…无 `.gitattributes` | 仓库体积不可控，二进制无法 diff；中文文件名跨平台有编码风险 |
| 无 install / CTest / CI / README / LICENSE | 全仓库 grep 无 `install(`、`enable_testing`；根目录无 `README.md`、`LICENSE`、`.clang-format` | 无打包规则、无回归门禁、无许可证（含第三方 LuaJIT/SDL3/fmt，法律风险）、新人无构建说明 |
| vcpkg 只是"半锁定" | `vcpkg.json:22` 有 `builtin-baseline` ✅ 做法正确；但 `CMakePresets.json:15` 用 `$env{VCPKG_ROOT}` **无存在性校验**，且 baseline 只锁 ports、不锁 vcpkg 工具自身版本，也没有 `vcpkg-configuration.json` | 同一台机器可复现，跨机器/跨 vcpkg 版本不可复现；未设 `VCPKG_ROOT` 时会解析成 `/scripts/buildsystems/vcpkg.cmake`，报错难懂 |
| 日志劫持了 spdlog 默认 logger | `engine_log.cpp:28` `spdlog::set_default_logger(logger)`；`engine_log.hpp:24-26` 只有 `info/warn/err`（**没有 fatal 级**） | 第三方库若也用 spdlog 会写进同一个 `engine.log`；缺少 fatal 级别正是 `err` 被滥用的成因；`m_initialized` 非原子（`engine_log.cpp:13,16,29`）；无 `flush_on`/`flush_every` 策略 |

### 6.7 文档部署的两个隐患

**证据**：`src/CMakeLists.txt:28-33` 把 `doc/*.lua` 拷到 `$<TARGET_FILE_DIR>/doc`；
而 `engine_frame.cpp:189-190` 又把 exe 目录塞进 `package.path`（`src + "?.lua;"`）。

1. **POST_BUILD 只在目标重链接时执行** → 只改 `doc/*.lua` 不会触发再拷贝 → 文档静默过期
2. 文档以 **`.lua`** 扩展名落在 exe 目录、又在 `?.lua` 搜索路径里 →
   `require("doc.audio")` 之类会**真的把文档当代码加载执行**
3. `copy_directory` 只覆盖不删除 → 改名/删除的旧文档会残留

**建议**：文档改用 `.md` 或独立 `docs/` 输出目录；用带 `DEPENDS ${DOC_FILES}` 的
`add_custom_command` + `add_custom_target` 管理。

### 6.8 其它工程细节

- **include guard 命名混乱**：前缀两套（`SDL3_ENGINE_*` vs `HATTA_ENGINE_*`，工程已改名 `hatta-engine` 但 guard 没跟上）；
  **3 处 guard 与文件名不符**：`engine_window.hpp:6` 用 `SDL3_ENGINE_ENGINE_RENDER_HPP`、
  `Sprite.hpp:5` 用 `SDL3_ENGINE_GRAPHIC_HPP`、`Renderer.hpp:5` 用 `SDL3_ENGINE_TEXTURE_HPP`；
  扩展名混用（`engine/` 下只有 `engine_frame.h` 是 `.h`）
- **`SetCurrentCWDPath()` 返回值被丢弃**（`engine_frame.cpp:25` 调用处不检查，定义在 `:233-246`）：
  一旦取不到 exe 目录，进程沿用调用者 CWD，`packages/`、`userdata/log` 全落到错误位置，
  且要到更晚的加载失败才暴露
- **`userdata/` 混进仓库**：根 `userdata/` 下有验证截图 2.5MB 和几个临时 Python 脚本
  （脚本里还写死了绝对路径 `D:\...\cmake-build-release-visual-studio\bin\Release`）。
  建议脚本迁到 `tools/`、路径改成参数/环境变量，截图加进 `.gitignore`
- **`std::format` 与 spdlog 自带 fmt 并存**（`engine_frame.cpp:11` + vcpkg 装了 fmt 12.2.0）：
- **PowerShell 脚本的编码陷阱**：`tools/*.ps1` 若存成**无 BOM 的 UTF-8**，Windows PowerShell 5.1
  会按系统 ANSI（GBK）解码 → 中文注释/字符串变乱码 → 解析器报「字符串缺少终止符」。
  修法：`.ps1` 一律存 **UTF-8 带 BOM**，或脚本正文只用 ASCII
  （`pwsh` 7 默认按 UTF-8 读取，所以用 7 做语法检查测不出这个问题）
  两套格式化实现，编译时间与体积双增

---

## 7. 优先级路线图

### P0 —— 会咬人的

| # | 事项 | 位置 |
|---|---|---|
| 1 | 根目录纳入版本控制（现在根 CMake/presets/vcpkg/doc/demo 全无历史） | 仓库根 |
| 2 | LuaJIT 依赖改成可复现构建（FetchContent + 固定 commit） | `external/CMakeLists.txt:3-7` |
| 3 | `LoadTexture`/`LoadImage` 重名覆盖导致悬垂 → 改成"已存在则忽略"，判重前置 | `Renderer.cpp:33-54` |
| 4 | 资源缺失从 `err`（弹框退出）降级为 `warn` + 占位图 | `Renderer.cpp:48,62,73,77` |
| 5 | 修 `LoadLuaScripts` 的 `err`/`ec` 逻辑（首次运行会弹框退出） | `engine_frame.cpp:170-186` |
| 6 | 给 `Renderer`/`Audio` 补 RAII 析构 + `Destroy` 幂等置空，`Engine::Init` 失败路径统一收尾 | `Renderer.cpp:154`、`Audio.cpp:72`、`engine_frame.cpp:41-75` |
| 7 | 实现或删除假 API（`SetVsync`/`SetWindowed`/`SetImageScale`） | `engine_window.cpp:46-51`、`Renderer.cpp:142` |
| 8 | 补 `debug` 库（或改 `task.lua` 的错误路径） | `task.lua:48`、`lua_bind.cpp:126` |
| 9 | 所有未初始化成员加默认值 | `engine_window.hpp:28-29`、`frame_controller.hpp:57-72` |
| 10 | 统一资源目录大小写 + 补齐 `demo` 缺失的 `bullet.png` | 脚本 + `demo/packages/resources` |
| 11 | **`Engine::IsKeyDown` 改回 `m_input.IsKeyDown`（一行修复，恢复边缘检测）** | `engine_frame.cpp:392` |
| 12 | 输入接口加范围校验 + `m_length` 初值 + `m_last_keys` 尺寸与真实长度对齐 | `Input.cpp:25-31`、`Input.hpp:17-19` |
| 13 | `SetFPS` 加上限；节流不要整段交给会忙等的 `SDL_DelayPrecise` | `frame_controller.cpp:30-35,79-83` |

### P1 —— 影响手感和可维护性

| # | 事项 |
|---|---|
| 11 | 引入 delta time（补 `GetDeltaTime()` 定义 + 逻辑乘 dt，或改固定步长），并给 dt 加上限 clamp（防长帧穿透） |
| 12 | `SDL_ShowWindow` 提到 `GameInit` 之前，修初始化时序 |
| 13 | 统一 `LoadTexture` 三层参数顺序为 `(name, path)`（现在靠"双重否定"工作） |
| 14 | 统一颜色/坐标/按键/循环点的参数类型 + 入口钳制 |
| 15 | 脚本每帧回调错误加限流 + 升级策略 |
| 16 | 运行时数据加部署规则（`demo/packages` → 输出目录） |
| 17 | 打开 `/W4` 或 `-Wall -Wextra`，并加 `target_include_directories` 去掉 `../` 相对包含 |
| 18 | 文档改 `.md` / 独立目录，用带 `DEPENDS` 的自定义目标部署 |
| 19 | `Renderer::Destroy`/`EngineWindow::Destroy` 幂等置空；`AcquireSETrack` 加空队列兜底；SE 回收加 `paused` 判断 |
| 20 | 检查 `SetCurrentCWDPath()` 返回值；`userdata/*.py` 迁到 `tools/` 并去掉硬编码路径 |
| 21 | `main` 控制流明确化 + 显式 `SDL_Quit()` + `~Engine` 调用幂等的 `ReleaseResources()` |
| 22 | 异常不要穿过 `noexcept`（`SetTitle` 判空、日志 `vformat` 包 try/catch） |
| 23 | `m_window_start_time` 初始化；`UpdateLastInput()` 挪到 `OnUpdate` 开头（或改事件驱动） |

### P2 —— 规模上来才痛

| # | 事项 |
|---|---|
| 24 | 渲染批处理 + 消除每精灵混合模式切换 + `rot==0` 快速路径 + 纹理级状态缓存 |
| 25 | 资源卸载接口 / 按场景分组释放 / `HasTexture` 查询 |
| 26 | 碰撞加空间划分 |
| 27 | 对象池大小可配置 + 记账数组瘦身 |
| 28 | `GetFPS()` 增加短窗口指标（上一帧耗时 ms） |
| 29 | 事件系统补窗口 resize / 手柄 / 文本输入 |
| 30 | 加 LTO、锁 vcpkg baseline、裁掉未用的 avif feature、删除无人引用的 8MB ogg |
| 31 | 补 `install()` / CTest 冒烟测试 / CI / README / LICENSE / `.gitattributes`(LFS) |
| 32 | 统一 include guard 命名与扩展名；`std::format` 与 spdlog fmt 二选一 |

---

## 8. 可以带走的经验（学习参考）

### 8.1 值得继续坚持的

1. **对象池 + 侵入式链表 + pmr** 是 2D 引擎里很对的组合：遍历零分配、增删 O(1)、顺序稳定。
2. **文档先行**：`doc/*.lua` 的注解质量高于大多数个人项目，而且反过来帮你发现了
   "`self.layer = 100` 不生效"这类 API 与实现不一致的问题。
3. **音频模块的"轨道池 + 抢占"** 思路清晰，是全部代码里最工程化的一段，可以作为其它模块的模板。
4. **把游戏逻辑放 Lua、性能敏感部分放 C++** 的分工是对的；
   `enemy_bullet` 不实现 `update` 从而避开每帧 Lua 调用，这个细节很漂亮。

### 8.2 五个反模式（记住它们）

1. **"错误策略一刀切"**：把"用户输入错误 / 资源缺失 / 编程错误"混成一个 `err` → 弹框退出。
   正确做法是**分级**：可恢复的记日志 + 降级，不可恢复的才快速失败。
2. **"假 API"**：接口存在、调用不报错、但什么也不做。这比崩溃更难查。
   规则：**每个 public 方法要么有实现，要么不写**（或显式标未实现）。
3. **"依赖隐式状态"**：`lua_State` 混用、未初始化成员、靠 BSS 零初始化侥幸正确、
   靠"参数名互相抵消"侥幸正确 —— 这类 bug 只在特定条件下出现，平时完全不露头。
   规则：**跨边界的东西一律显式传入，成员一律给初值，名字一律与语义一致**。
4. **"文档当代码"**：`doc/*.lua` 落在 `?.lua` 搜索路径里，一个 `require` 就会执行它。
   规则：不同用途的文件用不同扩展名/目录。
5. **"只写不读"**：`m_resized`、`m_vsync`、`m_windowed`、`m_scale`、`m_copy_map` 全是这类。
   规则：打开 `/W4`（MSVC 的 C4189/C4820 之类）或定期 grep 成员引用数，让编译器替你发现。

### 8.3 一条通用心法

> 这次修的所有 bug（协程崩溃、`__gc` 报错、`windows.h` 宏、`__index` 抽象泄漏、
> 参数名双重否定），本质都是**"我以为它会这样，但它其实是那样"**。
>
> 对抗它的办法不是"更小心"，而是**更早地把假设变成检查**：
> 编译器警告、`static_assert`、成员初始化器、断言、单元测试、CI，
> 以及 —— 像你做完这些改动后做的那样 —— **把关键状态写进日志/窗口标题，跑起来看**。
>
> 你这次用"截窗口 + 读标题栏"定位问题的做法非常有效，
> 建议把它固化成 `tools/` 下的小工具（现在的脚本写死了绝对路径，先解决这一点）。

---

## 附：本次评审已修复的问题（供对照）

| 问题 | 状态 |
|---|---|
| 协程里创建/访问对象崩溃（`lObject.push` 用错栈） | ✅ 已修 `lua_object_bind.cpp` |
| `usertype<T*>` 的 `__gc` 报 `cannot call the destructor` | ✅ 已修（空析构） |
| `windows.h` 宏破坏 `LoadImage`/`DrawText` | ✅ 已修 `engine_frame.cpp:15-20` |
| 自定义 `__index` 把 sol2 的 `__index` 当函数调用 | ✅ 已修（函数/表双分支） |
| 类缺 `__index` 导致 `attempt to call a table value` | ✅ 已加 `Class.New` 自动补 |
| 世界模式逻辑尺寸永远为 0（只在失败分支赋值） | ✅ 已修 `Renderer::SetLogicalPresentMode` |
| 脚本 `tasks` 从不被驱动（没有 `resume` 调用点） | ✅ 已修 `Scene:Update` |
| `enemy_bullet` 用了大写 `Init`/`Render` + 缺 `return` | ✅ 已修 |

*本文由代码通读 + 实机运行验证 + 三个子系统的独立评审交叉核对得出；
其中「`Renderer.cpp:31` 参数对调导致纹理全部加载失败」这一条经实机验证为**误判**（见 4.2），已更正。*
