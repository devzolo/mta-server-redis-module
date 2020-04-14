{
  'variables': {
    'platform': '<(OS)',
    'build_arch': '<!(node -p "process.arch")',
    'build_win_platform': '<!(node -p "process.arch==\'ia32\'?\'Win32\':process.arch")',
    'module_name': 'ml_hello'
  },
  'conditions': [
    # Replace gyp platform with node platform, blech
    ['platform == "mac"', {'variables': {'platform': 'darwin', 'lib_target':'lib.target/', 'shared_library_ext': '.dylib'}}],
    ['platform == "linux"', {'variables': {'platform': 'linux', 'lib_target':'lib.target/','shared_library_ext': '.so'}}],
    ['platform == "win"', {'variables': {'platform': 'win32', 'lib_target':'','shared_library_ext': '.dll'}}],
  ],
  'targets': [
    {
      'target_name': '<(module_name)',
      'type': 'shared_library',
      "dependencies" : [ "lua5.1s" ],
      "cflags!": [ "-fno-exceptions", "-std=c++11" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      'sources': [
        "src/extra/CLuaArgument.cpp",
        "src/extra/CLuaArguments.cpp",
        "src/CFunctions.cpp",
        "src/CThread.cpp",
        "src/CThreadData.cpp",
        "src/ml_base.cpp",
      ],
      'defines' : [],
      "include_dirs": [
        "./"
        "./vendor/lua/src"
      ],
      'msvs_disabled_warnings': [4101, 4293, 4018, 4334]
    },
    {
      'target_name': 'lua5.1s',
      'type': 'static_library',
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      'sources': [
        "vendor/lua/src/lapi.c",
        "vendor/lua/src/lcode.c",
        "vendor/lua/src/ldebug.c",
        "vendor/lua/src/ldo.c",
        "vendor/lua/src/ldump.c",
        "vendor/lua/src/lfunc.c",
        "vendor/lua/src/lgc.c",
        "vendor/lua/src/llex.c",
        "vendor/lua/src/lmem.c",
        "vendor/lua/src/lobject.c",
        "vendor/lua/src/lopcodes.c",
        "vendor/lua/src/lparser.c",
        "vendor/lua/src/lstate.c",
        "vendor/lua/src/lstring.c",
        "vendor/lua/src/ltable.c",
        "vendor/lua/src/ltm.c",
        "vendor/lua/src/lundump.c",
        "vendor/lua/src/lvm.c",
        "vendor/lua/src/lzio.c",
        "vendor/lua/src/lauxlib.c",
        "vendor/lua/src/lbaselib.c",
        "vendor/lua/src/ldblib.c",
        "vendor/lua/src/liolib.c",
        "vendor/lua/src/lmathlib.c",
        "vendor/lua/src/loslib.c",
        "vendor/lua/src/ltablib.c",
        "vendor/lua/src/lstrlib.c",
        "vendor/lua/src/loadlib.c",
        "vendor/lua/src/linit.c",
      ],
      'defines' : [],
      'libraries': [],
      "include_dirs": [
        "./vendor/lua/src"
      ],
      'library_dirs' : [],
      'msvs_disabled_warnings': [4101, 4293, 4018, 4334]
    },
    {
      "target_name": "copy_modules",
      "type":"none",
      "dependencies" : [ "<(module_name)" ],
      "copies":[
        {
          'destination': '<(module_root_dir)/bin/<(platform)/<(target_arch)',
          'files':  ['<(module_root_dir)/build/Release/<(lib_target)<(module_name)<(shared_library_ext)']
        }
      ]
    }
  ]
}
