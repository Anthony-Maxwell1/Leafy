-- ============================================================
-- LEAFY LAUNCHER  –  Kindle-style home screen
-- ============================================================

SCREEN_W = 600
SCREEN_H = 800

-- ── Layout constants ─────────────────────────────────────────
local STATUS_H   = 40     -- top status bar height
local DOCK_H     = 90     -- bottom dock bar height
local HEADER_H   = STATUS_H
local CONTENT_Y  = STATUS_H + 6
local CONTENT_H  = SCREEN_H - HEADER_H - DOCK_H - 12
local APPGRID_Y  = CONTENT_Y
local APPGRID_H  = CONTENT_H

-- ── Grayscale palette (e-ink friendly) ───────────────────────
local C = {
    BLACK      = 0,
    DARK       = 20,
    MID_DARK   = 60,
    MID        = 120,
    MID_LIGHT  = 160,
    LIGHT      = 200,
    NEAR_WHITE = 230,
    WHITE      = 255,
}

-- ── App definitions ───────────────────────────────────────────
local APPS = {
    {name="Settings", icon="settings"},
    {name="Calendar", icon="calendar"},
    {name="Music",    icon="music"},
    {name="Books",    icon="books"},
    {name="Clock",    icon="clock"},
    {name="Weather",  icon="weather"},
    {name="Camera",   icon="camera"},
    {name="Gallery",  icon="gallery"},
    {name="Email",    icon="email"},
    {name="Notes",    icon="notes"},
    {name="Maps",     icon="maps"},
    {name="Store",    icon="store"},
}

-- Dock apps (always visible)
local DOCK_APPS = {
    {name="Phone",    icon="email"},
    {name="Docs",     icon="notes"},
    {name="Store",    icon="store"},
    {name="Settings", icon="settings"},
}

local APPS_PER_PAGE = 8     -- 4 cols × 2 rows
local COLS = 4
local ROWS = 2

-- ── State ─────────────────────────────────────────────────────
local current_page  = 0
local total_pages   = math.ceil(#APPS / APPS_PER_PAGE)
local dialog        = nil   -- {app, bg, close_btn, title_text}
local page_views    = {}    -- current page buttons
local font_sm       = nil
local font_md       = nil
local font_lg       = nil
local icon_textures = {}    -- name → Texture*
local ic_weather    = nil
local ic_clock      = nil
local ic_battery    = nil
local ic_wifi       = nil

-- ── Helpers ───────────────────────────────────────────────────
local function btn(x, y, w, h, bg, cb)
    local b = ui.create("Button")
    ui.set(b, "x", x)
    ui.set(b, "y", y)
    ui.set(b, "w", w)
    ui.set(b, "h", h)
    ui.set(b, "bgColor", bg)
    if cb then ui.onClick(b, cb) end
    return b
end

local function frame(x, y, w, h, bg)
    local f = ui.create("Frame")
    ui.set(f, "x", x)
    ui.set(f, "y", y)
    ui.set(f, "w", w)
    ui.set(f, "h", h)
    ui.set(f, "bgColor", bg)
    return f
end

local function label(x, y, w, h, text, color, bg, font)
    local t = ui.create("Text")
    ui.set(t, "x", x)
    ui.set(t, "y", y)
    ui.set(t, "w", w)
    ui.set(t, "h", h)
    ui.set(t, "text", text)
    ui.set(t, "color", color or C.BLACK)
    if bg ~= nil then
        ui.set(t, "bgColor", bg)
    else
        ui.set(t, "hasBackground", false)
    end
    if font then ui.set(t, "font", font) end
    return t
end

-- ── Texture loading ───────────────────────────────────────────
local TEX_DIR = "/mnt/us/textures/"

local function load_icon(name)
    local t = render.loadTexture(TEX_DIR .. name .. ".png")
    if t then
        icon_textures[name] = t
        print("[Launcher] Loaded icon: " .. name)
    else
        print("[Launcher] WARN: missing icon " .. name)
    end
    return t
end

-- ── Page rendering ────────────────────────────────────────────
-- Cell layout within the app grid area
local CELL_PAD_X = 12
local CELL_PAD_Y = 8
local ICON_SIZE  = 64

local function cell_rect(col, row)
    -- col 1..COLS, row 1..ROWS
    local total_w = SCREEN_W - 2 * CELL_PAD_X
    local cell_w  = math.floor(total_w / COLS)
    local total_h = APPGRID_H
    local cell_h  = math.floor(total_h / ROWS)

    local x = CELL_PAD_X + (col-1)*cell_w
    local y = APPGRID_Y  + (row-1)*cell_h
    return x, y, cell_w, cell_h
end

local function rebuild_page()
    ui.clearRoot()
    page_views = {}

    local start = current_page * APPS_PER_PAGE + 1
    local slot  = 0

    for i = start, math.min(start + APPS_PER_PAGE - 1, #APPS) do
        local app  = APPS[i]
        slot = slot + 1
        local col  = ((slot-1) % COLS) + 1
        local row  = math.floor((slot-1) / COLS) + 1

        local cx, cy, cw, ch = cell_rect(col, row)

        -- App button (light background, border)
        local b = btn(cx+4, cy+4, cw-8, ch-8, C.WHITE, function()
            open_dialog(app, i)
        end)
        ui.set(b, "strokeColor", C.LIGHT)
        ui.addToRoot(b)
        table.insert(page_views, b)

        -- Icon as TextureFrame inside cell
        local icon_x = cx + math.floor(cw/2) - 32
        local icon_y = cy + 8
        local tex = icon_textures[app.icon]
        if tex then
            local tv = ui.create("TextureFrame")
            ui.set(tv, "x", icon_x)
            ui.set(tv, "y", icon_y)
            ui.set(tv, "w", ICON_SIZE)
            ui.set(tv, "h", ICON_SIZE)
            ui.set(tv, "hasBackground", false)
            ui.set(tv, "texture", tex)
            ui.addToRoot(tv)
            table.insert(page_views, tv)
        end

        -- Label below icon
        local lbl = label(cx+4, icon_y + ICON_SIZE + 2, cw-8, 22,
                          app.name, C.DARK, false, font_sm)
        ui.addToRoot(lbl)
        table.insert(page_views, lbl)
    end

    -- ── Status bar ────────────────────────────────────────────
    local sb = frame(0, 0, SCREEN_W, STATUS_H, C.DARK)
    ui.addToRoot(sb)

    -- ── Dock bar ─────────────────────────────────────────────
    local dock_y = SCREEN_H - DOCK_H
    local dock = frame(0, dock_y, SCREEN_W, DOCK_H, C.NEAR_WHITE)
    ui.set(dock, "strokeColor", C.LIGHT)
    ui.addToRoot(dock)

    -- Dock app buttons
    local dock_cell_w = math.floor(SCREEN_W / #DOCK_APPS)
    for di, dapp in ipairs(DOCK_APPS) do
        local dx = (di-1)*dock_cell_w
        local db = btn(dx+6, dock_y+8, dock_cell_w-12, DOCK_H-16, C.WHITE, function()
            open_dialog(dapp, -di)
        end)
        ui.set(db, "strokeColor", C.LIGHT)
        ui.addToRoot(db)

        local dtex = icon_textures[dapp.icon]
        if dtex then
            local dtv = ui.create("TextureFrame")
            ui.set(dtv, "x", dx + dock_cell_w//2 - 20)
            ui.set(dtv, "y", dock_y + 10)
            ui.set(dtv, "w", 40)
            ui.set(dtv, "h", 40)
            ui.set(dtv, "hasBackground", false)
            ui.set(dtv, "texture", dtex)
            ui.addToRoot(dtv)
        end
    end

    -- Page indicator dots
    local dot_y = dock_y - 18
    local total_dot_w = total_pages * 16
    local dot_start = math.floor(SCREEN_W/2 - total_dot_w/2)
    for p = 0, total_pages-1 do
        local dot = frame(dot_start + p*16 + 2, dot_y, 10, 10,
                          p == current_page and C.DARK or C.MID_LIGHT)
        ui.addToRoot(dot)
    end

    -- Prev / Next page nav areas (left/right edges of content area)
    if current_page > 0 then
        local prev_nav = btn(0, CONTENT_Y, 30, CONTENT_H, C.NEAR_WHITE, function()
            current_page = current_page - 1
            rebuild_page()
        end)
        ui.set(prev_nav, "hasBackground", false)
        ui.addToRoot(prev_nav)
    end

    if current_page < total_pages - 1 then
        local next_nav = btn(SCREEN_W-30, CONTENT_Y, 30, CONTENT_H, C.NEAR_WHITE, function()
            current_page = current_page + 1
            rebuild_page()
        end)
        ui.set(next_nav, "hasBackground", false)
        ui.addToRoot(next_nav)
    end

    print("[Launcher] Page " .. current_page+1 .. "/" .. total_pages .. " built")
end

-- ── Dialog ────────────────────────────────────────────────────
function open_dialog(app, idx)
    print("[Launcher] Opening: " .. app.name)

    -- Overlay (semi-transparent feel via light bg)
    local overlay = frame(40, 160, 520, 420, C.WHITE)
    ui.set(overlay, "strokeColor", C.DARK)
    ui.addToRoot(overlay)

    -- Close button (top-right X)
    local close = btn(508, 168, 44, 44, C.NEAR_WHITE, function()
        close_dialog()
    end)
    ui.set(close, "strokeColor", C.MID)
    ui.addToRoot(close)

    -- Title
    local title = label(60, 172, 400, 36, app.name, C.DARK, false, font_lg)
    ui.addToRoot(title)

    -- App icon (large)
    local tex = icon_textures[app.icon]
    if tex then
        local big_icon = ui.create("TextureFrame")
        ui.set(big_icon, "x", 60)
        ui.set(big_icon, "y", 224)
        ui.set(big_icon, "w", 96)
        ui.set(big_icon, "h", 96)
        ui.set(big_icon, "hasBackground", false)
        ui.set(big_icon, "texture", tex)
        ui.addToRoot(big_icon)
    end

    -- Description text
    local desc = label(168, 230, 360, 24, "Tap Open to launch " .. app.name, C.MID_DARK, false, font_sm)
    ui.addToRoot(desc)

    local info = label(168, 260, 360, 22, "Version 1.0 • Leafy Platform", C.MID, false, font_sm)
    ui.addToRoot(info)

    -- Divider
    local div = frame(60, 336, 480, 2, C.LIGHT)
    ui.addToRoot(div)

    -- Open button
    local open_btn = btn(60, 350, 220, 52, C.DARK, function()
        print("[Launcher] Launch: " .. app.name)
        close_dialog()
    end)
    ui.addToRoot(open_btn)
    local open_lbl = label(70, 360, 200, 32, "Open", C.WHITE, false, font_md)
    ui.addToRoot(open_lbl)

    -- Info button
    local info_btn = btn(300, 350, 220, 52, C.NEAR_WHITE, function()
        print("[Launcher] Info: " .. app.name)
    end)
    ui.set(info_btn, "strokeColor", C.MID)
    ui.addToRoot(info_btn)
    local info_lbl = label(310, 360, 200, 32, "Details", C.DARK, false, font_md)
    ui.addToRoot(info_lbl)

    dialog = {
        views = {overlay, close, title, div, open_btn, open_lbl, info_btn, info_lbl}
    }
    if tex then
        local big = ui.create("TextureFrame")
        ui.set(big, "x", 60) ui.set(big, "y", 224)
        ui.set(big, "w", 96) ui.set(big, "h", 96)
        ui.set(big, "hasBackground", false)
        ui.set(big, "texture", tex)
        table.insert(dialog.views, big)
    end
    table.insert(dialog.views, desc)
    table.insert(dialog.views, info)
end

function close_dialog()
    if not dialog then return end
    for _, v in ipairs(dialog.views) do
        ui.removeFromRoot(v)
    end
    dialog = nil
    print("[Launcher] Dialog closed")
end

-- ── Start ─────────────────────────────────────────────────────
function start()
    print("[Launcher] Booting...")

    -- Load fonts (small/medium/large sizes from same TTF)
    font_sm = render.loadFont("/mnt/us/test/font.ttf", 16)
    font_md = render.loadFont("/mnt/us/test/font.ttf", 22)
    font_lg = render.loadFont("/mnt/us/test/font.ttf", 28)

    if font_sm then print("[Launcher] Fonts loaded") else print("[Launcher] WARN: no font") end

    -- Load all app icons
    for _, app in ipairs(APPS) do
        load_icon(app.icon)
    end
    for _, app in ipairs(DOCK_APPS) do
        if not icon_textures[app.icon] then load_icon(app.icon) end
    end

    -- Status bar icons
    ic_weather = render.loadTexture(TEX_DIR .. "ic_weather.png")
    ic_clock   = render.loadTexture(TEX_DIR .. "ic_clock.png")
    ic_battery = render.loadTexture(TEX_DIR .. "ic_battery.png")
    ic_wifi    = render.loadTexture(TEX_DIR .. "ic_wifi.png")

    rebuild_page()
    print("[Launcher] Ready")
end

-- ── Update (per-frame drawing on top of UI) ───────────────────
function update(dt)
    -- ── Status bar content ────────────────────────────────────
    -- Left: wifi icon
    if ic_wifi then
        render.drawTexture(ic_wifi, 4, 4)
    else
        render.rect(4, 10, 20, 20, C.WHITE)
    end

    -- Center: time text
    if font_md then
        render.text(220, 30, "12:34", C.WHITE, font_md)
    end

    -- Right side icons: battery, clock
    if ic_battery then
        render.drawTexture(ic_battery, SCREEN_W - 68, 4)
    end

    -- ── Page swipe arrows ────────────────────────────────────
    -- Only draw if at edge of grid (subtle visual cue)
    if current_page > 0 then
        -- Left chevron
        render.polygon({
            {14, CONTENT_Y + CONTENT_H//2 - 18},
            {26, CONTENT_Y + CONTENT_H//2},
            {14, CONTENT_Y + CONTENT_H//2 + 18},
        }, C.MID_LIGHT)
    end
    if current_page < total_pages - 1 then
        -- Right chevron
        render.polygon({
            {SCREEN_W - 14, CONTENT_Y + CONTENT_H//2 - 18},
            {SCREEN_W - 26, CONTENT_Y + CONTENT_H//2},
            {SCREEN_W - 14, CONTENT_Y + CONTENT_H//2 + 18},
        }, C.MID_LIGHT)
    end

    -- ── Dialog X label ────────────────────────────────────────
    if dialog then
        if font_md then
            render.text(515, 200, "X", C.DARK, font_md)
        end
    end
end

-- ── Touch callbacks (passed through to UI system) ─────────────
callbacks.register("began", function(id, x, y) end)
callbacks.register("ended", function(id, x, y) end)