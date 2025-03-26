#include <lua.h>
#include <lauxlib.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    MYSQL *conn;
} MySQLConn;

typedef struct {
    MYSQL_STMT *stmt;
    MYSQL_BIND *bind;
    int param_count;
} MySQLStmt;

// Connect function
static int l_connect(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    const char *user = luaL_checkstring(L, 2);
    const char *pass = luaL_checkstring(L, 3);
    const char *db   = luaL_checkstring(L, 4);

    MySQLConn *conn = (MySQLConn *)lua_newuserdata(L, sizeof(MySQLConn));
    conn->conn = mysql_init(NULL);
    if (!mysql_real_connect(conn->conn, host, user, pass, db, 0, NULL, 0)) {
        return luaL_error(L, "Connection failed: %s", mysql_error(conn->conn));
    }

    luaL_getmetatable(L, "MySQLConn");
    lua_setmetatable(L, -2);
    return 1;
}

// Prepare statement
static int l_prepare(lua_State *L) {
    MySQLConn *conn = (MySQLConn *)luaL_checkudata(L, 1, "MySQLConn");
    const char *query = luaL_checkstring(L, 2);

    MySQLStmt *stmt = (MySQLStmt *)lua_newuserdata(L, sizeof(MySQLStmt));
    stmt->stmt = mysql_stmt_init(conn->conn);
    if (!stmt->stmt) {
        return luaL_error(L, "Failed to initialize statement");
    }

    if (mysql_stmt_prepare(stmt->stmt, query, strlen(query)) != 0) {
        return luaL_error(L, "Prepare failed: %s", mysql_stmt_error(stmt->stmt));
    }

    stmt->param_count = mysql_stmt_param_count(stmt->stmt);
    stmt->bind = calloc(stmt->param_count, sizeof(MYSQL_BIND));

    luaL_getmetatable(L, "MySQLStmt");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_bind(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");

    int num_params = stmt->param_count;
    int provided_params = lua_gettop(L) - 1;

    if (provided_params != num_params) {
        return luaL_error(L, "Expected %d parameters, got %d", num_params, provided_params);
    }

    for (int i = 0; i < num_params; i++) {
        if (!lua_isinteger(L, i + 2)) {
            return luaL_error(L, "Parameter %d must be an integer", i + 1);
        }

        int val = lua_tointeger(L, i + 2);
        stmt->bind[i].buffer_type = MYSQL_TYPE_LONG;
        stmt->bind[i].buffer = malloc(sizeof(int));
        memcpy(stmt->bind[i].buffer, &val, sizeof(int));
    }

    if (mysql_stmt_bind_param(stmt->stmt, stmt->bind) != 0) {
        return luaL_error(L, "Binding failed: %s", mysql_stmt_error(stmt->stmt));
    }

    return 0;
}

// Execute statement
static int l_execute(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");

    if (mysql_stmt_execute(stmt->stmt) != 0) {
        return luaL_error(L, "Execute failed: %s", mysql_stmt_error(stmt->stmt));
    }

    lua_pushboolean(L, 1);
    return 1;
}

// Fetch results (integer columns only)
static int l_fetch(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");

    MYSQL_RES *meta_result = mysql_stmt_result_metadata(stmt->stmt);
    if (!meta_result) {
        lua_pushnil(L);
        return 1;
    }

    int num_fields = mysql_num_fields(meta_result);
    MYSQL_BIND *result_bind = calloc(num_fields, sizeof(MYSQL_BIND));
    int *results = calloc(num_fields, sizeof(int));

    for (int i = 0; i < num_fields; i++) {
        result_bind[i].buffer_type = MYSQL_TYPE_LONG;
        result_bind[i].buffer = &results[i];
    }

    mysql_stmt_bind_result(stmt->stmt, result_bind);
    if (mysql_stmt_fetch(stmt->stmt) == 0) {
        lua_newtable(L);
        for (int i = 0; i < num_fields; i++) {
            lua_pushinteger(L, results[i]);
            lua_rawseti(L, -2, i + 1);
        }
    } else {
        lua_pushnil(L);
    }

    free(result_bind);
    free(results);
    mysql_free_result(meta_result);
    return 1;
}

// Cleanup function
static int l_close(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");

    if (stmt->stmt) {
        mysql_stmt_close(stmt->stmt);
    }

    for (int i = 0; i < stmt->param_count; i++) {
        free(stmt->bind[i].buffer);
    }

    free(stmt->bind);
    return 0;
}

// Register functions
static const luaL_Reg mysqlprep[] = {
    {"connect", l_connect},
    {"prepare", l_prepare},
    {"bind", l_bind},
    {"execute", l_execute},
    {"fetch", l_fetch},
    {"close", l_close},
    {NULL, NULL}
};

int luaopen_mysqlprep(lua_State *L) {
    luaL_newmetatable(L, "MySQLConn");
    luaL_newmetatable(L, "MySQLStmt");

    luaL_newlib(L, mysqlprep);
    return 1;
}
