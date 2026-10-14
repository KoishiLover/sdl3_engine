local taskmgr = require("packages.scripts.library.task")

---@class Scene
local Scene = {}

function Scene:Init(name,mgr)
    self.name = name
    self.tasks = taskmgr.create()
    self.timer = 0
    self.alpha = 0
    self.mgr = mgr
end

function Scene:Update()
    self.timer = self.timer + 1
end

function Scene:Render() end

function Scene:Leave()
    self.tasks:add(function()
        for i = 29,0,-1 do
            self.alpha = i/29
            self.tasks:wait(1)
        end
    end)
end

function Scene:Enter()
    self.tasks:add(function()
        for i = 0,29 do
            self.alpha = i/29
            self.tasks:wait(1)
        end
    end)
end

function Scene:Create()
    local instance = {}
    for k,v in pairs(self) do
        if(type(v) == "function")then
            instance[k]=v
        end
    end
    instance:Init()
    return instance
end

return Scene