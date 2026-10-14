---@class Class 对象创建工具
---New 等价于 Object.New，但会先把类的 __index 指向类自身。
---Object.New 只负责把「实例表的 metatable」指向类表；真正让 instance:method() 生效的是类表的 __index。
---把这一步收进 New 里，类定义就只需要 local X = {}，不会再漏。
local Class = {}

---创建一个游戏对象
---@param base table 脚本类，必须实现 init
---@param ... any 传给 init 的额外实参
---@return hatta.GameObject|nil 新对象；对象池满时返回 nil
function Class.New(base, ...)
    base.__index = base
    return hatta.Object.New(base, ...)
end

return Class
