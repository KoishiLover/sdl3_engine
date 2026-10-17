---@meta
---- 一些框架方法
----@author 帽子屋小姐

---note设置窗口化
---@param v boolean
function hatta.SetWindowed(v)
end

---note 设置垂直同步
---@param v boolean
function hatta.SetVsync(v)
end

---note 设置目标帧率
---@param fps number @必须是整数
function hatta.SetFPS(fps)
end

---note 设置窗口分辨率
---@param w number
---@param h number
function hatta.SetResolution(w,h)
end

---note 设置窗口标题
---@param title string
function hatta.SetTitle(title)
end

---note 请求退出游戏：状态置为 OnExit，帧循环结束后清理资源并正常退出（退出码 0）
function hatta.GameExit()
end

