#include <lua.h>
#include <lauxlib.h>
#include <hiredis/hiredis.h>

static redisContext *c = NULL;

// Connect to Redis
static int l_connect(lua_State *L) {
    const char *hostname = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);
    c = redisConnect(hostname, port);
    if (c != NULL && c->err) {
        lua_pushnil(L);
        lua_pushstring(L, c->errstr);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Execute a Redis command
static int l_command(lua_State *L) {
    const char *cmd = luaL_checkstring(L, 1);
    redisReply *reply = redisCommand(c, cmd);

    if (reply == NULL) {
        lua_pushnil(L);
        return 1;
    }

    // Handle reply types
    if (reply->type == REDIS_REPLY_STRING || reply->type == REDIS_REPLY_STATUS) {
        lua_pushstring(L, reply->str);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        lua_pushinteger(L, reply->integer);
    } else if (reply->type == REDIS_REPLY_NIL) {
        lua_pushnil(L);
    } else {
        lua_pushnil(L);
    }

    freeReplyObject(reply);
    return 1;
}

// Append a command for pipelining
static int l_appendCommand(lua_State *L) {
    const char *cmd = luaL_checkstring(L, 1);
    if (redisAppendCommand(c, cmd) != REDIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, c->errstr);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Fetch the next reply from the pipeline
static int l_getReply(lua_State *L) {
    redisReply *reply;
    if (redisGetReply(c, (void *)&reply) != REDIS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, c->errstr);
        return 2;
    }

    if (reply == NULL) {
        lua_pushnil(L);
        return 1;
    }

    if (reply->type == REDIS_REPLY_STRING || reply->type == REDIS_REPLY_STATUS) {
        lua_pushstring(L, reply->str);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        lua_pushinteger(L, reply->integer);
    } else if (reply->type == REDIS_REPLY_NIL) {
        lua_pushnil(L);
    } else {
        lua_pushnil(L);
    }

    freeReplyObject(reply);
    return 1;
}

// Lua module definition
static const struct luaL_Reg redislib[] = {
    {"connect", l_connect},
    {"command", l_command},
    {"appendCommand", l_appendCommand},
    {"getReply", l_getReply},
    {NULL, NULL}
};

// Module entry point
int luaopen_myredis(lua_State *L) {
    luaL_newlib(L, redislib);
    return 1;
}
