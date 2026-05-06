-- print("Leafy test app starting")

-- =========================
-- CALLBACKS
-- =========================
callbacks.register("began", function(id, x, y)
    -- print("TOUCH BEGIN", id, x, y)
end)

callbacks.register("going", function(id, x, y)
    -- movement spam is normal
end)

callbacks.register("ended", function(id, x, y)
    -- print("TOUCH END", id, x, y)
end)

-- =========================
-- RENDER TEST
-- =========================
function start()
    -- print("start() called")

    render.circle(100, 100, 40, 50)
    render.rect(20, 20, 80, 60, 100)

    render.pixel(10, 10, 255)

    render.polygon(4, {
        {150, 150},
        {200, 150},
        {200, 200},
        {150, 200}
    }, 80)

    -- =========================
    -- UI TEST
    -- =========================
    local frame = ui.create("Frame")
    ui.set(frame, "x", 50)
    ui.set(frame, "y", 50)
    ui.set(frame, "w", 120)
    ui.set(frame, "h", 80)

    local btn = ui.create("Button")
    ui.set(btn, "x", 60)
    ui.set(btn, "y", 60)
    ui.set(btn, "w", 100)
    ui.set(btn, "h", 40)

    ui.onClick(btn, function()
        -- print("BUTTON CLICKED")
    end)

    ui.add(frame, btn)

    local cb = ui.create("Checkbox")
    ui.set(cb, "x", 60)
    ui.set(cb, "y", 120)

    ui.onToggle(cb, function(state)
        -- print("CHECKBOX:", state)
    end)

    ui.add(frame, cb)
end

function update(dt)
    -- animate something simple
    render.circle(120, 160, 20, 120)
end