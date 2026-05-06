-- Touch-driven shape test app
-- Rectangle when idle
-- Circle while finger is held down

local isTouching = false
local touchX = 300
local touchY = 350

function start()
    print("Touch shape test started!")
end

-- Finger down
function began(id, x, y)
    isTouching = true
    touchX = x
    touchY = y
end

-- Finger move (optional but useful)
function going(id, x, y)
    touchX = x
    touchY = y
end

-- Finger up
function ended(id, x, y)
    isTouching = false
end

function update(dt)
    if isTouching then
        -- Draw circle while touching
        leafy.circle(touchX, touchY, 60, 0)
    else
        -- Draw rectangle when idle
        leafy.rect(200, 275, 200, 150, 0)
    end
end