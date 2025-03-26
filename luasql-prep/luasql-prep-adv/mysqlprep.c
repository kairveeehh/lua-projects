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
    MYSQL_BIND bind[10];
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

// prepare statement
static int l_prepare(lua_State *L) {
    MySQLConn *conn = (MySQLConn *)luaL_checkudata(L, 1, "MySQLConn");
    const char *query = luaL_checkstring(L, 2);

    MySQLStmt *stmt = (MySQLStmt *)lua_newuserdata(L, sizeof(MySQLStmt));
    stmt->stmt = mysql_stmt_init(conn->conn);

    if (mysql_stmt_prepare(stmt->stmt, query, strlen(query)) != 0) {
        return luaL_error(L, "Prepare failed: %s", mysql_stmt_error(stmt->stmt));
    }

    stmt->param_count = mysql_stmt_param_count(stmt->stmt);
    memset(stmt->bind, 0, sizeof(stmt->bind));

    luaL_getmetatable(L, "MySQLStmt");
    lua_setmetatable(L, -2);
    return 1;
}

// bind parameter rn for int only
static int l_bind(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");
    int index = luaL_checkinteger(L, 2) - 1;  // Lua index starts at 1
    int val = luaL_checkinteger(L, 3);

    if (index >= stmt->param_count || index < 0) {
        return luaL_error(L, "Invalid bind index");
    }

    stmt->bind[index].buffer_type = MYSQL_TYPE_LONG;
    stmt->bind[index].buffer = malloc(sizeof(int));
    memcpy(stmt->bind[index].buffer, &val, sizeof(int));

    return 0;
}

// execute statement
static int l_execute(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");

    mysql_stmt_bind_param(stmt->stmt, stmt->bind);

    if (mysql_stmt_execute(stmt->stmt) != 0) {
        return luaL_error(L, "Execute failed: %s", mysql_stmt_error(stmt->stmt));
    }

    lua_pushboolean(L, 1);
    return 1;
}

// fetch binary for now
static int l_fetch(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");
    MYSQL_RES *prepare_meta_result = mysql_stmt_result_metadata(stmt->stmt);
    if (!prepare_meta_result) {
        lua_pushstring(L, "No result");
        return 1;
    }
    MYSQL_FIELD *fields = mysql_fetch_fields(prepare_meta_result);
    MYSQL_BIND result_bind[10];
    memset(result_bind, 0, sizeof(result_bind));
    int result[10];

    for (int i = 0; i < mysql_num_fields(prepare_meta_result); i++) {
        result_bind[i].buffer_type = MYSQL_TYPE_LONG;
        result_bind[i].buffer = &result[i];
    }

    mysql_stmt_bind_result(stmt->stmt, result_bind);
    if (mysql_stmt_fetch(stmt->stmt) == 0) {
        lua_pushinteger(L, result[0]); // returning the first column
    } else {
        lua_pushnil(L);
    }

    mysql_free_result(prepare_meta_result);
    return 1;
}

// register functions
static const luaL_Reg mysqlprep[] = {
    {"connect", l_connect},
    {"prepare", l_prepare},
    {"bind", l_bind},
    {"execute", l_execute},
    {"fetch", l_fetch},
    {NULL, NULL}
};

int luaopen_mysqlprep(lua_State *L) {
    luaL_newmetatable(L, "MySQLConn");
    luaL_newmetatable(L, "MySQLStmt");
    luaL_newlib(L, mysqlprep);
    return 1;
}
