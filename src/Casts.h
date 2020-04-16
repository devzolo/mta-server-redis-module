#pragma once

#include <string>
#include "hiredis.h"

// class -> class name
inline std::string GetClassTypeName(redisContext*)
{
    return "redis-context";
}

//
// T from userdata
//
template <class T>
T* UserDataCast(T*, void* ptr, lua_State*)
{
    return reinterpret_cast<T*>(ptr);
}
