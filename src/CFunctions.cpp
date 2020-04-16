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

#include "CFunctions.h"
#include "extra/CLuaArguments.h"
#include "extra/CScriptArgReader.h"

int CFunctions::CreateRedisClient(lua_State* luaVM)
{
  if (luaVM)
  {
    std::string strIp;
    int iPort;
    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strIp);
    argStream.ReadNumber(iPort);
    if (!argStream.HasErrors()) {
      redisContext *c = redisConnect(strIp.c_str(), iPort);
      if (c == NULL || c->err) {
        if (c) {
          lua_pushnil(luaVM);
          lua_pushinteger(luaVM, c->err);
          lua_pushstring(luaVM, c->errstr);
          return 3;
        } else {
          lua_pushnil(luaVM);
          lua_pushinteger(luaVM, 1);
          lua_pushstring(luaVM, "Can't allocate redis context");
          return 3;
        }
      }
      else {
        lua_pushlightuserdata(luaVM, c);
        return 1;
      }
    }
  }
  lua_pushboolean(luaVM, 0);
  return 0;
}

int CFunctions::RedisClientPing(lua_State* luaVM)
{
    if (luaVM)
    {
        redisContext* c = NULL;
        CScriptArgReader argStream(luaVM);
        argStream.ReadUserData(c);

        // PING server
        redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(c,"PING"));
        printf("PING: %s\n", reply->str);
        lua_pushstring(luaVM, reply->str);
        freeReplyObject(reply);
        return 1;
    }
    lua_pushboolean(luaVM, 0);
    return 0;
}

int CFunctions::RedisClientCommand(lua_State* luaVM)
{
    if (luaVM)
    {
        redisContext* c = NULL;
        std::string strCommand;
        CScriptArgReader argStream(luaVM);
        argStream.ReadUserData(c);
        argStream.ReadString(strCommand);
        printf("COMMAND = %s\n", strCommand.c_str());
        redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(c, "%s", strCommand.c_str()));
        lua_pushstring(luaVM, reply->str);
        freeReplyObject(reply);
        return 1;
    }
    lua_pushboolean(luaVM, 0);
    return 0;
}

int CFunctions::RedisClientSet(lua_State* luaVM)
{
  if (luaVM)
  {
    redisContext* c = NULL;
    std::string strKey;
    std::string strValue;
    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(c);
    argStream.ReadString(strKey);
    argStream.ReadString(strValue);

    if (!argStream.HasErrors())
    {
      redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(c, "SET %s %s", strKey.c_str(), strValue.c_str()));
      lua_pushboolean(luaVM, 1);
      freeReplyObject(reply);
      return 1;
    }
  }
  lua_pushboolean(luaVM, 0);
  return 0;
}

int CFunctions::PopulateTableWithReply(lua_State* luaVM, redisReply* reply)
{
  for(int i=0; i < reply->elements; i++) {
    redisReply* element = reply->element[i];
    switch (element->type)
    {
    case REDIS_REPLY_STRING:
      lua_pushinteger(luaVM, i);
      lua_pushstring(luaVM, reply->str);
      lua_settable(luaVM, -3);
      break;
    case REDIS_REPLY_ARRAY:
      lua_pushinteger(luaVM, i);
      lua_pushnil(luaVM);
      lua_settable(luaVM, -3);
      break;
    case REDIS_REPLY_INTEGER:
      lua_pushinteger(luaVM, i);
      lua_pushinteger(luaVM, reply->integer);
      lua_settable(luaVM, -3);
      break;
    case REDIS_REPLY_NIL:
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_ERROR:
    default:
      lua_pushinteger(luaVM, i);
      lua_pushnil(luaVM);
      lua_settable(luaVM, -3);
      break;
    }
  }
}

int CFunctions::RedisClientGet(lua_State* luaVM)
{
  if (luaVM)
  {
    redisContext* c = NULL;
    std::string strKey;
    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(c);
    argStream.ReadString(strKey);

    if (!argStream.HasErrors())
    {
      redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(c, "GET %s", strKey.c_str()));
      switch (reply->type)
      {
      case REDIS_REPLY_STRING:
        lua_pushstring(luaVM, reply->str);
        freeReplyObject(reply);
        return 1;
      case REDIS_REPLY_ARRAY:
        lua_newtable(luaVM);
        PopulateTableWithReply(luaVM, reply);
        lua_pushinteger(luaVM, reply->elements);
        freeReplyObject(reply);
        return 2;
      case REDIS_REPLY_INTEGER:
        lua_pushinteger(luaVM, reply->integer);
        freeReplyObject(reply);
        return 1;
      case REDIS_REPLY_NIL:
        lua_pushnil(luaVM);
        return 1;
      case REDIS_REPLY_STATUS:
      case REDIS_REPLY_ERROR:
      default:
        break;
      }
    }
  }
  return 0;
}

int CFunctions::RedisClientDestroy(lua_State* luaVM) {
  if (luaVM)
  {
    redisContext* c = NULL;
    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(c);
    if (!argStream.HasErrors())
    {
      redisFree(c);
    }
  }
  return 0;
}
