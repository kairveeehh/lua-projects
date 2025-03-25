local db_lib = require("luasql_prepared")
local db = db_lib.open("test.db")

local stmt = db:prepare("CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, name TEXT)")
stmt:step()
stmt:finalize()

local insert_stmt = db:prepare("INSERT INTO test (name) VALUES ('LuaUser')")
insert_stmt:step()
insert_stmt:finalize()

db:close()

print("Database operations completed successfully!")
