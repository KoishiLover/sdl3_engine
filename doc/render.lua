---@author 帽子屋小姐
---渲染接口
---@meta


---@class hatta.BlendMode 混合模式
hatta.BlendMode = {
    None = 0x00000000,
    Blend = 0x00000001,
    Add = 0x00000002,
    Mod = 0x00000004,
    Mul = 0x00000008,
}

---@param tex string 纹理名称
---@param path string 路径
--- 从指定的路径加载纹理资源
function hatta.LoadTexture(tex ,path)

end

---@param img string 图像名称
---@param tex string 纹理名称
---@param x number 图像左上角在纹理中的位置(相对于左上角)
---@param y number 图像左上角在纹理中的位置(相对于左上角)
---@param w number 图像的宽度
---@param h number 图像的高度
---从已加载的纹理中加载图像资源
function hatta.LoadImage(img,tex,x,y,w,h)

end

---@param img string 要渲染的图像名称
---@param x number 图像中心的位置
---@param y number 图像中心的位置
---@param rot number 旋转角度
---@param scale number 缩放
---将加载的图像资源以指定坐标，旋转，缩放绘制
function hatta.DrawImage(img,x,y,rot,scale)

end

---@param img string 图像名称
---@param blend hatta.BlendMode 指定图像的混合模式
---@param r number 图像颜色的 r通道
---@param g number 图像颜色的 g通道
---@param b number 图像颜色的 b通道
---@param a number 图像颜色的 a通道
---设置图像的混合和颜色模式
function hatta.SetImageState(img,blend,r,g,b,a)

end

---@param scale number
---设置图像的全局缩放 TODO(未实现)
function hatta.SetImageScale(scale)

end

---@param img string 图像名称
---@param x  number 中心坐标
---@param y  number 中心坐标
---设置图像渲染中心
function hatta.SetImageCenter(img,x,y)

end

---@param w number 渲染目标的逻辑宽度（必须为整数）
---@param h number  渲染目标的逻辑高度（必须为整数）
---设置渲染目标的逻辑大小
function hatta.SetLogicalPresentSize(w,h)

end