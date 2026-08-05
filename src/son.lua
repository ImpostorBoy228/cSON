-- son.lua — a great json alternative. why son? json was father
-- runtime version for Lua 5.4: parse + eval, no C codegen.
--
--   local son = require("son")
--   local tree = son.eval_file("config.son", { owo = "dick" })
--   print(son.get(tree, "f1"))
local son = {}

local CSON_OBJECT = "object"
local CSON_STRING = "string"
local CSON_IF     = "if"
local CSON_INLINE = "inline_lua"

son.CSON_OBJECT = CSON_OBJECT
son.CSON_STRING = CSON_STRING
son.CSON_IF     = CSON_IF
son.CSON_INLINE = CSON_INLINE

local function is_ws(b) return b == 32 or b == 9 or b == 10 or b == 13 end

local function escape_pattern(s)
    return s:gsub("(%W)", "%%%1")
end

local function resolve_exact(s, macros)
    for _, m in ipairs(macros) do
        if s == m.name then return m.value end
    end
    return s
end

local function resolve_all(s, macros)
    for _, m in ipairs(macros) do
        s = s:gsub(escape_pattern(m.name), (m.value:gsub("%%", "%%%%")))
    end
    return s
end

local function read_string(text, i)
    local out = {}
    local n = #text
    i = i + 1
    while i <= n do
        local c = text:byte(i)
        if c == 34 or c == 10 then return table.concat(out), i end
        if c == 92 and i + 1 <= n then
            i = i + 1
            local e = text:byte(i)
            local map = { [110] = "\n", [116] = "\t", [34] = '"', [92] = "\\" }
            out[#out + 1] = map[e] or string.char(e)
            i = i + 1
        else
            out[#out + 1] = string.char(c)
            i = i + 1
        end
    end
    return table.concat(out), i
end

local function parse_string(text, macros)
    local root, ifopen, pending_key, pending_kind
    local stack = {}
    local i, n = 1, #text
    while i <= n do
        local c = text:byte(i)
        if c == 59 then -- ;
            local nl = text:find("\n", i)
            i = nl and nl + 1 or n + 1
        elseif c == 35 then -- #
            local nl = text:find("\n", i)
            local e = nl and nl - 1 or n
            local name, value = text:sub(i, e):match("^#define%s+(%S+)%s*(.*)$")
            if name then
                macros[#macros + 1] = { name = name, value = value:gsub("\r$", "") }
            end
            i = nl and nl + 1 or n + 1
        elseif c == 123 then -- {
            local o = { type = CSON_OBJECT, key = nil, value = nil, children = {} }
            if root == nil then
                root = o
            elseif ifopen then
                table.insert(ifopen.children, o)
                ifopen = nil
            else
                o.key = pending_key
                pending_key = nil
                if stack[#stack] then table.insert(stack[#stack].children, o) end
            end
            stack[#stack + 1] = o
            i = i + 1
        elseif c == 125 then -- }
            ifopen = nil
            stack[#stack] = nil
            i = i + 1
        elseif c == 33 then -- !
            if text:byte(i + 1) == 33 then -- !!
                local q = i + 2
                if text:byte(q) == 34 then q = q + 1 end
                while q <= n and text:byte(q) ~= 34 and text:byte(q) ~= 10 do q = q + 1 end
                pending_kind = CSON_INLINE
                i = q + 1
            else
                local q = i + 2
                local start = q
                while q <= n do
                    local b = text:byte(q)
                    if b == 34 then
                        local j = q + 1
                        while j <= n and is_ws(text:byte(j)) do j = j + 1 end
                        if j <= n and text:byte(j) == 58 then break end
                    elseif b == 10 then
                        break
                    end
                    q = q + 1
                end
                local o = { type = CSON_IF, key = pending_key,
                            value = resolve_all(text:sub(start, q - 1), macros), children = {} }
                pending_key = nil
                if stack[#stack] then table.insert(stack[#stack].children, o) end
                ifopen = o
                i = q + 1
            end
        elseif c == 34 then
            local buf, idx = read_string(text, i)
            local j = idx + 1
            while j <= n and is_ws(text:byte(j)) do j = j + 1 end
            if j <= n and text:byte(j) == 58 then -- :
                pending_key = buf
                i = j + 1
            elseif pending_key then
                local o = { type = CSON_STRING, key = pending_key,
                            value = resolve_exact(buf, macros), children = {} }
                pending_key = nil
                if stack[#stack] then table.insert(stack[#stack].children, o) end
                i = idx + 1
            elseif pending_kind then
                local o = { type = pending_kind, key = nil,
                            value = resolve_all(buf, macros), children = {} }
                pending_kind = nil
                if stack[#stack] then table.insert(stack[#stack].children, o) end
                i = idx + 1
            else
                io.stderr:write("your son is fucked\n")
                i = idx + 1
            end
        else
            i = i + 1
        end
    end
    return root
end

local function run_inline(code, env)
    local fn, err = load("return function() " .. (code or "") .. " end", "=<son inline>", "t", env)
    if not fn then error("son: bad inline code '" .. (code or "") .. "': " .. tostring(err)) end
    return tostring(fn()())
end

local function eval_node(node, env)
    if node.type == CSON_IF then
        local cond = node.value:match("^if%s*%((.*)%)%s*$") or node.value
        local fn, err = load("return " .. cond, "=<son if>", "t", env)
        if not fn then error("son: bad if condition '" .. node.value .. "': " .. tostring(err)) end
        if not fn() then return {} end
        local body = node.children[1]
        local out = {}
        if body then
            for _, child in ipairs(body.children) do
                for _, c in ipairs(eval_node(child, env)) do out[#out + 1] = c end
            end
        end
        return out
    elseif node.type == CSON_INLINE then
        return { { type = CSON_STRING, key = node.key, value = run_inline(node.value, env), children = {} } }
    elseif node.type == CSON_OBJECT then
        if #node.children == 1 and node.children[1].type == CSON_INLINE then
            return { { type = CSON_STRING, key = node.key,
                       value = run_inline(node.children[1].value, env), children = {} } }
        end
        local o = { type = CSON_OBJECT, key = node.key, value = node.value, children = {} }
        for _, child in ipairs(node.children) do
            for _, c in ipairs(eval_node(child, env)) do o.children[#o.children + 1] = c end
        end
        return { o }
    end
    return { { type = CSON_STRING, key = node.key, value = node.value, children = {} } }
end

function son.parse(content)
    return parse_string(content, {})
end

function son.parse_file(path)
    local f = io.open(path, "rb")
    if not f then return nil, false end
    local content = f:read("*a")
    f:close()
    return parse_string(content, {}), true
end

function son.eval(tree, env)
    if not tree then return nil end
    return eval_node(tree, env)[1]
end

function son.eval_file(path, env)
    local tree, ok = son.parse_file(path)
    if not ok then return nil, false end
    return son.eval(tree, env), true
end

function son.find(obj, key)
    if not obj then return nil end
    for _, c in ipairs(obj.children or {}) do
        if c.key == key then return c end
    end
    return nil
end

function son.get(obj, key)
    local c = son.find(obj, key)
    if c and c.type ~= CSON_OBJECT then return c.value end
    return nil
end

function son.dump(o, d)
    d = d or 0
    if not o then return end
    for _, c in ipairs(o.children or {}) do
        io.write(("  "):rep(d))
        if c.type == CSON_IF then
            io.write(('IF "%s" {\n'):format(c.value or "?"))
            son.dump(c.children[1], d + 1)
            io.write(("  "):rep(d) .. "}\n")
        elseif c.type == CSON_INLINE then
            io.write(('!!"__lua__": "%s"\n'):format(c.value or "?"))
        else
            if c.key then io.write(c.key .. ": ") end
            if c.type == CSON_OBJECT and c.children and #c.children > 0 then
                io.write("{\n")
                son.dump(c, d + 1)
                io.write(("  "):rep(d) .. "}\n")
            else
                io.write(('"%s"\n'):format(c.value or "?"))
            end
        end
    end
end

return son
