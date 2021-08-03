-- If LuaRocks is installed, make sure that packages installed through it are
-- found (e.g. lgi). If LuaRocks is not installed, do nothing.
pcall(require, "luarocks.loader")

-- Standard awesome library
local gears = require("gears")
local awful = require("awful")
require("awful.autofocus")
-- Widget and layout library
local wibox = require("wibox")
-- Theme handling library
local beautiful = require("beautiful")
-- Notification library
local naughty = require("naughty")

-- Needed for create_titlebar_widget_button
local imagebox = require("wibox.widget.imagebox")

-- {{{ Error handling
-- Check if awesome encountered an error during startup and fell back to
-- another config (This code will only ever execute for the fallback config)
if awesome.startup_errors then
    naughty.notify({ preset = naughty.config.presets.critical,
                     title = "Oops, there were errors during startup!",
                     text = awesome.startup_errors })
end

-- Handle runtime errors after startup
do
    local in_error = false
    awesome.connect_signal("debug::error", function (err)
        -- Make sure we don't go into an endless error loop
        if in_error then return end
        in_error = true

        naughty.notify({ preset = naughty.config.presets.critical,
                         title = "Oops, an error happened!",
                         text = tostring(err) })
        in_error = false
    end)
end
-- }}}

-- Themes define colours, icons, font and wallpapers.
beautiful.init(gears.filesystem.get_themes_dir() .. "default/theme.lua")

-- Default modkey.
-- Usually, Mod4 is the key with a logo between Control and Alt.
-- If you do not like this or do not have such a key,
-- I suggest you to remap Mod4 to another key using xmodmap or other tools.
-- However, you can use another modifier like Mod1, but it may interact with others.
modkey = "Mod1"

-- Set default mouse settings
awful.mouse.snap.edge_enabled = true
awful.mouse.snap.client_enabled = true
awful.mouse.drag_to_tag.enabled = false

-- Global keyboard shortcuts
globalkeys = gears.table.join(
    awful.key({ }, "XF86AudioPlay", function ()
      awful.util.spawn("playerctl play-pause", { hidden = true })
    end),
    awful.key({ }, "XF86AudioPause", function ()
      awful.util.spawn("playerctl play-pause", { hidden = true })
    end),
    awful.key({ }, "XF86AudioNext", function ()
      awful.util.spawn("playerctl next", { hidden = true })
    end),
    awful.key({ }, "XF86AudioPrev", function ()
      awful.util.spawn("playerctl previous", { hidden = true })
    end)
)
root.keys(globalkeys)


-- {{{ Wibar
-- Create a wibox for each screen and add it
local tasklist_buttons = gears.table.join(
                     awful.button({ }, 1, function (c)
                                              c:emit_signal(
                                                  "request::activate",
                                                  "tasklist",
                                                  {raise = true}
                                              )
                                          end),
                     awful.button({ }, 4, function ()
                                              awful.client.focus.byidx(1)
                                          end),
                     awful.button({ }, 5, function ()
                                              awful.client.focus.byidx(-1)
                                          end))

awful.screen.connect_for_each_screen(function(s)
    -- Each screen has its own tag table.
    awful.tag({ "1"}, s, awful.layout.suit.floating)

    -- Create a tasklist widget
    s.mytasklist = awful.widget.tasklist {
        screen  = s,
        filter  = awful.widget.tasklist.filter.currenttags,
        buttons = tasklist_buttons
    }

    -- Create the wibox
    s.mywibox = awful.wibar({ position = "bottom", screen = s, ontop = true })

    -- Add widgets to the wibox
    s.mywibox:setup {
        layout = wibox.layout.flex.horizontal,
        s.mytasklist,
    }
end)
-- }}}


clientkeys = gears.table.join(
    awful.key({ modkey,           }, "m",
        function (c)
            c.maximized = not c.maximized
            c:raise()
        end ,
        {description = "(un)maximize", group = "client"}),
    awful.key({ modkey, "Control" }, "m",
        function (c)
            c.maximized_vertical = not c.maximized_vertical
            c:raise()
        end ,
        {description = "(un)maximize vertically", group = "client"}),
    awful.key({ modkey, "Shift"   }, "m",
        function (c)
            c.maximized_horizontal = not c.maximized_horizontal
            c:raise()
        end ,
        {description = "(un)maximize horizontally", group = "client"})
)


clientbuttons = gears.table.join(
    awful.button({ }, 1, function (c)
        c:emit_signal("request::activate", "mouse_click", {raise = true})

        if c == awful.client.getmaster() then
            return
        end

        -- Originally based on https://www.reddit.com/r/awesomewm/comments/fr9ss3/awesomewm_as_a_floating_window_manager/flvdlga
        -- top corners are handled in `client.connect_signal("request::titlebars")`
        local bottom_corners = {
            { c.x, c.y + c.height },
            { c.x + c.width, c.y + c.height },
        }

        local m = mouse.coords()
        local distance_sq = 100

        for _, pos in ipairs(bottom_corners) do
            if (m.x - pos[1]) ^ 2 + (m.y - pos[2]) ^ 2 <= distance_sq then
                awful.mouse.client.resize(c)
                break
            end
        end
    end)
)


-- {{{ Rules
-- Rules to apply to new clients (through the "manage" signal).
awful.rules.rules = {
    -- All clients will match this rule.
    { rule = { },
      properties = { border_width = beautiful.border_width,
                     border_color = beautiful.border_normal,
                     focus = awful.client.focus.filter,
                     raise = true,
                     keys = clientkeys,
                     buttons = clientbuttons,
                     screen = awful.screen.preferred,
                     placement = awful.placement.no_offscreen,
                     attach = true,
     }
    },
    -- Add titlebars to normal clients and dialogs
    { rule_any = {type = { "normal", "dialog" }
      }, properties = { titlebars_enabled = true }
    },
}
-- }}}


-- {{{ Signals
notify = function (title, text)
  naughty.notify( { preset = naughty.config.presets.normal,
                    title = "" .. title,
                    text = "" .. text })
end


compute_table_length = function(t)
  local len = 0
  for k, v in pairs(t) do
    len = len + 1
  end
  return len
end


-- This function is a modified copy of `titlebar.widget.button` from
-- /usr/share/awesome/lib/awful/titlebar.lua to allow for running a function on
-- mouse button _press_, not just release.
function create_titlebar_widget_button(c, name, selector, action_pressed, action_released, tooltips_enabled)
    local ret = imagebox()

    if tooltips_enabled then
        ret._private.tooltip = awful.tooltip({ objects = {ret}, delay_show = 1 })
        ret._private.tooltip:set_text(name)
    end

    local function update()
        local img = selector(c)
        if type(img) ~= "nil" then
            -- Convert booleans automatically
            if type(img) == "boolean" then
                if img then
                    img = "active"
                else
                    img = "inactive"
                end
            end
            local prefix = "normal"
            if client.focus == c then
                prefix = "focus"
            end
            if img ~= "" then
                prefix = prefix .. "_"
            end
            local state = ret.state
            if state ~= "" then
                state = "_" .. state
            end
            -- First try with a prefix based on the client's focus state,
            -- then try again without that prefix if nothing was found,
            -- and finally, try a fallback for compatibility with Awesome 3.5 themes
            local theme = beautiful["titlebar_" .. name .. "_button_" .. prefix .. img .. state]
                       or beautiful["titlebar_" .. name .. "_button_" .. prefix .. img]
                       or beautiful["titlebar_" .. name .. "_button_" .. img]
                       or beautiful["titlebar_" .. name .. "_button_" .. prefix .. "_inactive"]
            if theme then
                img = theme
            end
        end
        ret:set_image(img)
    end
    ret.state = ""
    if action_pressed then
      action_pressed_function = function() action_pressed(c) end
    else
      action_pressed_function = nil
    end
    if action_released then
        ret:buttons(awful.button({ }, 1, action_pressed_function, function()
            ret.state = ""
            update()
            action_released(c, selector(c))
        end))
    else
        ret:buttons(awful.button({ }, 1, action_pressed_function, function()
            ret.state = ""
            update()
        end))
    end
    ret:connect_signal("mouse::enter", function()
        ret.state = "hover"
        update()
    end)
    ret:connect_signal("mouse::leave", function()
        ret.state = ""
        update()
    end)
    ret:connect_signal("button::press", function(_, _, _, b)
        if b == 1 then
            ret.state = "press"
            update()
        end
    end)
    ret.update = update
    update()

    -- We do magic based on whether a client is focused above, so we need to
    -- connect to the corresponding signal here.
    c:connect_signal("focus", update)
    c:connect_signal("unfocus", update)

    return ret
end



manage_taskbar_visibility = function ()
  local s = awful.screen.focused()
  length = compute_table_length(s.all_clients)

  if length <= 1 then
    s.mywibox.visible = false
  else
    s.mywibox.visible = true
  end
end


show_master_window = function ()
  local m = awful.client.getmaster()
  if m ~= nil then
    notify("Master window is now", m.name)
  end
end


function serializeTable(val, name, skipnewlines, depth)
    skipnewlines = skipnewlines or false
    depth = depth or 0

    local tmp = string.rep(" ", depth)
    if name then tmp = tmp .. name .. " = " end

    if type(val) == "table" then
        tmp = tmp .. "{" .. (not skipnewlines and "\n" or "")
        for k, v in pairs(val) do
            tmp =  tmp .. serializeTable(v, k, skipnewlines, depth + 1) .. "," .. (not skipnewlines and "\n" or "")
        end

        tmp = tmp .. string.rep(" ", depth) .. "}"
    elseif type(val) == "number" then
        tmp = tmp .. tostring(val)
    elseif type(val) == "string" then
        tmp = tmp .. string.format("%q", val)
    elseif type(val) == "boolean" then
        tmp = tmp .. (val and "true" or "false")
    else
        tmp = tmp .. "\"[inserializeable datatype:" .. type(val) .. "]\""
    end

    return tmp
end


ensure_client_is_not_offscreen = function (c)
  -- https://awesomewm.org/doc/api/classes/client.html#client.valid
  local is_valid = pcall(function() return c.valid end) and c.valid
  if not is_valid then return end

  -- make sure master takes up the whole screen
  if c == awful.client.getmaster() then
    awful.titlebar.hide(c)
    c.border_width = 0
    c:geometry(awful.screen.focused().workarea)
    return
  end

  local g = c:geometry()
  local w = awful.screen.focused().workarea

  if g.x < w.x then
    g.x = w.x
  end
  if g.x > w.x + w.width - 150 then
    g.x = w.x + w.width - 150
  end
  if g.x + g.width > w.x + w.width then
    g.width = w.x + w.width - g.x
  end
  if g.y < w.y then
    g.y = w.y
  end
  if g.y > w.y + w.height - 150 then
    g.y = w.y + w.height - 150
  end
  if g.y + g.height > w.y + w.height then
    g.height = w.y + w.height - g.y
  end

  c:geometry(g)
end


client.connect_signal("manage", function (c)
    -- Set the windows at the slave,
    -- i.e. put it at the end of others instead of setting it master.
    if not awesome.startup then awful.client.setslave(c) end

    manage_taskbar_visibility()
    awful.placement.no_offscreen(c, {honor_workarea=true})

    ensure_client_is_not_offscreen(c)
end)

client.connect_signal("unmanage", function (c)
    local numclients = compute_table_length(awful.screen.focused().all_clients)
    -- If last closed client was Chrome, focus on the first (and only) tab in its window.
    --     By sending the focus tab 1 command to the window, we prevent a new
    --     instance of Chrome opening if the user has hit the tab "x".
    if numclients == 0 and (c.class == "Google-chrome" or c.class == "Brave-browser" or c.class == "Sidekick-browser") then
      os.execute("xdotool key --window " .. c.window .. " ctrl+1")
    end
    manage_taskbar_visibility()
    ensure_client_is_not_offscreen(awful.client.getmaster())
end)

client.connect_signal("request::geometry", function (client, context, extra_args)
  if context ~= "mouse.move" then
    ensure_client_is_not_offscreen(client)
  end
end)


screen.connect_signal("property::geometry", function (s)
   for k, v in pairs(s.all_clients) do
     ensure_client_is_not_offscreen(v)
   end
end)


-- Add a titlebar if titlebars_enabled is set to true in the rules.
client.connect_signal("request::titlebars", function(c)
    -- buttons for the titlebar
    local buttons = gears.table.join(
        awful.button({ }, 1, function()
            c:emit_signal("request::activate", "titlebar", {raise = true})

            if c == awful.client.getmaster() then
                return
            end

            local m = mouse.coords()
            local distance_sq = 100

            -- check for top left corner
            if (m.x - c.x) ^ 2 + (m.y - c.y) ^ 2 <= distance_sq then
                awful.mouse.client.resize(c)
            else
                awful.mouse.client.move(c)
            end
        end),
        awful.button({ }, 3, function()
            c:emit_signal("request::activate", "titlebar", {raise = true})
            awful.mouse.client.resize(c)
        end)
    )

    awful.titlebar(c) : setup {
        { -- Left
            awful.titlebar.widget.iconwidget(c),
            buttons = buttons,
            layout  = wibox.layout.fixed.horizontal
        },
        { -- Middle
            { -- Title
                align  = "center",
                widget = awful.titlebar.widget.titlewidget(c)
            },
            buttons = buttons,
            layout  = wibox.layout.flex.horizontal
        },
        { -- Right
            awful.titlebar.widget.maximizedbutton(c),

            create_titlebar_widget_button(c, "close", function() return "" end, function(cl)
              local m = mouse.coords()
              local distance_sq = 100
              -- check for top right corner
              if (m.x - (cl.x + cl.width)) ^ 2 + (m.y - cl.y) ^ 2 <= distance_sq then
                  awful.mouse.client.resize(cl)
              else
                  cl:kill()
              end
            end, function() end, true),

            layout = wibox.layout.fixed.horizontal()
        },
        layout = wibox.layout.align.horizontal
    }
end)

client.connect_signal("focus", function(c) c.border_color = beautiful.border_focus end)
client.connect_signal("unfocus", function(c) c.border_color = beautiful.border_normal end)
-- }}}
