-- =========================
-- LAUNCHER APP - REDESIGNED
-- =========================
SCREEN_W = 600
SCREEN_H = 800

-- App definitions with maximum contrast
local APPS = {
    {id=1, name="1:Settings", color=15},   -- Almost black
    {id=2, name="2:Calendar", color=40},   
    {id=3, name="3:Music", color=90},      
    {id=4, name="4:Books", color=140},     
    {id=5, name="5:Clock", color=190},     
    {id=6, name="6:Weather", color=245},   -- Almost white
    {id=7, name="7:Camera", color=25},     
    {id=8, name="8:Gallery", color=65},    
    {id=9, name="9:Email", color=115},     
    {id=10, name="10:Notes", color=165},   
    {id=11, name="11:Maps", color=215},    
    {id=12, name="12:Store", color=255},   -- Pure white
}

local APPS_PER_PAGE = 6
local current_page = 0
local dialog_app_id = nil
local app_buttons = {}
local prev_btn = nil
local next_btn = nil

-- =========================
-- GRID COMPONENT
-- =========================
function Grid(x, y, width, height, rows, cols, spacing)
    spacing = spacing or 5
    local cell_w = (width - spacing * math.max(0, cols - 1)) / cols
    local cell_h = (height - spacing * math.max(0, rows - 1)) / rows
    
    return {
        x = x, y = y, width = width, height = height,
        rows = rows, cols = cols, spacing = spacing,
        cell_w = cell_w, cell_h = cell_h,
        
        tile = function(self, row, col, row_span, col_span)
            row_span = row_span or 1
            col_span = col_span or 1
            local tile_x = math.floor(self.x + (col - 1) * (self.cell_w + self.spacing))
            local tile_y = math.floor(self.y + (row - 1) * (self.cell_h + self.spacing))
            local tile_w = math.floor(col_span * self.cell_w + math.max(0, col_span - 1) * self.spacing)
            local tile_h = math.floor(row_span * self.cell_h + math.max(0, row_span - 1) * self.spacing)
            return tile_x, tile_y, tile_w, tile_h
        end
    }
end

-- =========================
-- TEXTURE LOADING
-- =========================
local textures = {}

function load_textures()
    -- App icon textures
    textures.app_settings = render.loadTexture("/mnt/us/textures/app_settings.ppm") or nil
    textures.app_calendar = render.loadTexture("/mnt/us/textures/app_calendar.ppm") or nil
    textures.app_music = render.loadTexture("/mnt/us/textures/app_music.ppm") or nil
    textures.app_books = render.loadTexture("/mnt/us/textures/app_books.ppm") or nil
    textures.app_clock = render.loadTexture("/mnt/us/textures/app_clock.ppm") or nil
    textures.app_weather = render.loadTexture("/mnt/us/textures/app_weather.ppm") or nil
    
    -- Status bar widget textures
    textures.widget_weather = render.loadTexture("/mnt/us/textures/widget_weather.ppm") or nil
    textures.widget_time = render.loadTexture("/mnt/us/textures/widget_time.ppm") or nil
    textures.widget_system = render.loadTexture("/mnt/us/textures/widget_system.ppm") or nil
    textures.widget_battery = render.loadTexture("/mnt/us/textures/widget_battery.ppm") or nil
    
    print("[LAUNCHER] Textures loaded")
end

function get_app_texture(app_id)
    if not textures or app_id < 1 or app_id > #APPS then
        return nil
    end
    
    local texture_map = {
        textures.app_settings, textures.app_calendar, textures.app_music,
        textures.app_books, textures.app_clock, textures.app_weather,
        textures.app_settings, textures.app_calendar, textures.app_music,
        textures.app_books, textures.app_clock, textures.app_weather
    }
    
    if #texture_map == 0 then
        return nil
    end
    
    local idx = ((app_id - 1) % #texture_map) + 1
    return texture_map[idx]
end

-- =========================
-- CALLBACKS
-- =========================
callbacks.register("began", function(id, x, y)
end)

callbacks.register("ended", function(id, x, y)
end)
function start()
    print("[LAUNCHER] Starting")
    load_textures()
    
    -- APP GRID AREA
    local app_grid = Grid(30, 80, 540, 240, 2, 3, 15)
    
    -- Create app buttons
    local function create_app_buttons()
        app_buttons = {}
        
        for i = 1, APPS_PER_PAGE do
            local app_idx = current_page * APPS_PER_PAGE + i
            if app_idx <= #APPS then
                local app = APPS[app_idx]
                local row = ((i - 1) % 2) + 1
                local col = math.floor((i - 1) / 2) + 1
                
                local x, y, w, h = app_grid:tile(row, col)
                local btn = ui.create("Button")
                ui.set(btn, "x", x)
                ui.set(btn, "y", y)
                ui.set(btn, "w", w)
                ui.set(btn, "h", h)
                ui.set(btn, "bgColor", app.color)
                
                local function on_click()
                    print("[LAUNCHER] Opened " .. app.name)
                    dialog_app_id = app.id
                    show_dialog(app)
                end
                
                ui.onClick(btn, on_click)
                ui.addToRoot(btn)
                table.insert(app_buttons, btn)
            end
        end
        
        print("[LAUNCHER] Created " .. #app_buttons .. " buttons")
    end
    
    create_app_buttons()
    
    -- NAVIGATION BUTTONS
    prev_btn = ui.create("Button")
    ui.set(prev_btn, "x", 10)
    ui.set(prev_btn, "y", 330)
    ui.set(prev_btn, "w", 60)
    ui.set(prev_btn, "h", 80)
    ui.set(prev_btn, "bgColor", 80)
    ui.onClick(prev_btn, function()
        if current_page > 0 then
            current_page = current_page - 1
            print("[LAUNCHER] <- Page " .. current_page)
            create_app_buttons()
        end
    end)
    ui.addToRoot(prev_btn)
    
    next_btn = ui.create("Button")
    ui.set(next_btn, "x", 530)
    ui.set(next_btn, "y", 330)
    ui.set(next_btn, "w", 60)
    ui.set(next_btn, "h", 80)
    ui.set(next_btn, "bgColor", 80)
    ui.onClick(next_btn, function()
        local max_pages = math.ceil(#APPS / APPS_PER_PAGE) - 1
        if current_page < max_pages then
            current_page = current_page + 1
            print("[LAUNCHER] -> Page " .. current_page)
            create_app_buttons()
        end
    end)
    ui.addToRoot(next_btn)
    
    -- STATUS BAR (MAIN BOTTOM SECTION) - LIGHT BACKGROUND WITH WIDGETS
    local status_bar = ui.create("Frame")
    ui.set(status_bar, "x", 0)
    ui.set(status_bar, "y", 430)
    ui.set(status_bar, "w", SCREEN_W)
    ui.set(status_bar, "h", 370)
    ui.set(status_bar, "bgColor", 235)
    ui.addToRoot(status_bar)
    
    -- WIDGET BOXES - Weather, Time, System
    local weather_box = ui.create("Frame")
    ui.set(weather_box, "x", 20)
    ui.set(weather_box, "y", 450)
    ui.set(weather_box, "w", 160)
    ui.set(weather_box, "h", 100)
    ui.set(weather_box, "bgColor", 210)
    ui.addToRoot(weather_box)
    
    local time_box = ui.create("Frame")
    ui.set(time_box, "x", 200)
    ui.set(time_box, "y", 450)
    ui.set(time_box, "w", 160)
    ui.set(time_box, "h", 100)
    ui.set(time_box, "bgColor", 210)
    ui.addToRoot(time_box)
    
    local system_box = ui.create("Frame")
    ui.set(system_box, "x", 380)
    ui.set(system_box, "y", 450)
    ui.set(system_box, "w", 200)
    ui.set(system_box, "h", 100)
    ui.set(system_box, "bgColor", 210)
    ui.addToRoot(system_box)
    
    -- BATTERY LEVEL BOX
    local battery_box = ui.create("Frame")
    ui.set(battery_box, "x", 20)
    ui.set(battery_box, "y", 570)
    ui.set(battery_box, "w", 540)
    ui.set(battery_box, "h", 80)
    ui.set(battery_box, "bgColor", 210)
    ui.addToRoot(battery_box)
    
    print("[LAUNCHER] Ready!")
end

function show_dialog(app)
    -- Dialog background (semi-dark)
    local dialog_bg = ui.create("Frame")
    ui.set(dialog_bg, "x", 50)
    ui.set(dialog_bg, "y", 200)
    ui.set(dialog_bg, "w", 500)
    ui.set(dialog_bg, "h", 250)
    ui.set(dialog_bg, "bgColor", 170)
    ui.addToRoot(dialog_bg)
    
    -- Close button
    local close_btn = ui.create("Button")
    ui.set(close_btn, "x", 480)
    ui.set(close_btn, "y", 210)
    ui.set(close_btn, "w", 60)
    ui.set(close_btn, "h", 40)
    ui.set(close_btn, "bgColor", 80)
    ui.onClick(close_btn, function()
        print("[LAUNCHER] Closed " .. app.name)
        dialog_app_id = nil
    end)
    ui.addToRoot(close_btn)
end

function update(dt)
    -- =============================
    -- HEADER SECTION
    -- =============================
    -- Top bar background
    render.polygon({
        {0, 0}, {SCREEN_W, 0},
        {SCREEN_W, 40}, {0, 40}
    }, 240)
    
    -- Top bar border
    render.polygon({
        {0, 38}, {SCREEN_W, 38},
        {SCREEN_W, 42}, {0, 42}
    }, 0)
    
    -- =============================
    -- APP BUTTON ICONS (TEXTURES)
    -- =============================
    local app_grid = Grid(30, 80, 540, 240, 2, 3, 15)
    for i = 1, APPS_PER_PAGE do
        local app_idx = current_page * APPS_PER_PAGE + i
        if app_idx <= #APPS then
            local app = APPS[app_idx]
            local row = ((i - 1) % 2) + 1
            local col = math.floor((i - 1) / 2) + 1
            
            local x, y, w, h = app_grid:tile(row, col)
            
            -- Draw app texture icon (centered in top of button)
            local icon_x = x + w/2 - 20
            local icon_y = y + 8
            local tex = get_app_texture(app_idx)
            if tex then
                render.drawTexture(tex, icon_x, icon_y)
            end
            
            -- App label
            local label_x = x + 5
            local label_y = y + 50
            render.text(label_x, label_y, app.name, 12, 40)
        end
    end
    
    -- =============================
    -- GRID VISUAL DIVIDERS
    -- =============================
    -- Vertical dividers (high contrast)
    render.polygon({
        {209, 80}, {216, 80},
        {216, 320}, {209, 320}
    }, 0)
    
    render.polygon({
        {388, 80}, {395, 80},
        {395, 320}, {388, 320}
    }, 0)
    
    -- Horizontal divider
    render.polygon({
        {30, 194}, {570, 194},
        {570, 201}, {30, 201}
    }, 0)
    
    -- =============================
    -- NAVIGATION ARROWS
    -- =============================
    -- Left arrow
    render.polygon({
        {32, 360},
        {48, 345},
        {48, 375}
    }, 255)
    
    -- Right arrow
    render.polygon({
        {548, 360},
        {532, 345},
        {532, 375}
    }, 255)
    
    -- =============================
    -- STATUS BAR BORDERS
    -- =============================
    -- Top border of status bar
    render.polygon({
        {0, 428}, {SCREEN_W, 428},
        {SCREEN_W, 432}, {0, 432}
    }, 0)
    
    -- Dividers in status bar
    render.polygon({
        {188, 450}, {192, 450},
        {192, 550}, {188, 550}
    }, 0)
    
    render.polygon({
        {368, 450}, {372, 450},
        {372, 550}, {368, 550}
    }, 0)
    
    -- Battery divider
    render.polygon({
        {0, 568}, {SCREEN_W, 568},
        {SCREEN_W, 572}, {0, 572}
    }, 0)
    
    -- =============================
    -- STATUS BAR WIDGET TEXTURES
    -- =============================
    -- Weather widget with texture
    if textures.widget_weather then
        render.drawTexture(textures.widget_weather, 20, 450)
    end
    render.text(30, 520, "Weather", 10, 100)
    
    -- Time widget with texture
    if textures.widget_time then
        render.drawTexture(textures.widget_time, 200, 450)
    end
    render.text(210, 520, "12:30 PM", 10, 100)
    
    -- System widget with texture
    if textures.widget_system then
        render.drawTexture(textures.widget_system, 380, 450)
    end
    render.text(390, 520, "System", 10, 100)
    
    -- =============================
    -- BATTERY VISUALIZATION
    -- =============================
    -- Battery widget texture
    if textures.widget_battery then
        render.drawTexture(textures.widget_battery, 20, 570)
    end
    
    -- Battery label
    render.text(200, 640, "Battery: 80%", 10, 100)
    
    -- =============================
    -- PAGE INDICATOR
    -- =============================
    local max_pages = math.ceil(#APPS / 6)
    for p = 0, max_pages - 1 do
        local dot_x = 270 + p * 20
        if p == current_page then
            render.circle(dot_x, 720, 7, 0)  -- Filled dark
        else
            render.circle(dot_x, 720, 6, 120)  -- Outline light
        end
    end
    
    -- Page number label
    render.text(280, 740, "Page " .. (current_page + 1) .. "/" .. max_pages, 10, 100)
    
    -- =============================
    -- BOTTOM DECORATIONS
    -- =============================
    -- Bottom bar
    render.polygon({
        {0, 755}, {SCREEN_W, 755},
        {SCREEN_W, 800}, {0, 800}
    }, 220)
    
    -- Bottom bar border
    render.polygon({
        {0, 752}, {SCREEN_W, 752},
        {SCREEN_W, 756}, {0, 756}
    }, 0)
    
    -- Version label
    render.text(250, 775, "Leafy v1.0", 9, 100)
end