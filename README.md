# SGLThing

Estimated Requirements
- atleast 255MB of RAM
- a graphics card capable of OpenGL 3.2 core
- a computer

## Windows Cross-Compilation Notes

Assimp doesn't copy itself over to the build directory for some reason. Just do cp deps/assimp/bin/libassimp-5.dll . to get it in.

Wine (by default) wont detect the MinGW DLL's. Copy them over something like as follows

    cp /usr/i686-w64-mingw/bin/libstdc++-6.dll .
    cp /usr/i686-w64-mingw/bin/libgcc_s_dw2-1.dll .
    cp /usr/i686-w64-mingw/bin/libwinpthread-1.dll .

Assimp requires stdc++. I don't like that, but I guess we have to deal with it.

If ODE complains about not finding "Windows.h", just set the name of the include to "windows.h"