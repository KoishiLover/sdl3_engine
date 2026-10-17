--- 协程管理器
---author by 帽子屋

---@class TaskManager
local TaskManager = {}
function TaskManager:init()
    self.list ={}
    self.size = 0
    self.lock =false
    self.request_clear = false
end

---@desc 向协程管理器添加一个协程
---@param f fun()
---@return thread
function TaskManager:add(f)
    local task = coroutine.create(f)
    local index = self.size+1
    self.list[index] = task
    self.size = index
    return task
end

---@desc 移出某个协程
---@param task thread
function TaskManager:remove(task)
    local list = self.list
    local size = self.size
    for i = 1,size do
        if(list[i] == task) then
            list[i] =nil
        end
    end
end

---@desc 恢复协程执行
function TaskManager:resume()
    self.lock = true
    local list = self.list
    local size = self.size
    for index = 1,size do
        local task = list[index]
        if task and coroutine.status(task)~="dead" then
            local result,err = coroutine.resume(task)
            if not result then
                local msg = tostring(err)
                msg = msg.."\n========== coroutine traceback ==========\n"
                msg = msg..debug.traceback(task)
                msg = msg.."\n========== traceback =========="
                error(msg)
            end
        end
    end
    if self.request_clear then
        self.request_clear = false
        self.size = 0
    end
    self.lock = false
end

---@desc 重新排列使列表连续
function TaskManager:sort()
    local list = self.list
    local load_index = 0
    local store_index =0
    local scan_end = self.size
    while load_index <scan_end do
        load_index = load_index+1
        local task = list[load_index]
        if task and coroutine.status(task) ~="dead" then
            store_index = store_index + 1
            if load_index ~= store_index then
                list[store_index] = task
            end
        end
    end
    self.size = store_index
end

function TaskManager:clear()
    if self.lock then
        self.request_clear = true
    else
        self.size = 0
    end
end

---@param frames number
function TaskManager:wait(frames)
    for _ = 1, frames do
        coroutine.yield()
    end
end

---@nodiscard
---@return TaskManager
function TaskManager.create()
    local instance = {}
    instance.init = TaskManager.init
    instance.add = TaskManager.add
    instance.remove =TaskManager.remove
    instance.sort = TaskManager.sort
    instance.resume = TaskManager.resume
    instance.clear = TaskManager.clear
    instance.wait = TaskManager.wait
    instance:init()
    return instance
end

return TaskManager