---@meta
---@author 帽子屋小姐
---游戏对象系统

---@class hatta.Object 游戏对象管理器
hatta.Object = {}

---创建一个游戏对象
---@param class table 脚本类。必须实现 init；update/render/beforeUpdate/afterUpdate/collision/kill 会被引擎自动收集调用
---@param ... any 传给 init 的额外参数
---@return hatta.GameObject|nil 创建的对象；对象池满时返回 nil
---@note 对象池容量 16384
---
--- 脚本类约定（Object.New 会自动收集以下方法，均以 self 为第一个实参）：
---   function class:init(...)           创建时调用一次
---   function class:beforeUpdate()      每帧逻辑更新之前
---   function class:update()            每帧逻辑更新
---   function class:afterUpdate()       每帧逻辑更新之后
---   function class:render()            每帧渲染
---   function class:collision(other)    碰撞命中（other 为对方对象）
---   function class:kill()              被回收时调用
function hatta.Object.New(class, ...) end


---@class hatta.GameObject 游戏对象句柄
---@field x number 世界坐标 x
---@field y number 世界坐标 y
---@field vx number 每帧 x 轴位移量
---@field vy number 每帧 y 轴位移量
---@field ax number 每帧 x 轴加速度
---@field ay number 每帧 y 轴加速度
---@field rot number 角度（弧度）
---@field omega number 每帧角度增量
---@field col_r number 圆形碰撞判定半径
---@field rect_a number 矩形判定半轴 a
---@field rect_b number 矩形判定半轴 b
---@field navi boolean 渲染角度是否跟随速度方向
---@field bound boolean 是否开启出界检测（出界即消亡）
---@field colli boolean 是否参与碰撞检测（默认 true）
---@field hide boolean 是否不渲染
---@field rect boolean 是否使用矩形判定
---@field ignore_special_pause boolean 是否无视全局暂停
---@field resolve_move boolean 位移是否由脚本直接改写坐标产生
hatta.GameObject = {}

---设置坐标
---@param x number
---@param y number
function hatta.GameObject:SetPos(x, y) end

---设置速度（由大小和方向角）
---@param v number 速率
---@param ang number 方向角（弧度）
function hatta.GameObject:SetV(v, ang) end

---设置速度分量
---@param vx number
---@param vy number
function hatta.GameObject:SetV2(vx, vy) end

---设置加速度（由大小和方向角）
---@param a number 加速度大小
---@param ang number 方向角（弧度）
function hatta.GameObject:SetA(a, ang) end

---设置加速度分量
---@param ax number
---@param ay number
function hatta.GameObject:SetA2(ax, ay) end

---设置矩形碰撞判定
---@param a number 半轴 a
---@param b number 半轴 b
---@param rect boolean 是否启用矩形判定（false 时退化为圆形，半径为 min(a,b)）
function hatta.GameObject:SetRect(a, b, rect) end

---当前速率
---@return number
function hatta.GameObject:GetSpeed() end

---当前速度方向（弧度）
---@return number
function hatta.GameObject:GetDirection() end

---对象池索引（稳定）
---@return number
function hatta.GameObject:GetId() end

---对象唯一标识（每次创建递增）
---@return number
function hatta.GameObject:GetUid() end

---当前位置
---@return number[] {x, y}
function hatta.GameObject:GetPos() end

---存活帧数
---@return number
function hatta.GameObject:GetTimer() end

---动画计时器
---@return number
function hatta.GameObject:GetAniTimer() end

---设置碰撞组（同步管理器的碰撞链表）
---@param group number 组号 [0,16)
function hatta.GameObject:SetGroup(group) end

---当前碰撞组
---@return number
function hatta.GameObject:GetGroup() end

---设置渲染图层（同步管理器的有序渲染集）
---@param layer number
function hatta.GameObject:SetLayer(layer) end

---当前图层
---@return number
function hatta.GameObject:GetLayer() end

---请求销毁：本帧结束时统一回收（帧内随时调用都安全）
function hatta.GameObject:Destroy() end

---立即销毁
function hatta.GameObject:Kill() end

---对象是否仍然存活
---@return boolean
function hatta.GameObject:IsAlive() end
