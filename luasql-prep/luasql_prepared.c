#include <lua.h>
#include <lauxlib.h>
#include <sqlite3.h>
#include <stdio.h>

// Lua binding for prepared statement
typedef struct {
    sqlite3_stmt *stmt;
} lua_stmt;

// Prepare function: db:prepare(sql)
static int l_prepare(lua_State *L) {
    sqlite3 *db = *(sqlite3 **)luaL_checkudata(L, 1, "LuaSQL.db");
    const char *sql = luaL_checkstring(L, 2);
    
    lua_stmt *stmt = (lua_stmt *)lua_newuserdata(L, sizeof(lua_stmt));
    luaL_getmetatable(L, "LuaSQL.stmt");
    lua_setmetatable(L, -2);

    if (sqlite3_prepare_v2(db, sql, -1, &stmt->stmt, NULL) != SQLITE_OK) {
        return luaL_error(L, "Failed to prepare: %s", sqlite3_errmsg(db));
    }

    return 1; // returns stmt userdata
}

// stmt:step()
static int l_stmt_step(lua_State *L) {
    lua_stmt *stmt = (lua_stmt *)luaL_checkudata(L, 1, "LuaSQL.stmt");
    int rc = sqlite3_step(stmt->stmt);
    lua_pushboolean(L, rc == SQLITE_ROW || rc == SQLITE_DONE);
    return 1;
}

// stmt:finalize()
static int l_stmt_finalize(lua_State *L) {
    lua_stmt *stmt = (lua_stmt *)luaL_checkudata(L, 1, "LuaSQL.stmt");
    sqlite3_finalize(stmt->stmt);
    return 0;
}

// db:close()
static int l_db_close(lua_State *L) {
    sqlite3 *db = *(sqlite3 **)luaL_checkudata(L, 1, "LuaSQL.db");
    sqlite3_close(db);
    return 0;
}

// LuaSQL.open(filename)
static int l_db_open(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    sqlite3 **db = (sqlite3 **)lua_newuserdata(L, sizeof(sqlite3 *));
    luaL_getmetatable(L, "LuaSQL.db");
    lua_setmetatable(L, -2);

    if (sqlite3_open(filename, db) != SQLITE_OK) {
        return luaL_error(L, "Cannot open database: %s", sqlite3_errmsg(*db));
    }

    return 1; // returns db userdata
}

// Register functions
int luaopen_luasql_prepared(lua_State *L) {
    // Create metatable for database userdata
    luaL_newmetatable(L, "LuaSQL.db");

    // Set __index = metatable itself to enable method lookup
    lua_pushvalue(L, -1);          // duplicate metatable
    lua_setfield(L, -2, "__index"); // metatable.__index = metatable

    lua_pushcfunction(L, l_prepare);
    lua_setfield(L, -2, "prepare");
    lua_pushcfunction(L, l_db_close);
    lua_setfield(L, -2, "close");
    lua_pop(L, 1);

    // Metatable for statement
    luaL_newmetatable(L, "LuaSQL.stmt");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, l_stmt_step);
    lua_setfield(L, -2, "step");
    lua_pushcfunction(L, l_stmt_finalize);
    lua_setfield(L, -2, "finalize");
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushcfunction(L, l_db_open);
    lua_setfield(L, -2, "open");

    return 1;
}

