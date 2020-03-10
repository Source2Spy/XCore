#include "xcore.h"

#if _XCORE_COMPILER_VISUAL_STUDIO
    #include "windows.h"
#endif

#include <iostream>
#include <stdarg.h>
#include <algorithm>
#include <iterator>
#include <malloc.h>

#include "Implementation/xcore_global.cpp"
#include "Implementation/xcore_log.cpp"
//#include "Implementation/xcore_textfile.cpp"
#include "Implementation/xcore_textfile2.cpp"
#include "Implementation/xcore_sprintf.cpp"
#include "Implementation/xcore_scheduler.cpp"
#include "Implementation/xcore_cmdline.cpp"
#include "Implementation/xcore_file.cpp"
#include "Implementation/xcore_memory.cpp"
#include "Implementation/xcore_compression.cpp"
#include "Implementation/xcore_serializer.cpp"

