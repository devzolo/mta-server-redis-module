/*********************************************************
 *
 *  Multi Theft Auto: San Andreas - Deathmatch
 *
 *  ml_base, External lua add-on module
 *
 *  Copyright © 2003-2018 MTA.  All Rights Reserved.
 *
 *  Grand Theft Auto is © 2002-2018 Rockstar North
 *
 *  THE FOLLOWING SOURCES ARE PART OF THE MULTI THEFT
 *  AUTO SOFTWARE DEVELOPMENT KIT AND ARE RELEASED AS
 *  OPEN SOURCE FILES. THESE FILES MAY BE USED AS LONG
 *  AS THE DEVELOPER AGREES TO THE LICENSE THAT IS
 *  PROVIDED WITH THIS PACKAGE.
 *
 *********************************************************/

class CFunctions;

#pragma once

#include <stdio.h>

#include "include/ILuaModuleManager.h"
#include "hiredis.h"

extern ILuaModuleManager10* pModuleManager;

class CFunctions
{
private:
    static int PopulateTableWithReply(lua_State* luaVM, redisReply* reply);
public:
    static int CreateRedisClient(lua_State* luaVM);
    static int RedisClientPing(lua_State* luaVM);
    static int RedisClientCommand(lua_State* luaVM);
    static int RedisClientSet(lua_State* luaVM);
    static int RedisClientGet(lua_State* luaVM);
    static int RedisClientDestroy(lua_State* luaVM);
};
