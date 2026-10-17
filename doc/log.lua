---@meta
---@author 帽子屋小姐
---日志输出（spdlog）
---@see hatta.LogInfo
---@see hatta.LogWarn
---@see hatta.LogErr

--[[
    日志写到 userdata/log/engine.log，每次启动都会清空重建。
    格式：[年-月-日 时:分:秒][级别] 消息

    用法示例：

        LogInfo("场景初始化完成")                       -- 级别 info
        LogWarn(string.format("纹理缺失: %s", name))    -- 级别 warning

        -- 致命错误：记录 error、弹出消息框，然后结束进程
        LogErr("无法加载必需资源，游戏退出")

    要点：
      · 消息里可以随意写 {} %d 之类的符号，不会被当成格式串解析
        （C++ 侧是把消息作为「参数」传给 std::format 的，只有 [lua] 前缀是格式串）
      · LogErr 会终止进程，只在真的无法继续时使用
      · info / warn 是带缓冲的：进程被强杀时最后几条可能丢失；
        正常退出（GameExit、关窗口、崩溃走 OnDestroy）会刷盘
]]


---记录一条 info 级别日志
---@param message string 日志内容
function hatta.LogInfo(message) end


---记录一条 warning 级别日志
---@param message string 日志内容
---@note 不会中断运行，只是留痕
function hatta.LogWarn(message) end


---记录一条 error 级别日志，随后弹出消息框并结束进程
---@param message string 日志内容
---@note 致命操作：弹框会阻塞，确认后进程退出（error 级别会立即刷盘）
---@note 只是想在日志里留个错误、不想退出的话，用 LogWarn
function hatta.LogErr(message) end
