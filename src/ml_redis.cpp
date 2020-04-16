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

#include "ml_redis.h"

ILuaModuleManager10* pModuleManager = NULL;

// Initialisation function (module entrypoint)
MTAEXPORT bool InitModule(ILuaModuleManager10* pManager, char* szModuleName, char* szAuthor, float* fVersion)
{
    pModuleManager = pManager;

    // Set the module info
    strncpy(szModuleName, MODULE_NAME, MAX_INFO_LENGTH);
    strncpy(szAuthor, MODULE_AUTHOR, MAX_INFO_LENGTH);
    (*fVersion) = MODULE_VERSION;

    return true;
}

MTAEXPORT void RegisterFunctions(lua_State* luaVM)
{
    if (pModuleManager && luaVM)
    {
      std::map<const char*, lua_CFunction> functions{
        {"createRedisClient", CFunctions::CreateRedisClient},
        {"redisClientPing", CFunctions::RedisClientPing},
        {"redisClientCommand", CFunctions::RedisClientCommand},
        {"redisClientSet", CFunctions::RedisClientSet},
        {"redisClientGet", CFunctions::RedisClientGet},
        {"redisClientDestroy", CFunctions::RedisClientDestroy},

      };

      // Add functions
      for (const auto& pair : functions) {
          pModuleManager->RegisterFunction(luaVM, pair.first, pair.second);
      }
    }
}

MTAEXPORT bool DoPulse(void)
{
    return true;
}

MTAEXPORT bool ShutdownModule(void)
{
    return true;
}

MTAEXPORT bool ResourceStopping(lua_State* luaVM)
{
    return true;
}

MTAEXPORT bool ResourceStopped(lua_State* luaVM)
{
    return true;
}
