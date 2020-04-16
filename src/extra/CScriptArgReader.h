#pragma once

extern "C"
{
    #include <lua.h>
}
#include "../Casts.h"
#include <limits>
#include <type_traits>
#include <cfloat>
#include <cmath>

/////////////////////////////////////////////////////////////////////////
//
// CScriptArgReader
//
//
// Attempt to simplify the reading of arguments from a script call
//
//////////////////////////////////////////////////////////////////////
class CScriptArgReader
{
public:

    CScriptArgReader(lua_State* luaVM)
    {
        m_luaVM = luaVM;
        m_iIndex = 1;
        m_iErrorIndex = 0;
        m_bError = false;
        //m_pPendingFunctionOutValue = NULL;
        m_pPendingFunctionIndex = -1;
        m_bResolvedErrorGotArgumentTypeAndValue = false;
        m_bHasCustomMessage = false;
    }

    ~CScriptArgReader() {
      // assert(!IsReadFunctionPending());
    }

    //
    // Read next number
    //
    template <typename T>
    void ReadNumber(T& outValue, bool checkSign = true)
    {
        int iArgument = lua_type(m_luaVM, m_iIndex);
        if (iArgument == LUA_TNUMBER || iArgument == LUA_TSTRING)
        {
            // The string received may not actually be a number
            if (!lua_isnumber(m_luaVM, m_iIndex))
            {
                SetCustomWarning("Expected number, got non-convertible string. This warning may be an error in future versions.");
            }

            // Returns 0 even if the string cannot be parsed as a number
            lua_Number number = lua_tonumber(m_luaVM, m_iIndex++);

            if (std::isnan(number))
            {
                SetCustomError("Expected number, got NaN", "Bad argument");
                outValue = 0;
                return;
            }

            if (std::is_unsigned<T>())
            {
                if (checkSign && number < -FLT_EPSILON)
                {
                    SetCustomWarning("Expected positive value, got negative. This warning may be an error in future versions.");
                }
                outValue = static_cast<T>(static_cast<int64_t>(number));
                return;
            }

            outValue = static_cast<T>(number);
            return;
        }

        outValue = 0;
        SetTypeError("number");
        m_iIndex++;
    }

    //
    // Read next number, using default if needed
    //
    template <typename T, typename U>
    void ReadNumber(T& outValue, const U& defaultValue, bool checkSign = true)
    {
        int iArgument = lua_type(m_luaVM, m_iIndex);
        if (iArgument == LUA_TNUMBER || iArgument == LUA_TSTRING)
        {
            // The string received may not actually be a number
            if (!lua_isnumber(m_luaVM, m_iIndex))
            {
                SetCustomWarning("Expected number, got non-convertible string. This warning may be an error in future versions.");
            }

            // Returns 0 even if the string cannot be parsed as a number
            lua_Number number = lua_tonumber(m_luaVM, m_iIndex++);

            if (std::isnan(number))
            {
                SetCustomError("Expected number, got NaN", "Bad argument");
                outValue = 0;
                return;
            }

            if (checkSign && std::is_unsigned<T>() && number < -FLT_EPSILON)
            {
                SetCustomWarning("Expected positive value, got negative. This warning may be an error in future versions.");
            }

            outValue = static_cast<T>(number);
            return;
        }
        else if (iArgument == LUA_TNONE || iArgument == LUA_TNIL)
        {
            outValue = static_cast<T>(defaultValue);
            m_iIndex++;
            return;
        }

        outValue = 0;
        SetTypeError("number");
        m_iIndex++;
    }

    //
    // Read next bool
    //
    void ReadBool(bool& bOutValue)
    {
        int iArgument = lua_type(m_luaVM, m_iIndex);
        if (iArgument == LUA_TBOOLEAN)
        {
            bOutValue = lua_toboolean(m_luaVM, m_iIndex++) ? true : false;
            return;
        }

        bOutValue = false;
        SetTypeError("bool");
        m_iIndex++;
    }

    //
    // Read next bool, using default if needed
    //
    void ReadBool(bool& bOutValue, const bool bDefaultValue)
    {
        int iArgument = lua_type(m_luaVM, m_iIndex);
        if (iArgument == LUA_TBOOLEAN)
        {
            bOutValue = lua_toboolean(m_luaVM, m_iIndex++) ? true : false;
            return;
        }
        else if (iArgument == LUA_TNONE || iArgument == LUA_TNIL)
        {
            bOutValue = bDefaultValue;
            m_iIndex++;
            return;
        }

        bOutValue = false;
        SetTypeError("bool");
        m_iIndex++;
    }

    //
    // Read next string, using default if needed
    //
    void ReadString(std::string& outValue, const char* defaultValue = NULL)
    {
        int iArgument = lua_type(m_luaVM, m_iIndex);

        if (iArgument == LUA_TSTRING || iArgument == LUA_TNUMBER)
        {
            size_t length = lua_strlen(m_luaVM, m_iIndex);

            try
            {
                outValue.assign(lua_tostring(m_luaVM, m_iIndex++), length);
            }
            catch (const std::bad_alloc&)
            {
                SetCustomError("out of memory", "Memory allocation");
            }

            return;
        }
        else if (iArgument == LUA_TNONE || iArgument == LUA_TNIL)
        {
            if (defaultValue)
            {
                m_iIndex++;

                try
                {
                    outValue.assign(defaultValue);
                }
                catch (const std::bad_alloc&)
                {
                    SetCustomError("out of memory", "Memory allocation");
                }

                return;
            }
        }

        outValue = "";
        SetTypeError("string");
        m_iIndex++;
    }

    //
    // Force-reads next argument as string
    //
    void ReadAnyAString(std::string& outValue)
    {
        if (luaL_callmeta(m_luaVM, m_iIndex, "__tostring"))
        {
            auto oldIndex = m_iIndex;
            m_iIndex = -1;
            ReadAnyAString(outValue);

            lua_pop(m_luaVM, 1);            // Clean up stack
            m_iIndex = oldIndex + 1;
            return;
        }

        switch (lua_type(m_luaVM, m_iIndex))
        {
            case LUA_TNUMBER:
            case LUA_TSTRING:
                outValue = lua_tostring(m_luaVM, m_iIndex);
                break;
            case LUA_TBOOLEAN:
                outValue = lua_toboolean(m_luaVM, m_iIndex) ? "true" : "false";
                break;
            case LUA_TNIL:
                outValue = "nil";
                break;
            case LUA_TNONE:
                outValue = "";
                SetTypeError("non-none");
                break;
            default:
                outValue = std::string(luaL_typename(m_luaVM, m_iIndex)) + std::string(": ") + std::to_string(reinterpret_cast<long long>(lua_topointer(m_luaVM, m_iIndex)));
                break;
        }

        ++m_iIndex;
    }

    //
    // Read next string as a string reference
    //
    // void ReadCharStringRef(SCharStringRef& outValue)
    // {
    //     int iArgument = lua_type(m_luaVM, m_iIndex);
    //     if (iArgument == LUA_TSTRING)
    //     {
    //         outValue.pData = (char*)lua_tolstring(m_luaVM, m_iIndex++, &outValue.uiSize);
    //         return;
    //     }

    //     outValue.pData = NULL;
    //     outValue.uiSize = 0;
    //     SetTypeError("string");
    //     m_iIndex++;
    // }

    //
    // Read next string as an enum
    //
    // template <class T>
    // void ReadEnumString(T& outValue)
    // {
    //     int iArgument = lua_type(m_luaVM, m_iIndex);
    //     if (iArgument == LUA_TSTRING)
    //     {
    //         String strValue = lua_tostring(m_luaVM, m_iIndex);
    //         if (StringToEnum(strValue, outValue))
    //         {
    //             m_iIndex++;
    //             return;
    //         }
    //     }

    //     outValue = (T)0;
    //     SetTypeError(GetEnumTypeName(outValue));
    //     m_iIndex++;
    // }

    //
    // Read next string as an enum, using default if needed
    //
    // template <class T>
    // void ReadEnumString(T& outValue, const T& defaultValue)
    // {
    //     int iArgument = lua_type(m_luaVM, m_iIndex);
    //     if (iArgument == LUA_TSTRING)
    //     {
    //         String strValue = lua_tostring(m_luaVM, m_iIndex);
    //         if (StringToEnum(strValue, outValue))
    //         {
    //             m_iIndex++;
    //             return;
    //         }
    //     }
    //     else if (iArgument == LUA_TNONE || iArgument == LUA_TNIL)
    //     {
    //         outValue = defaultValue;
    //         m_iIndex++;
    //         return;
    //     }

    //     outValue = (T)0;
    //     SetTypeError(GetEnumTypeName(outValue));
    //     m_iIndex++;
    // }

    //
    // Read next string as a comma separated list of enums, using default if needed
    //
    // template <class T>
    // void ReadEnumStringList(std::vector<T>& outValueList, const std::string& strDefaultValue)
    // {
    //     outValueList.clear();
    //     int     iArgument = lua_type(m_luaVM, m_iIndex);
    //     String strValue;
    //     if (iArgument == LUA_TSTRING)
    //     {
    //         strValue = lua_tostring(m_luaVM, m_iIndex);
    //     }
    //     else if (iArgument == LUA_TNONE || iArgument == LUA_TNIL)
    //     {
    //         strValue = strDefaultValue;
    //     }
    //     else
    //     {
    //         T outValue;
    //         SetTypeError(GetEnumTypeName(outValue));
    //         m_iIndex++;
    //         return;
    //     }

    //     // Parse each part of the string
    //     std::vector<String> inValueList;
    //     strValue.Split(",", inValueList);
    //     for (uint i = 0; i < inValueList.size(); i++)
    //     {
    //         T outValue;
    //         if (StringToEnum(inValueList[i], outValue))
    //         {
    //             outValueList.push_back(outValue);
    //         }
    //         else
    //         {
    //             // Error
    //             SetTypeError(GetEnumTypeName(outValue));
    //             m_bResolvedErrorGotArgumentTypeAndValue = true;
    //             m_strErrorGotArgumentType = EnumToString((eLuaType)lua_type(m_luaVM, m_iErrorIndex));
    //             m_strErrorGotArgumentValue = inValueList[i];

    //             if (iArgument == LUA_TSTRING)
    //                 m_iIndex++;
    //             return;
    //         }
    //     }

    //     // Success
    //     if (iArgument == LUA_TSTRING)
    //         m_iIndex++;
    // }

    //
    // Read next string or number as an enum
    //
    // template <class T>
    // void ReadEnumStringOrNumber(T& outValue)
    // {
    //     int iArgument = lua_type(m_luaVM, m_iIndex);
    //     if (iArgument == LUA_TSTRING)
    //     {
    //         String strValue = lua_tostring(m_luaVM, m_iIndex);
    //         if (StringToEnum(strValue, outValue))
    //         {
    //             m_iIndex++;
    //             return;
    //         }

    //         // If will be coercing a string to an enum, make sure string contains only digits
    //         size_t uiPos = strValue.find_first_not_of("0123456789");
    //         if (uiPos != String::npos || strValue.empty())
    //             iArgument = LUA_TNONE;            //  Force error
    //     }

    //     if (iArgument == LUA_TSTRING || iArgument == LUA_TNUMBER)
    //     {
    //         outValue = static_cast<T>((int)lua_tonumber(m_luaVM, m_iIndex));
    //         if (EnumValueValid(outValue))
    //         {
    //             m_iIndex++;
    //             return;
    //         }
    //     }

    //     outValue = (T)0;
    //     SetTypeError(GetEnumTypeName(outValue));
    //     m_iIndex++;
    // }

protected:
    //
    // Read next userdata, using rules and stuff
    //
    template <class T>
    void InternalReadUserData(bool bAllowNilResult, T*& outValue, bool bHasDefaultValue, T* defaultValue = (T*)-2)
    {
        outValue = NULL;
        int iArgument = lua_type(m_luaVM, m_iIndex);

        if (iArgument == LUA_TLIGHTUSERDATA)
        {
            outValue = (T*)UserDataCast<T>((T*)0, lua_touserdata(m_luaVM, m_iIndex), m_luaVM);
            if (outValue)
            {
                m_iIndex++;
                return;
            }
        }
        else if (iArgument == LUA_TUSERDATA)
        {
            outValue = (T*)UserDataCast<T>((T*)0, *((void**)lua_touserdata(m_luaVM, m_iIndex)), m_luaVM);
            if (outValue)
            {
                m_iIndex++;
                return;
            }
        }
        else if (iArgument == LUA_TNONE || iArgument == LUA_TNIL)
        {
            if (bHasDefaultValue)
                outValue = defaultValue;
            else
                outValue = NULL;

            if (outValue || bAllowNilResult)
            {
                m_iIndex++;
                return;
            }
        }

        outValue = NULL;
        //SetTypeErrorSetTypeError(GetClassTypeName((T*)0));
        m_iIndex++;
    }

public:
    //
    // Read next userdata
    // Userdata always valid if no error
    //  * value not userdata - error
    //  * nil value          - error
    //  * no arguments left  - error
    //  * result is NULL     - error
    //
    template <class T>
    void ReadUserData(T*& outValue)
    {
        InternalReadUserData(false, outValue, false);
    }

    //
    // Read next userdata, using default if needed
    // Userdata always valid if no error
    //  * value not userdata - error
    //  * nil value          - use defaultValue
    //  * no arguments left  - use defaultValue
    //  * result is NULL     - error
    //
    template <class T>
    void ReadUserData(T*& outValue, T* defaultValue)
    {
         InternalReadUserData(false, outValue, true, defaultValue);
    }

    //
    // Read next userdata, using NULL default if needed, allowing NULL result
    // Userdata might be NULL even when no error
    //  * false              - use NULL (For easier use of function returns as arguments)
    //  * value not userdata - error
    //  * nil value          - use NULL
    //  * no arguments left  - use NULL
    //
    // template <class T>
    // void ReadUserData(T*& outValue, int*** defaultValue)
    // {
    //     assert(defaultValue == NULL);
    //     if (NextIsBool() && !lua_toboolean(m_luaVM, m_iIndex))
    //     {
    //         outValue = NULL;
    //         m_iIndex++;
    //         return;
    //     }
    //     InternalReadUserData(true, outValue, true, (T*)NULL);
    // }

    //
    // Read next wrapped userdata
    //
    // template <class T, class U>
    // void ReadUserData(U*& outValue)
    // {
    //     ReadUserData(outValue);
    //     if (outValue)
    //     {
    //         String strErrorExpectedType;
    //         if (CheckWrappedUserDataType<T>(outValue, strErrorExpectedType))
    //             return;
    //         SetTypeError(strErrorExpectedType, m_iIndex - 1);
    //     }
    // }

    //
    // Read CLuaArguments
    //
    void ReadLuaArguments(CLuaArguments& outValue)
    {
        outValue.ReadArguments(m_luaVM, m_iIndex);
        for (int i = outValue.Count(); i > 0; i--)
        {
            m_iIndex++;
        }
    }

    //
    // Read one CLuaArgument
    //
    void ReadLuaArgument(CLuaArgument& outValue)
    {
        int iArgument = lua_type(m_luaVM, m_iIndex);
        if (iArgument != LUA_TNONE)
        {
            outValue.Read(m_luaVM, m_iIndex++);
            return;
        }

        SetTypeError("argument");
        m_iIndex++;
    }

    //
    // Read a table of userdatas
    //
    // template <class T>
    // void ReadUserDataTable(std::vector<T*>& outList)
    // {
    //     if (lua_type(m_luaVM, m_iIndex) != LUA_TTABLE)
    //     {
    //         SetTypeError("table");
    //         m_iIndex++;
    //         return;
    //     }

    //     for (lua_pushnil(m_luaVM); lua_next(m_luaVM, m_iIndex) != 0; lua_pop(m_luaVM, 1))
    //     {
    //         // int idx = lua_tonumber ( m_luaVM, -2 );
    //         int iArgumentType = lua_type(m_luaVM, -1);

    //         T* value = NULL;
    //         if (iArgumentType == LUA_TLIGHTUSERDATA)
    //         {
    //             value = (T*)UserDataCast<T>((T*)0, lua_touserdata(m_luaVM, -1), m_luaVM);
    //         }
    //         else if (iArgumentType == LUA_TUSERDATA)
    //         {
    //             value = (T*)UserDataCast<T>((T*)0, *((void**)lua_touserdata(m_luaVM, -1)), m_luaVM);
    //         }

    //         if (value != NULL)
    //             outList.push_back(value);
    //     }
    //     m_iIndex++;
    // }

    //
    // Read a table of CLuaArguments
    //
    // void ReadLuaArgumentsTable(CLuaArguments& outLuaArguments)
    // {
    //     int iArgument = lua_type(m_luaVM, m_iIndex);
    //     if (iArgument == LUA_TTABLE)
    //     {
    //         for (lua_pushnil(m_luaVM); lua_next(m_luaVM, m_iIndex) != 0; lua_pop(m_luaVM, 1))
    //         {
    //             outLuaArguments.ReadArgument(m_luaVM, -1);
    //         }
    //         m_iIndex++;
    //         return;
    //     }

    //     SetTypeError("table");
    //     m_iIndex++;
    // }

    //
    // Read a table of strings
    //
    // void ReadStringTable(std::vector<std::string>& outList)
    // {
    //     outList.clear();

    //     int iArgument = lua_type(m_luaVM, m_iIndex);
    //     if (iArgument == LUA_TTABLE)
    //     {
    //         for (lua_pushnil(m_luaVM); lua_next(m_luaVM, m_iIndex) != 0; lua_pop(m_luaVM, 1))
    //         {
    //             int iArgument = lua_type(m_luaVM, -1);
    //             if (iArgument == LUA_TSTRING || iArgument == LUA_TNUMBER)
    //             {
    //                 uint uiLength = lua_strlen(m_luaVM, -1);
    //                 outList.push_back(StringX(lua_tostring(m_luaVM, -1), uiLength));
    //             }
    //         }
    //         m_iIndex++;
    //         return;
    //     }

    //     SetTypeError("table");
    //     m_iIndex++;
    // }

    //
    // Reads a table as key-value string pair
    //
    // void ReadStringMap(CStringMap& outMap)
    // {
    //     outMap.clear();

    //     int argument = lua_type(m_luaVM, m_iIndex);
    //     if (argument == LUA_TTABLE)
    //     {
    //         InternalReadStringMap(outMap, m_iIndex);
    //         ++m_iIndex;
    //         return;
    //     }

    //     SetTypeError("table");
    //     ++m_iIndex;
    // }

    //
    // Reads a table of floating point numbers
    // Taken from CrosRoad95 dxDrawPrimitive pull request
    //
    void ReadNumberTable(std::vector<float>& outList)
    {
        outList.clear();
        int iArgument = lua_type(m_luaVM, m_iIndex);
        if (iArgument == LUA_TTABLE)
        {
            for (lua_pushnil(m_luaVM); lua_next(m_luaVM, m_iIndex) != 0; lua_pop(m_luaVM, 1))
            {
                int iArgument = lua_type(m_luaVM, -1);
                if (iArgument == LUA_TNUMBER)
                {
                    outList.push_back(static_cast<float>(lua_tonumber(m_luaVM, -1)));
                }
            }
            m_iIndex++;
            return;
        }
        SetTypeError("table");
        m_iIndex++;
    }

protected:
    // void InternalReadStringMap(CStringMap& outMap, int iIndex)
    // {
    //     lua_pushnil(m_luaVM);
    //     while (lua_next(m_luaVM, iIndex) != 0)
    //     {
    //         int keyType = lua_type(m_luaVM, -2);
    //         int valueType = lua_type(m_luaVM, -1);
    //         if (keyType == LUA_TSTRING)
    //         {
    //             StringMapValue value;
    //             if (valueType == LUA_TSTRING || valueType == LUA_TNUMBER)
    //             {
    //                 uint uiLength = lua_strlen(m_luaVM, -1);
    //                 value.assign(lua_tostring(m_luaVM, -1), uiLength);
    //             }
    //             else if (valueType == LUA_TBOOLEAN)
    //             {
    //                 value = (lua_toboolean(m_luaVM, -1) ? "1" : "0");
    //             }
    //             else if (valueType == LUA_TTABLE)
    //             {
    //                 // Recurse
    //                 InternalReadStringMap(value.subMap, lua_gettop(m_luaVM));
    //             }
    //             outMap.insert({StringX(lua_tostring(m_luaVM, -2)), value});
    //         }
    //         lua_pop(m_luaVM, 1);
    //     }
    // }

public:
    //
    // Read a function, but don't do it yet due to Lua stack issues
    //
    // void ReadFunction(CLuaFunctionRef& outValue, int defaultValue = -2)
    // {
    //     assert(!m_pPendingFunctionOutValue);

    //     int iArgument = lua_type(m_luaVM, m_iIndex);
    //     if (iArgument == LUA_TFUNCTION)
    //     {
    //         m_pPendingFunctionOutValue = &outValue;
    //         m_pPendingFunctionIndex = m_iIndex++;
    //         return;
    //     }
    //     else if (iArgument == LUA_TNONE || iArgument == LUA_TNIL)
    //     {
    //         // Only valid default value for function is nil
    //         if (defaultValue == LUA_REFNIL)
    //         {
    //             outValue = CLuaFunctionRef();
    //             m_iIndex++;
    //             return;
    //         }
    //     }

    //     SetTypeError("function", m_iIndex);
    //     m_iIndex++;
    // }

    //
    // Call after other arguments have been read
    //
    // void ReadFunctionComplete()
    // {
    //     if (!m_pPendingFunctionOutValue)
    //         return;

    //     // As we are going to change the stack, save any error info already gotten
    //     ResolveErrorGotArgumentTypeAndValue();

    //     assert(m_pPendingFunctionIndex != -1);
    //     *m_pPendingFunctionOutValue = luaM_toref(m_luaVM, m_pPendingFunctionIndex);
    //     if (VERIFY_FUNCTION(*m_pPendingFunctionOutValue))
    //     {
    //         m_pPendingFunctionIndex = -1;
    //         return;
    //     }

    //     SetTypeError("function", m_pPendingFunctionIndex);
    //     m_pPendingFunctionIndex = -1;
    // }

    // Debug check
    bool IsReadFunctionPending() const { return /*m_pPendingFunctionOutValue &&*/ m_pPendingFunctionIndex != -1; }

    //
    // Peek at next type
    //
    bool NextIs(int iArgument, int iOffset = 0) const { return iArgument == lua_type(m_luaVM, m_iIndex + iOffset); }
    bool NextIsNone(int iOffset = 0) const { return NextIs(LUA_TNONE, iOffset); }
    bool NextIsNil(int iOffset = 0) const { return NextIs(LUA_TNIL, iOffset); }
    bool NextIsBool(int iOffset = 0) const { return NextIs(LUA_TBOOLEAN, iOffset); }
    bool NextIsUserData(int iOffset = 0) const { return NextIs(LUA_TUSERDATA, iOffset) || NextIsLightUserData(iOffset); }
    bool NextIsLightUserData(int iOffset = 0) const { return NextIs(LUA_TLIGHTUSERDATA, iOffset); }
    bool NextIsNumber(int iOffset = 0) const { return NextIs(LUA_TNUMBER, iOffset); }
    bool NextIString(int iOffset = 0) const { return NextIs(LUA_TSTRING, iOffset); }
    bool NextIsTable(int iOffset = 0) const { return NextIs(LUA_TTABLE, iOffset); }
    bool NextIsFunction(int iOffset = 0) const { return NextIs(LUA_TFUNCTION, iOffset); }
    bool NextCouldBeNumber(int iOffset = 0) const { return NextIsNumber(iOffset) || NextIString(iOffset); }
    bool NextCouldBeString(int iOffset = 0) const { return NextIsNumber(iOffset) || NextIString(iOffset); }

    // template <class T>
    // bool NextIsEnumString(T&, int iOffset = 0)
    // {
    //     int iArgument = lua_type(m_luaVM, m_iIndex + iOffset);
    //     if (iArgument == LUA_TSTRING)
    //     {
    //         T       eDummyResult;
    //         String strValue = lua_tostring(m_luaVM, m_iIndex + iOffset);
    //         return StringToEnum(strValue, eDummyResult);
    //     }
    //     return false;
    // }

    // template <class T>
    // bool NextIsUserDataOfType(int iOffset = 0) const
    // {
    //     int iArgument = lua_type(m_luaVM, m_iIndex + iOffset);
    //     if (iArgument == LUA_TLIGHTUSERDATA)
    //     {
    //         if (UserDataCast<T>((T*)0, lua_touserdata(m_luaVM, m_iIndex + iOffset), m_luaVM))
    //             return true;
    //     }
    //     else if (iArgument == LUA_TUSERDATA)
    //     {
    //         if (UserDataCast<T>((T*)0, *((void**)lua_touserdata(m_luaVM, m_iIndex + iOffset)), m_luaVM))
    //             return true;
    //     }
    //     return false;
    // }

    // bool NextIsVector4D() const
    // {
    //     return (NextCouldBeNumber() && NextCouldBeNumber(1) && NextCouldBeNumber(2) && NextCouldBeNumber(3)) || NextIsUserDataOfType<CLuaVector4D>();
    // }

    // bool NextIsVector3D() const
    // {
    //     return (NextCouldBeNumber() && NextCouldBeNumber(1) && NextCouldBeNumber(2)) || NextIsUserDataOfType<CLuaVector3D>() ||
    //            NextIsUserDataOfType<CLuaVector4D>();
    // }

    // bool NextIsVector2D() const
    // {
    //     return (NextCouldBeNumber() && NextCouldBeNumber(1)) || NextIsUserDataOfType<CLuaVector2D>() || NextIsUserDataOfType<CLuaVector3D>() ||
    //            NextIsUserDataOfType<CLuaVector4D>();
    // }

    //
    // Conditional reads. Default required in case condition is not met.
    //
    void ReadIfNextIsBool(bool& bOutValue, const bool bDefaultValue)
    {
        if (NextIsBool())
            ReadBool(bOutValue, bDefaultValue);
        else
            bOutValue = bDefaultValue;
    }

    template <class T>
    void ReadIfNextIsUserData(T*& outValue, T* defaultValue)
    {
        if (NextIsUserData())
            ReadUserData(outValue, defaultValue);
        else
            outValue = defaultValue;
    }

    template <class T, class U>
    void ReadIfNextIsNumber(T& outValue, const U& defaultValue)
    {
        if (NextIsNumber())
            ReadNumber(outValue, defaultValue);
        else
            outValue = defaultValue;
    }

    void ReadIfNextIString(std::string& outValue, const char* defaultValue)
    {
        if (NextIString())
            ReadString(outValue, defaultValue);
        else
            outValue = defaultValue;
    }

    template <class T>
    void ReadIfNextIsEnumString(T& outValue, const T& defaultValue)
    {
        if (NextIsEnumString(outValue))
            ReadEnumString(outValue, defaultValue);
        else
            outValue = defaultValue;
    }

    template <class T, class U>
    void ReadIfNextCouldBeNumber(T& outValue, const U& defaultValue)
    {
        if (NextCouldBeNumber())
            ReadNumber(outValue, defaultValue);
        else
            outValue = defaultValue;
    }

    void ReadIfNextCouldBeString(std::string& outValue, const char* defaultValue)
    {
        if (NextCouldBeString())
            ReadString(outValue, defaultValue);
        else
            outValue = defaultValue;
    }

    //
    // SetTypeError
    //
    void SetTypeError(const std::string& strExpectedType, int iIndex = -1)
    {
        if (iIndex == -1)
            iIndex = m_iIndex;
        if (!m_bError || iIndex <= m_iErrorIndex)
        {
            m_bError = true;
            m_iErrorIndex = iIndex;
            m_strErrorExpectedType = strExpectedType;
            m_bResolvedErrorGotArgumentTypeAndValue = false;
            m_strErrorCategory = "Bad argument";
        }
    }

    //
    // HasErrors - Optional check if there are any unread arguments
    //
    bool HasErrors(bool bCheckUnusedArgs = false)
    {
        //assert(!IsReadFunctionPending());
        if (bCheckUnusedArgs && lua_type(m_luaVM, m_iIndex) != LUA_TNONE)
            return true;

        // Output warning here (there's no better way to integrate it without huge code changes
        if (!m_bError && !m_strCustomWarning.empty())
        {
            // #ifdef MTA_CLIENT
            // CLuaFunctionDefs::m_pScriptDebugging->LogWarning(m_luaVM, m_strCustomWarning);
            // #else
            // g_pGame->GetScriptDebugging()->LogWarning(m_luaVM, m_strCustomWarning);
            // #endif

            m_strCustomWarning.clear();
        }

        return m_bError;
    }

    //
    // GetErrorMessage
    //
    // std::string GetErrorMessage()
    // {
    //     if (!m_bError)
    //         return "No error";

    //     if (m_bHasCustomMessage)
    //         return m_strCustomMessage;

    //     ResolveErrorGotArgumentTypeAndValue();

    //     // Compose error message
    //     std::string strMessage(std::string("Expected ") + m_strErrorExpectedType + " at argument " + std::to_string(m_iErrorIndex));

    //     if (!m_strErrorGotArgumentType.empty())
    //     {
    //         strMessage +=  std::string(", got ") + m_strErrorGotArgumentType;

    //         // Append value if available
    //         if (!m_strErrorGotArgumentValue.empty())
    //             strMessage += std::string(" '") + m_strErrorGotArgumentValue + "'";
    //     }

    //     return strMessage;
    // }

    //
    // Put off getting error type and value until just before we need it
    //
    // void ResolveErrorGotArgumentTypeAndValue()
    // {
    //     if (!m_bError || m_bResolvedErrorGotArgumentTypeAndValue)
    //         return;

    //     m_bResolvedErrorGotArgumentTypeAndValue = true;

    //     if (!m_bHasCustomMessage)
    //     {
    //         int iArgument = lua_type(m_luaVM, m_iErrorIndex);
    //         m_strErrorGotArgumentType = EnumToString((eLuaType)iArgument);
    //         m_strErrorGotArgumentValue = lua_tostring(m_luaVM, m_iErrorIndex);

    //         if (iArgument == LUA_TLIGHTUSERDATA)
    //         {
    //             m_strErrorGotArgumentType = GetUserDataClassName(lua_touserdata(m_luaVM, m_iErrorIndex), m_luaVM);
    //             m_strErrorGotArgumentValue = "";
    //         }
    //         else if (iArgument == LUA_TUSERDATA)
    //         {
    //             m_strErrorGotArgumentType = GetUserDataClassName(*((void**)lua_touserdata(m_luaVM, m_iErrorIndex)), m_luaVM);
    //             m_strErrorGotArgumentValue = "";
    //         }
    //     }
    // }

    //
    // Set custom error message
    //
    void SetCustomError(const char* szReason, const char* szCategory = "Bad usage")
    {
        if (!m_bError)
        {
            m_bError = true;
            m_strErrorCategory = szCategory;
            m_bHasCustomMessage = true;
            m_strCustomMessage = szReason;
        }
    }

    //
    // Make full error message
    //
    // std::string GetFullErrorMessage() { return std::string("%s @ '%s' [%s]", *m_strErrorCategory, lua_tostring(m_luaVM, lua_upvalueindex(1)), *GetErrorMessage()); }

    //
    // Set custom warning message
    //
    void SetCustomWarning(const std::string& message) { m_strCustomWarning = message; }

    //
    // Skip n arguments
    //
    void Skip(int n) { m_iIndex += n; }

    bool             m_bError;
    int              m_iErrorIndex;
    std::string      m_strErrorExpectedType;
    int              m_iIndex;
    lua_State*       m_luaVM;
    //CLuaFunctionRef* m_pPendingFunctionOutValue;
    int              m_pPendingFunctionIndex;
    bool             m_bResolvedErrorGotArgumentTypeAndValue;
    std::string      m_strErrorGotArgumentType;
    std::string      m_strErrorGotArgumentValue;
    std::string      m_strErrorCategory;
    bool             m_bHasCustomMessage;
    std::string      m_strCustomMessage;
    std::string      m_strCustomWarning;
};