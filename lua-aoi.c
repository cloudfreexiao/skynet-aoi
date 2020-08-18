#define LUA_LIB

#include "aoi/aoi.h"
#include "skynet.h"

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>

// https://blog.codingnow.com/2012/03/dev_note_13.html
// https://blog.codingnow.com/2008/07/aoi.html

struct laoi_cookie
{
    int count;
    int max;
    int current;
};

struct laoi_space
{
    struct lua_State *L;
    struct aoi_space *space;
    struct laoi_cookie cookie;
};

static void
message(void *ud, uint32_t watcher, uint32_t marker)
{
    // printf("%u => %u\n", watcher, marker);
    struct laoi_space *aux = (struct laoi_space *)(ud);
    struct lua_State *L = aux->L;
    if (L == NULL)
    {
        printf("%s\n", "L is empty.");
        return;
    }
    lua_geti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_getfield(L, -1, "aoi_callback"); // function
    lua_pushinteger(L, watcher);
    lua_pushinteger(L, marker);
    lua_pcall(L, 2, 0, 0);
    return;
}

static void *
my_alloc(void *ud, void *ptr, size_t sz)
{
    struct laoi_space *aux = ud;
    struct laoi_cookie *cookie = &aux->cookie;
    if (ptr == NULL)
    {
        void *p = skynet_malloc(sz);
        ++cookie->count;
        cookie->current += sz;
        if (cookie->max < cookie->current)
        {
            cookie->max = cookie->current;
        }

        return p;
    }
    --cookie->count;
    cookie->current -= sz;

    skynet_free(ptr);
    return NULL;
}

static int
lnew(lua_State *L)
{
    struct laoi_space *aux = (struct laoi_space *)lua_newuserdata(L, sizeof(*aux));
    if (aux == NULL)
    {
        luaL_error(L, "malloc failture.\n");
        return 0;
    }
    else
    {
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_setmetatable(L, -2);

        aux->L = L;
        lua_geti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
        lua_newtable(L);
        lua_rawsetp(L, -2, aux);
        lua_pop(L, 1);

        struct laoi_cookie cookie = {0, 0, 0};
        aux->cookie = cookie;
        struct aoi_space *space = aoi_create(my_alloc, aux);
        aux->space = space;

        return 1;
    }
}

static int
lrelease(lua_State *L)
{
    if (lua_gettop(L) >= 1)
    {
        struct laoi_space *aux = (struct laoi_space *)lua_touserdata(L, 1);
        struct aoi_space *space = aux->space;
        aoi_release(space);
        return 0;
    }
    else
    {
        luaL_error(L, "must be.");
        return 0;
    }
}

static int
lupdate(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    struct laoi_space *aux = (struct laoi_space *)lua_touserdata(L, 1);
    struct aoi_space *space = aux->space;
    lua_Integer id = luaL_checkinteger(L, 2);
    const char *m = luaL_checkstring(L, 3);
    lua_Number x = luaL_checknumber(L, 4);
    lua_Number y = luaL_checknumber(L, 5);
    lua_Number z = luaL_checknumber(L, 6);
    float pos[3] = {x, y, z};
    aoi_update(space, id, m, pos);
    return 0;
}

static int
lmessage(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    struct laoi_space *aux = (struct laoi_space *)lua_touserdata(L, 1);
    aux->L = L;
    struct aoi_space *space = aux->space;

    lua_geti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, "aoi_callback");
    aoi_message(space, message, aux);
    return 0;
}

static int
ldump(lua_State *L)
{
    struct laoi_space *aux = (struct laoi_space *)lua_touserdata(L, 1);
    printf("max memory = %d, current memory = %d\n", aux->cookie.max, aux->cookie.current);
    return 0;
}

LUAMOD_API int
luaopen_aoi_core(lua_State *L)
{
    luaL_checkversion(L);
    lua_newtable(L); // met
    luaL_Reg l[] = {
        {"update", lupdate},
        {"message", lmessage},
        {"dump", ldump},
        {NULL, NULL},
    };
    luaL_newlib(L, l);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, lrelease, 0);
    lua_setfield(L, -2, "__gc");
    lua_pushcclosure(L, lnew, 1);
    return 1;
}
