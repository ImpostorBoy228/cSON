local script_dir = arg[0]:match("^(.*)[/\\]") or "."
package.path = script_dir .. "/../src/?.lua;" .. package.path
local son = require("son")

local checks, fails = 0, 0

local function CHECK(cond, msg)
    checks = checks + 1
    if not cond then
        fails = fails + 1
        io.stderr:write("FAIL: " .. tostring(msg) .. "\n")
    end
end

local dir
do
    local base = os.getenv("TMPDIR") or "/tmp"
    dir = base .. "/cson_lua_test_" .. tostring(os.time()) .. "_" .. tostring(math.random(1e8))
    os.execute("mkdir -p '" .. dir .. "'")
end

local function write_fixture(name, content)
    local path = dir .. "/" .. name
    local f = assert(io.open(path, "wb"))
    f:write(content)
    f:close()
    return path
end

local function parse_ok(name, content)
    local root = son.parse(content)
    CHECK(root ~= nil, "parse_ok " .. name)
    return root
end

local function test_basic()
    local root = parse_ok("basic.son", [[
{
    "a": "bubububu"
    "b": "hello"
    "c": {
        "d": "e"
    }
}
]])
    local v = son.get(root, "a")
    CHECK(v == "bubububu", "basic a")
    CHECK(son.get(root, "b") == "hello", "basic b")
    local c = son.find(root, "c")
    CHECK(c and c.type == son.CSON_OBJECT, "basic c type")
    CHECK(c and son.get(c, "d") == "e", "basic c.d")
    CHECK(son.get(root, "nope") == nil, "basic missing")
end

local function test_if()
    local root = parse_ok("if.son", [[
{
    "a": {
        !"if(owo == "dick")": {
            "text": "Hello"
        }
    }
}
]])
    local a = son.find(root, "a")
    CHECK(a and a.type == son.CSON_OBJECT, "if a type")
    local ifn = a and a.children[1]
    CHECK(ifn and ifn.type == son.CSON_IF, "if node type")
    CHECK(ifn and ifn.value == 'if(owo == "dick")', "if value")
    CHECK(son.get(ifn, "text") == nil, "if get nil")
    CHECK(ifn and ifn.children[1] and son.get(ifn.children[1], "text") == "Hello", "if body")
end

local function test_escapes()
    local root = parse_ok("escapes.son", [[
{
    "q": "say \"hi\""
    "s": "a\\b"
}
]])
    CHECK(son.get(root, "q") == 'say "hi"', "escapes q")
    CHECK(son.get(root, "s") == "a\\b", "escapes s")
end

local function test_comments()
    local root = parse_ok("comments.son", [[
{
; comment
    "k": "v" ; trailing
}
]])
    CHECK(son.get(root, "k") == "v", "comments k")
end

local function test_orphan()
    local root = parse_ok("orphan.son", [[
{
    "k": "v"
    "orphan"
}
]])
    CHECK(son.get(root, "k") == "v", "orphan k")
end

local function test_macro()
    local root = parse_ok("macro.son", "#define VAR dick\n{\n    \"cock\": \"VAR\"\n}\n")
    CHECK(son.get(root, "cock") == "dick", "macro value")
end

local function test_macro_if()
    local root = parse_ok("macro_if.son", [[
#define VAR dick
{
    "a": {
        !"if(owo == "VAR")": {
            "text": "Hello owo!!"
        }
    }
}
]])
    local a = son.find(root, "a")
    local ifn = a and a.children[1]
    CHECK(ifn and ifn.type == son.CSON_IF, "macro_if type")
    CHECK(ifn and ifn.value == 'if(owo == "dick")', "macro_if resolved")
end

local function test_inline_lua()
    local root = parse_ok("inline_lua.son", [[
{
    "res": {
        !!"__lua__": "return '998'"
    }
}
]])
    local res = son.find(root, "res")
    CHECK(res and res.type == son.CSON_OBJECT, "inline res object")
    local inl = res and res.children[1]
    CHECK(inl and inl.type == son.CSON_INLINE, "inline node type")
    CHECK(inl and inl.key == nil, "inline key nil")
    CHECK(inl and inl.value == "return '998'", "inline value")
end

local function test_inline_aliases()
    local root = parse_ok("inline_aliases.son", [[
{
    "c": { !!"__c__": "return '1'" }
    "asm": { !!"__asm__": "return '2'" }
}
]])
    local c = son.find(root, "c")
    local a = son.find(root, "asm")
    CHECK(c and c.children[1].type == son.CSON_INLINE, "alias __c__")
    CHECK(a and a.children[1].type == son.CSON_INLINE, "alias __asm__")
end

local function test_brace_variants()
    local t1 = parse_ok("same_line.son", '{"a":{"b":"v"}}')
    local a1 = son.find(t1, "a")
    CHECK(son.get(a1, "b") == "v", "brace same line")

    local t2 = parse_ok("empty.son", '{"a":{}}')
    local a2 = son.find(t2, "a")
    CHECK(a2 and a2.type == son.CSON_OBJECT and #a2.children == 0, "brace empty")

    local t3 = parse_ok("close_nl.son", '{ "a": { "b": "v" }\n}')
    CHECK(son.get(son.find(t3, "a"), "b") == "v", "brace closing newline")

    local t4 = parse_ok("blank.son", '{ "a": { "b": "v" }\n\n\n}\n\n\n')
    CHECK(son.get(son.find(t4, "a"), "b") == "v", "brace blank lines")

    local t5 = parse_ok("sib.son", '{"a":{"b":{"c":"v"}} "x":"y"}')
    local b = son.find(son.find(t5, "a"), "b")
    CHECK(son.get(b, "c") == "v", "brace adjacent b.c")
    CHECK(son.get(t5, "x") == "y", "brace adjacent x")
end

local function test_crlf()
    local root = parse_ok("crlf.son", "{\r\n    \"a\": \"bubububu\"\r\n    \"c\": {\r\n        \"d\": \"e\"\r\n    }\r\n}\r\n")
    CHECK(son.get(root, "a") == "bubububu", "crlf a")
    CHECK(son.get(son.find(root, "c"), "d") == "e", "crlf c.d")
end

local function test_eval_if_unfold()
    local root = parse_ok("apply1.son", [[
{
    "a": {
        !"if(x == 1)": {
            "b": "v"
        }
    }
}
]])
    local ev = son.eval(root, { x = 1 })
    local a = son.find(ev, "a")
    CHECK(a ~= nil, "unfold a exists")
    local has_if = false
    if a then for _, c in ipairs(a.children) do if c.type == son.CSON_IF then has_if = true end end end
    CHECK(not has_if, "unfold no if left")
    CHECK(son.get(a, "b") == "v", "unfold b")
end

local function test_eval_if_delete()
    local root = parse_ok("apply2.son", [[
{
    "a": {
        !"if(x == 1)": {
            "b": "v"
        }
    }
}
]])
    local ev = son.eval(root, { x = 0 })
    local a = son.find(ev, "a")
    CHECK(a ~= nil, "delete a exists")
    CHECK(a and #a.children == 0, "delete a empty")
    CHECK(son.get(a, "b") == nil, "delete b nil")
end

local function test_if_siblings()
    local root = parse_ok("if_siblings.son", [[
{
    "a": {
        !"if(x == 1)": { "b": "v" }
        !"if(y == 2)": { "c": "w" }
    }
}
]])
    local a = son.find(root, "a")
    local ifc = 0
    if a then for _, c in ipairs(a.children) do if c.type == son.CSON_IF then ifc = ifc + 1 end end end
    CHECK(ifc == 2, "siblings count")
end

local function test_if_siblings_eval()
    local root = parse_ok("if_sib_eval.son", [[
{
    "a": {
        !"if(x == 1)": { "b": "v" }
        !"if(y == 2)": { "c": "w" }
    }
}
]])
    local ev = son.eval(root, { x = 1, y = 2 })
    local a = son.find(ev, "a")
    CHECK(son.get(a, "b") == "v", "siblings b")
    CHECK(son.get(a, "c") == "w", "siblings c")
end

local function test_eval_inline_collapse()
    local root = parse_ok("inline_collapse.son", [[
{
    "res": {
        !!"__lua__": "return '998'"
    }
}
]])
    local ev = son.eval(root)
    CHECK(son.get(ev, "res") == "998", "collapse res")
    CHECK(son.find(ev, "res").type == son.CSON_STRING, "collapse res type")
end

local function test_eval_inline_no_collapse()
    local root = parse_ok("inline_no_collapse.son", [[
{
    "res": {
        !!"__lua__": "return '998'"
        "x": "y"
    }
}
]])
    local ev = son.eval(root)
    local res = son.find(ev, "res")
    CHECK(res and res.type == son.CSON_OBJECT, "no-collapse res object")
    CHECK(son.get(ev, "res") == nil, "no-collapse get nil")
    local inl = res and res.children[1]
    CHECK(inl and inl.value == "998", "no-collapse inline value")
end

local function test_eval_nested_if()
    local root = parse_ok("nested.son", [[
{
    "a": {
        !"if(1)": {
            !"if(2)": { "deep": "yes" }
        }
    }
}
]])
    local ev = son.eval(root)
    CHECK(son.get(son.find(ev, "a"), "deep") == "yes", "nested if")
end

local function test_eval_inline_env()
    local root = parse_ok("inline_env.son", '{ "v": { !!"__lua__": "return n * 2" } }\n')
    local ev = son.eval(root, { n = 21 })
    CHECK(son.get(ev, "v") == "42", "inline env")
end

local function test_eval_bad_inline()
    local root = son.parse('{ "v": { !!"__lua__": "this is not lua" } }')
    local ok = pcall(function() son.eval(root) end)
    CHECK(not ok, "bad inline raises")
end

local function test_eval_bad_if()
    local root = son.parse('{ "a": { !"if(()": { "x": "y" } } }')
    local ok = pcall(function() son.eval(root) end)
    CHECK(not ok, "bad if raises")
end

local function test_eval_file_integration()
    local path = write_fixture("e2e.son", [[
#define VAR dick
{
    "important code result": { !!"__lua__": "return '998'" }
    "assembler result": { !!"__lua__": "return 42" }
    "a": {
        !"if(owo == "VAR")": { "text": "Hello owo!!" }
        !"if(1)": { "poop.txt": "microsoft" }
    }
}
]])
    local ev, ok = son.eval_file(path, { owo = "dick" })
    CHECK(ok and ev ~= nil, "e2e ok")
    CHECK(son.get(ev, "important code result") == "998", "e2e 998")
    CHECK(son.get(ev, "assembler result") == "42", "e2e 42")
    local a = son.find(ev, "a")
    CHECK(son.get(a, "text") == "Hello owo!!", "e2e text")
    CHECK(son.get(a, "poop.txt") == "microsoft", "e2e poop")
end

local function test_empty()
    CHECK(son.parse("") == nil, "empty parse")
    local path = write_fixture("empty.son", "")
    local root, ok = son.parse_file(path)
    CHECK(ok and root == nil, "empty file ok")
end

local function test_missing()
    local root, ok = son.parse_file("/no/such/file/x.son")
    CHECK(not ok and root == nil, "missing file")
end

test_basic()
test_if()
test_escapes()
test_comments()
test_orphan()
test_macro()
test_macro_if()
test_inline_lua()
test_inline_aliases()
test_brace_variants()
test_crlf()
test_eval_if_unfold()
test_eval_if_delete()
test_if_siblings()
test_if_siblings_eval()
test_eval_inline_collapse()
test_eval_inline_no_collapse()
test_eval_nested_if()
test_eval_inline_env()
test_eval_bad_inline()
test_eval_bad_if()
test_eval_file_integration()
test_empty()
test_missing()

os.execute("rm -rf '" .. dir .. "'")

print(string.format("%d checks, %d failures", checks, fails))
os.exit(fails == 0 and 0 or 1)
