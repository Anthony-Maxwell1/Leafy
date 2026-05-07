-- =========================
-- LAUNCHER APP
-- =========================
SCREEN_W = 600
SCREEN_H = 800

-- App definitions
local APPS = {
    {id=1, name="Settings", color=100},
    {id=2, name="Calendar", color=150},
    {id=3, name="Music", color=200},
    {id=4, name="Books", color=50},
    {id=5, name="Clock", color=80},
    {id=6, name="Weather", color=120},
    {id=7, name="Camera", color=180},
    {id=8, name="Gallery", color=140},
    {id=9, name="Email", color=160},
    {id=10, name="Notes", color=110},
    {id=11, name="Maps", color=130},
    {id=12, name="Store", color=170},
}

local APPS_PER_PAGE = 6  -- 3x2 grid
local current_page = 0
local dialog_open = false
local dialog_app_name = ""

-- =========================
-- STATE
-- =========================
local app_grid = nil
local prev_btn = nil
local next_btn = nil
local page_info_frame = nil
local status_bar = nil
local dialog_frame = nil
local dialog_close_btn = nil
local weather_frame = nil
local time_frame = nil
local system_frame = nil

-- =========================
-- UTILITY FUNCTIONS
-- =========================
function resolve_size(value, dimension)
    if value < 1 then
        return math.floor(dimension * value)
    else
        return math.floor(value)
    end
end

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
-- HSTACK COMPONENT
-- =========================
function HStack(x, y, width, height)
    return {
        x = x, y = y, width = width, height = height,
        children = {},
        
        add = function(self, child)
            table.insert(self.children, child)
            self:_layout()
        end,
        
        _layout = function(self)
            if #self.children == 0 then return end
            local child_width = math.floor(self.width / #self.children)
            for i, child in ipairs(self.children) do
                local child_x = math.floor(self.x + (i - 1) * child_width)
                ui.set(child, "x", child_x)
                ui.set(child, "y", self.y)
                ui.set(child, "w", child_width)
                ui.set(child, "h", self.height)
            end
        end
    }
end

-- =========================
-- CALLBACKS
-- =========================
callbacks.register("began", function(id, x, y)
    -- print("[TOUCH] BEGIN at (" .. x .. "," .. y .. ")")
end)

callbacks.register("ended", function(id, x, y)
    -- print("[TOUCH] END at (" .. x .. "," .. y .. ")")
end)

-- =========================
-- START FUNCTION
-- =========================
function start()
    print("[LAUNCHER] Starting launcher app")
    print("[LAUNCHER] Screen: " .. SCREEN_W .. "x" .. SCREEN_H)
    
    -- HEADER AREA (50px)
    local header_frame = ui.create("Frame")
    ui.set(header_frame, "x", 0)
    ui.set(header_frame, "y", 0)
    ui.set(header_frame, "w", SCREEN_W)
    ui.set(header_frame, "h", 40)
    ui.addToRoot(header_frame)
    print("[LAUNCHER] Created header")
    
    -- APP GRID AREA (350px)
    app_grid = Grid(20, 60, 560, 320, 2, 3, 15)
    
    local app_buttons = {}
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
            
            ui.onClick(btn, function()
                print("[LAUNCHER] App '" .. app.name .. "' tapped!")
                dialog_app_name = app.name
                dialog_open = true
                show_app_dialog(app.name)
            end)
            
            ui.addToRoot(btn)
            app_buttons[i] = {btn=btn, app=app}
        end
    end
    print("[LAUNCHER] Created " .. #app_buttons .. " app buttons for page " .. current_page)
    
    -- NAVIGATION AREA (50px)
    prev_btn = ui.create("Button")
    ui.set(prev_btn, "x", 20)
    ui.set(prev_btn, "y", 400)
    ui.set(prev_btn, "w", 50)
    ui.set(prev_btn, "h", 40)
    ui.onClick(prev_btn, function()
        if current_page > 0 then
            current_page = current_page - 1
            print("[LAUNCHER] Navigated to page " .. current_page)
            reset_app_grid()
        end
    end)
    ui.addToRoot(prev_btn)
    
    -- Page info
    page_info_frame = ui.create("Frame")
    ui.set(page_info_frame, "x", 280)
    ui.set(page_info_frame, "y", 400)
    ui.set(page_info_frame, "w", 40)
    ui.set(page_info_frame, "h", 40)
    ui.addToRoot(page_info_frame)
    
    next_btn = ui.create("Button")
    ui.set(next_btn, "x", 530)
    ui.set(next_btn, "y", 400)
    ui.set(next_btn, "w", 50)
    ui.set(next_btn, "h", 40)
    ui.onClick(next_btn, function()
        local max_pages = math.ceil(#APPS / APPS_PER_PAGE) - 1
        if current_page < max_pages then
            current_page = current_page + 1
            print("[LAUNCHER] Navigated to page " .. current_page)
            reset_app_grid()
        end
    end)
    ui.addToRoot(next_btn)
    print("[LAUNCHER] Created navigation buttons")
    
    -- STATUS BAR AREA (100px bottom)
    status_bar = ui.create("Frame")
    ui.set(status_bar, "x", 0)
    ui.set(status_bar, "y", SCREEN_H - 100)
    ui.set(status_bar, "w", SCREEN_W)
    ui.set(status_bar, "h", 100)
    ui.addToRoot(status_bar)
    
    -- Weather widget in status bar
    weather_frame = ui.create("Frame")
    ui.set(weather_frame, "x", 20)
    ui.set(weather_frame, "y", SCREEN_H - 90)
    ui.set(weather_frame, "w", 120)
    ui.set(weather_frame, "h", 80)
    ui.addToRoot(weather_frame)
    
    -- Time widget in status bar
    time_frame = ui.create("Frame")
    ui.set(time_frame, "x", 160)
    ui.set(time_frame, "y", SCREEN_H - 90)
    ui.set(time_frame, "w", 120)
    ui.set(time_frame, "h", 80)
    ui.addToRoot(time_frame)
    
    -- System widget in status bar (battery, signal, etc)
    system_frame = ui.create("Frame")
    ui.set(system_frame, "x", 300)
    ui.set(system_frame, "y", SCREEN_H - 90)
    ui.set(system_frame, "w", 280)
    ui.set(system_frame, "h", 80)
    ui.addToRoot(system_frame)
    
    print("[LAUNCHER] Created status bar with widgets")
    print("[LAUNCHER] Launcher ready!")
end

function show_app_dialog(app_name)
    print("[LAUNCHER] Showing dialog for '" .. app_name .. "'")
    
    -- Modal background (semi-transparent frame covering whole screen)
    local modal_bg = ui.create("Frame")
    ui.set(modal_bg, "x", 0)
    ui.set(modal_bg, "y", 0)
    ui.set(modal_bg, "w", SCREEN_W)
    ui.set(modal_bg, "h", SCREEN_H)
    ui.addToRoot(modal_bg)
    
    -- Dialog box
    dialog_frame = ui.create("Frame")
    ui.set(dialog_frame, "x", 50)
    ui.set(dialog_frame, "y", 200)
    ui.set(dialog_frame, "w", 500)
    ui.set(dialog_frame, "h", 300)
    ui.addToRoot(dialog_frame)
    
    -- Close button
    dialog_close_btn = ui.create("Button")
    ui.set(dialog_close_btn, "x", 450)
    ui.set(dialog_close_btn, "y", 210)
    ui.set(dialog_close_btn, "w", 80)
    ui.set(dialog_close_btn, "h", 30)
    ui.onClick(dialog_close_btn, function()
        print("[LAUNCHER] Closing dialog")
        dialog_open = false
        close_app_dialog()
    end)
    ui.addToRoot(dialog_close_btn)
    
    print("[LAUNCHER] Dialog setup complete")
end

function close_app_dialog()
    -- In a full implementation, we would remove the dialog frames
    -- For now, just log it
    print("[LAUNCHER] Dialog closed")
end

function reset_app_grid()
    print("[LAUNCHER] Resetting app grid for page " .. current_page)
    -- Recreate app grid buttons for new page
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
            
            ui.onClick(btn, function()
                print("[LAUNCHER] App '" .. app.name .. "' tapped!")
                dialog_app_name = app.name
                dialog_open = true
                show_app_dialog(app.name)
            end)
            
            ui.addToRoot(btn)
        end
    end
    print("[LAUNCHER] App grid reset complete")
end

function update(dt)
    -- Draw decorative elements and icons
    -- App icon indicators in grid area
    render.circle(100, 120, 15, 100)  -- Circle icon
    render.rect(250, 120, 20, 20, 150)  -- Square icon
    render.polygon({  -- Triangle icon
        {400, 100},
        {420, 140},
        {380, 140}
    }, 200)
    
    -- Status bar indicators
    render.circle(70, SCREEN_H - 40, 10, 80)  -- Weather cloud icon
    render.rect(195, SCREEN_H - 40, 12, 12, 120)  -- Clock icon
    render.circle(350, SCREEN_H - 40, 8, 160)  -- Signal strength icon
    
    if dialog_open then
        -- Draw dialog decorative border
        render.polygon({
            {50, 200},
            {550, 200},
            {550, 500},
            {50, 500}
        }, 50)
    end
end