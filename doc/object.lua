---@meta
---@author 帽子屋小姐
---游戏对象系统
---@see hatta.Object
---@see hatta.ObjectClass
---@see hatta.GameObject

--[[
    用法示例：

        local Bullet = {}                     -- 类表（__index 交给 New 自动补）

        function Bullet:init(x, y)            -- 必须实现；创建时调用一次
            self.x = x                        -- self.x 直接读写 C++ 的 x
            self.y = y
            self.speed = 3                    -- 自定义字段自动落到脚本实例表
        end

        function Bullet:update()              -- 每帧逻辑
            self.x = self.x + self.speed
        end

        function Bullet:render()              -- 每帧渲染
            DrawImage("img.bullet", self.x, self.y, self.rot, 1)
        end

        function Bullet:kill()                -- 被回收时
            PlaySE("se.hit")
        end

        -- 创建对象：New 会先补上 Bullet.__index = Bullet，再交给 Object.New
        -- 若直接用 Object.New，类表必须自己写 Bullet.__index = Bullet
        local bullet = New(Bullet, 100, 200)
]]


---@class hatta.Object 游戏对象管理器
hatta.Object = {}

---创建一个游戏对象
---@param class hatta.ObjectClass 脚本类，必须实现 init
---@param ... any 传给 init 的额外实参
---@return hatta.GameObject|nil 新对象；对象池（容量 16384）已满时返回 nil
---@note 类上实现了哪些回调就收集哪些，没实现的不调用
---@note 类表必须自己写好 class.__index = class，否则实例查不到方法；
---      想省掉这一步就用全局的 New(class, ...)（等价 Object.New，但会自动补 __index）
function hatta.Object.New(class, ...) end

---当前存活对象数量
---@return number
function hatta.Object.Count() end

---对象池容量（OBJECT_POOL_SIZE）
---@return number
function hatta.Object.Capacity() end

---立即回收所有对象；每个对象都会触发一次 kill 回调
function hatta.Object.Clear() end

---设置舞台边界（世界坐标 y 轴向上，需 left<=right 且 bottom<=top；顺序不对会被忽略）
---@param left number 左边界
---@param right number 右边界
---@param top number 上边界
---@param bottom number 下边界
function hatta.Object.SetBound(left, right, top, bottom) end

---查询坐标点是否在舞台边界内
---@param x number
---@param y number
---@return boolean
function hatta.Object.IsInBound(x, y) end

---请求全局暂停：接下来 frames 帧内对象不运动、计时器也不推进（回调照常执行）
---@param frames number 帧数；<=0 表示取消暂停
function hatta.Object.Pause(frames) end

---当前是否处于全局暂停中
---@return boolean
function hatta.Object.IsPaused() end

---对两个碰撞组做一次碰撞检测，并立即执行命中双方的 collision 回调
---@param group_a number 第一组组号，取值 [0,16)
---@param group_b number 第二组组号；与 group_a 相同表示组内自撞（每一对只判定一次）
---@note 调用时机由脚本决定（通常在 update 里），引擎不会自动执行。
--- 参与条件：存活且 colli=true 的对象。
--- 判定规则：双方都开启矩形判定（rect=true）时用 AABB；否则一律按圆判定，半径取 col_r。
function hatta.Object.Collision(group_a, group_b) end


--[[
    === 帧驱动接口 ===
    引擎不会自动调用这些函数：什么时候更新、按什么顺序更新，全部由开发者自行决定。
    一个典型的每帧流程：

        -- GameUpdate 里
        Object.BeforeUpdate()   -- 执行所有 beforeUpdate 回调
        Object.Update()         -- 执行所有 update 回调
        Object.AfterUpdate()    -- 执行所有 afterUpdate 回调
        Object.UpdateXY()       -- 按速度/加速度推进坐标（内部会递减全局暂停计数）
        Object.UpdateLast()     -- 推进计时器与上一帧坐标，并回收被标记销毁的对象
        Object.BoundCheck()     -- 出界检测

        -- GameRender 里
        Object.Render()         -- 按 layer 顺序执行所有 render 回调

    注意事项：
      · Object.UpdateXY() 每帧只能调一次 —— 它会递减全局暂停计数
      · Object.Destroy() 只是打标记，必须等 Object.UpdateLast() 执行后才真正回收
      · Object.Pause() 只冻结运动与计时器，逻辑回调照常执行
      · 只调 Update() 而不调 UpdateXY()，对象不会移动
]]

---执行所有存活对象的 beforeUpdate 回调
---@note 帧驱动接口，由开发者自行调用；引擎不会自动调用
function hatta.Object.BeforeUpdate() end

---执行所有存活对象的 update 回调
---@note 帧驱动接口，由开发者自行调用；引擎不会自动调用
function hatta.Object.Update() end

---执行所有存活对象的 afterUpdate 回调
---@note 帧驱动接口，由开发者自行调用；引擎不会自动调用
function hatta.Object.AfterUpdate() end

---按速度与加速度推进所有对象的坐标（并递减全局暂停计数）
---@note 帧驱动接口，由开发者自行调用；每帧只能调用一次
function hatta.Object.UpdateXY() end

---推进计时器与上一帧坐标，并回收所有被标记 Kill/Destroy 的对象
---@note 帧驱动接口，由开发者自行调用；Destroy() 的标记要等这里执行后才真正回收
function hatta.Object.UpdateLast() end

---对所有 bound=true 的对象做出界检测，越界者触发 kill 回调后消亡
---@note 帧驱动接口，由开发者自行调用；引擎不会自动调用
function hatta.Object.BoundCheck() end

---按图层顺序执行所有对象的 render 回调
---@note 帧驱动接口，由开发者自行调用；通常放在 GameRender 里
function hatta.Object.Render() end


---@class hatta.ObjectClass 脚本对象类
---Object.New 会从类上自动收集下列方法，调用时均把对象自身作为第一个实参。
---除 init 外全部可选；未实现则不会被调用。
---@field init fun(self:hatta.GameObject, ...) 创建时调用一次（必须实现）
---@field beforeUpdate fun(self:hatta.GameObject) 每帧逻辑更新之前
---@field update fun(self:hatta.GameObject) 每帧逻辑更新
---@field afterUpdate fun(self:hatta.GameObject) 每帧逻辑更新之后
---@field render fun(self:hatta.GameObject) 每帧渲染
---@field collision fun(self:hatta.GameObject, other:hatta.GameObject) 碰撞命中时；other 为对方对象
---@field kill fun(self:hatta.GameObject) 对象被回收时


---@class hatta.GameObject 游戏对象句柄
---
---【字段】直接映射到 C++ 数据：self.x = 10 与 C++ 的 obj->x = 10 完全等价。
---字段名是保留名，写它们一定落到 C++ 字段；其它名字（如 self.speed）落到脚本实例表。
---layer / group 需要同步管理器内部的链表与渲染集，故不开放为字段，请用 SetLayer / SetGroup。
---@field x number 世界坐标 x
---@field y number 世界坐标 y
---@field vx number 每帧 x 轴位移量
---@field vy number 每帧 y 轴位移量
---@field ax number 每帧 x 轴加速度（每帧叠加到 vx）
---@field ay number 每帧 y 轴加速度（每帧叠加到 vy）
---@field rot number 朝向角度（弧度）
---@field omega number 每帧角度增量
---@field col_r number 圆形判定半径
---@field rect_a number 矩形判定半轴 a
---@field rect_b number 矩形判定半轴 b
---@field timer number 存活帧数（由 UpdateLast 每帧自增，可写以重置）
---@field ani_timer number 动画计时器（与 timer 同步自增，供脚本做动画）
---@field navi boolean 是否让朝向自动跟随速度方向
---@field bound boolean 是否开启出界检测（出界即消亡）
---@field colli boolean 是否参与碰撞检测（默认 true）
---@field hide boolean 是否不渲染
---@field rect boolean 是否使用矩形判定
---@field ignore_special_pause boolean 是否无视全局暂停
---@field resolve_move boolean 位移是否由脚本直接改写坐标产生（开启后速度由坐标差反推）
hatta.GameObject = {}

---直接设置坐标（世界坐标，y 轴向上）
---@param x number
---@param y number
function hatta.GameObject:SetPos(x, y) end

---按「速率 + 方向角」设置速度
---@param v number 速率
---@param ang number 方向角（弧度）
function hatta.GameObject:SetV(v, ang) end

---按分量设置速度
---@param vx number
---@param vy number
function hatta.GameObject:SetV2(vx, vy) end

---按「大小 + 方向角」设置加速度
---@param a number 加速度大小
---@param ang number 方向角（弧度）
function hatta.GameObject:SetA(a, ang) end

---按分量设置加速度
---@param ax number
---@param ay number
function hatta.GameObject:SetA2(ax, ay) end

---设置碰撞判定形状
---@param a number 半轴 a
---@param b number 半轴 b
---@param rect boolean 是否使用矩形判定；为 false 时退化为圆，半径取 min(a, b)
function hatta.GameObject:SetRect(a, b, rect) end

---当前速率
---@return number
function hatta.GameObject:GetSpeed() end

---当前速度方向（弧度；速度为 0 时返回 rot）
---@return number
function hatta.GameObject:GetDirection() end

---对象池索引（对象存活期间固定不变）
---@return number
function hatta.GameObject:GetId() end

---对象唯一标识（每次创建自增）
---@return number
function hatta.GameObject:GetUid() end

---当前位置
---@return number[] {x, y}
function hatta.GameObject:GetPos() end

---存活帧数
---@return number
function hatta.GameObject:GetTimer() end

---动画计时器（与存活帧数同步自增，供脚本做动画用）
---@return number
function hatta.GameObject:GetAniTimer() end

---设置碰撞组（会同步管理器的碰撞组链表）
---@param group number 组号，取值 [0,16)
function hatta.GameObject:SetGroup(group) end

---当前碰撞组
---@return number
function hatta.GameObject:GetGroup() end

---设置渲染图层（会同步管理器的有序渲染集；layer 小者先画）
---@param layer number
function hatta.GameObject:SetLayer(layer) end

---当前图层
---@return number
function hatta.GameObject:GetLayer() end

---请求销毁：本帧末统一回收（帧内随时调用都安全）
function hatta.GameObject:Destroy() end

---立即销毁：语义上「这一帧就必须消失」
function hatta.GameObject:Kill() end

---对象是否仍然存活（未被销毁、也未被回收）
---@return boolean
function hatta.GameObject:IsAlive() end
