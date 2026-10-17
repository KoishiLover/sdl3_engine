local Utility = {}

function Utility.LoadImageGroup(img,tex,x,y,w,h,n,m)
    local id = 1
    for i = 1,m do
        for j = 1,n do
            LoadImage(img..id,tex,x+(j-1)*w,y+(i-1)*h,w,h)
            SetImageCenter(img..id, w/2, h/2)   -- 锚点设到图像中心，DrawImage 给的坐标即图像中心
            id = id + 1
        end
    end
end

return Utility