local script_dir = arg[0]:match("^(.*)[/\\]") or "."
package.path = script_dir .. "/?.lua;" .. package.path
local son = require("son")

local tree = son.parse_file(script_dir .. "/../emmm~lua~.son")
son.dump(tree, 0)
print()

local owo = "dick"
local ev = son.eval(tree, { owo = owo }) -- lua was mistake
local emm = son.find(ev, "emm")
print(son.get(emm, "f1"))                     -- lol
print(son.get(emm, "important code result"))  -- 998
print(son.get(emm, "assembler result"))       -- 42
print(son.get(son.find(ev, "a"), "text"))     -- Hello owo!!

son.dump(ev, 0)
