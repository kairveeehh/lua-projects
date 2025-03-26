#include <lua.h>
#include <lauxlib.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PARAMS 10  // Adjust as needed

typedef struct {
    MYSQL *conn;
} MySQLConn;

typedef struct {
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[MAX_PARAMS];
    int param_count;
    void *buffers[MAX_PARAMS];  // Track allocated memory for cleanup
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
    memset(stmt->bind, 0, sizeof(stmt->bind));  // Ensure clean memory

    if (mysql_stmt_prepare(stmt->stmt, query, strlen(query)) != 0) {
        return luaL_error(L, "Prepare failed: %s", mysql_stmt_error(stmt->stmt));
    }

    stmt->param_count = mysql_stmt_param_count(stmt->stmt);
    if (stmt->param_count > MAX_PARAMS) {
        return luaL_error(L, "Too many parameters: Max %d supported", MAX_PARAMS);
    }

    memset(stmt->buffers, 0, sizeof(stmt->buffers));  // Track memory allocations

    luaL_getmetatable(L, "MySQLStmt");
    lua_setmetatable(L, -2);
    return 1;
}

// Bind parameters (supports only integers for now)
static int l_bind(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");

    int num_params = stmt->param_count;
    if (lua_gettop(L) - 1 != num_params) {
        return luaL_error(L, "Expected %d parameters, got %d", num_params, lua_gettop(L) - 1);
    }

    // Clear previous memory
    for (int i = 0; i < num_params; i++) {
        if (stmt->buffers[i]) {
            free(stmt->buffers[i]);
            stmt->buffers[i] = NULL;
        }
    }

    for (int i = 0; i < num_params; i++) {
        if (!lua_isinteger(L, i + 2)) {  // Ensure all are integers (extend for other types)
            return luaL_error(L, "Parameter %d must be an integer", i + 1);
        }
        int *val = malloc(sizeof(int));  // Allocate memory for parameter
        *val = lua_tointeger(L, i + 2);

        stmt->bind[i].buffer_type = MYSQL_TYPE_LONG;
        stmt->bind[i].buffer = val;
        stmt->bind[i].is_null = 0;
        stmt->bind[i].length = 0;

        stmt->buffers[i] = val;  // Store pointer for cleanup
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

// Fetch results
static int l_fetch(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");
    MYSQL_RES *meta = mysql_stmt_result_metadata(stmt->stmt);
    if (!meta) {
        lua_pushnil(L);
        return 1;
    }

    int num_fields = mysql_num_fields(meta);
    MYSQL_BIND result_bind[num_fields];
    memset(result_bind, 0, sizeof(result_bind));

    int result[num_fields];

    for (int i = 0; i < num_fields; i++) {
        result_bind[i].buffer_type = MYSQL_TYPE_LONG;
        result_bind[i].buffer = &result[i];
        result_bind[i].is_null = 0;
        result_bind[i].length = 0;
    }

    mysql_stmt_bind_result(stmt->stmt, result_bind);
    if (mysql_stmt_fetch(stmt->stmt) == 0) {
        lua_pushinteger(L, result[0]); // Returning first column
    } else {
        lua_pushnil(L);
    }

    mysql_free_result(meta);
    return 1;
}

// Cleanup function
static int l_stmt_gc(lua_State *L) {
    MySQLStmt *stmt = (MySQLStmt *)luaL_checkudata(L, 1, "MySQLStmt");

    for (int i = 0; i < stmt->param_count; i++) {
        if (stmt->buffers[i]) {
            free(stmt->buffers[i]);
            stmt->buffers[i] = NULL;
        }
    }

    if (stmt->stmt) {
        mysql_stmt_close(stmt->stmt);
        stmt->stmt = NULL;
    }

    return 0;
}

// Register functions
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

    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, l_stmt_gc);
    lua_settable(L, -3);

    luaL_newlib(L, mysqlprep);
    return 1;
}
