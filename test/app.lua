-- Simple shape cycling test app
-- Cycles through: rectangle (20 frames) -> circle large (20 frames) -> circle small (20 frames)

local frameCount = 0
local shapeIndex = 1

function start()
    print("Test app started!")
end

function update(dt)
    frameCount = frameCount + 1
    
    -- Determine which shape to draw based on frame count
    if frameCount <= 20 then
        -- Draw rectangle (200x150, centered around 300x350)
        leafy.rect(200, 275, 200, 150, 0)
    elseif frameCount <= 40 then
        -- Draw large circle (radius 80)
        leafy.circle(300, 350, 80, 0)
    elseif frameCount <= 60 then
        -- Draw small circle (radius 40)
        leafy.circle(300, 350, 40, 0)
    end
    
    -- Reset cycle
    if frameCount >= 60 then
        frameCount = 0
    end
end
