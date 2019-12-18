#ifndef _XCORE_PERFILE_H
#define _XCORE_PERFILE_H
#pragma once

    #ifdef _XCORE_PROFILE

        #include "../dependencies/tracy/Tracy.hpp"
        #include "../dependencies/tracy/common/TracySystem.hpp"

        #define XCORE_PERF_ZONE_NAMED(varname, active)                          ZoneNamed(varname, active)
        #define XCORE_PERF_ZONE_NAMED_N(varname, name, active)                  ZoneNamedN(varname, name, active)
        #define XCORE_PERF_ZONE_NAMED_C(varname, color, active)                 ZoneNamedC(varname, color, active)
        #define XCORE_PERF_ZONE_NAMED_NC(varname, name, color, active)          ZoneNamedNC(varname, name, color, active)
               
        #define XCORE_PERF_ZONE_SCOPED()                                        ZoneScoped
        #define XCORE_PERF_ZONE_SCOPED_N(name)                                  ZoneScopedN(name)
        #define XCORE_PERF_ZONE_SCOPED_C(color)                                 ZoneScopedC(color)
        #define XCORE_PERF_ZONE_SCOPED_NC(name, color)                          ZoneScopedNC(name, color)
               
        #define XCORE_PERF_ZONE_TEXT(txt, size)                                 ZoneText(txt, size)
        #define XCORE_PERF_ZONE_NAME(txt, size)                                 ZoneName(txt, size)
               
        #define XCORE_PERF_FRAME_MARK()                                         FrameMark
        #define XCORE_PERF_FRAME_MARK_NAME(name)                                FrameMarkNamed(name)
        #define XCORE_PERF_FRAME_MARK_START(name)                               FrameMarkStart(name)
        #define XCORE_PERF_FRAME_MARK_END(name)                                 FrameMarkEnd(name)
               
        #define XCORE_PERF_FRAME_IMAGE(image, width, height, offset, flip)      FrameImage(image, width, height, offset, flip)
               
        #define XCORE_PERF_LOCKABLE( type, varname )                            TracyLockable( type, varname ) type varname;
        #define XCORE_PERF_LOCKABLE_N( TYPE, VAR_NAME, DESC )                   TracyLockableN( TYPE, VAR_NAME, DESC ) TYPE VAR_NAME;
        #define XCORE_PERF_SHARED_LOCKABLE( TYPE, VAR_NAME )                    TracySharedLockable( TYPE, VAR_NAME ) TYPE VAR_NAME;
        #define XCORE_PERF_SHARED_LOCKABLE_N( TYPE, VAR_NAME, DESC )            TracySharedLockableN( TYPE, VAR_NAME, DESC ) TYPE VAR_NAME;
        #define XCORE_PERF_LOCKABLE_BASE( TYPE )                                LockableBase( TYPE ) TYPE
        #define XCORE_PERF_SHARED_LOCKABLE_BASE( TYPE )                         SharedLockableBase( TYPE ) TYPE
        #define XCORE_PERF_LOCK_MARK(varname)                                   LockMark(varname) (void)varname;
               
        #define XCORE_PERF_PLOT(name, val)                                      TracyPlot(name, val)
        #define XCORE_PERF_PLOT_CONFIG(name,type)                               TracyPlotConfig(name,type)
               
        #define XCORE_PERF_MESSAGE(txt, size)                                   TracyMessage(txt, size)
        #define XCORE_PERF_MESSAGE_L(txt)                                       TracyMessageL(txt)
        #define XCORE_PERF_MESSAGE_C(txt, size, color)                          TracyMessageC(txt, size, color)
        #define XCORE_PERF_MESSAGE_LC(txt, color)                               TracyMessageLC(txt, color)
        #define XCORE_PERF_APP_INFO(txt, size)                                  TracyAppInfo(txt, size)
               
        #define XCORE_PERF_ALLOC(ptr, size)                                     TracyAlloc(ptr, size)
        #define XCORE_PERF_FREE(ptr)                                            TracyFree(ptr)
               
        #define XCORE_PERF_ZONE_NAMED_S(varname, depth, active)                 ZoneNamedS(varname, depth, active)
        #define XCORE_PERF_ZONE_NAMED_NS(varname, name, depth, active)          ZoneNamedNS(varname, name, depth, active)
        #define XCORE_PERF_ZONE_NAMED_CS(varname, color, depth, active)         ZoneNamedCS(varname, color, depth, active)
        #define XCORE_PERF_ZONE_NAMED_NCS(varname, name, color, depth, active)  ZoneNamedNCS(varname, name, color, depth, active)
               
        #define XCORE_PERF_SCOPED_S(depth)                                      ZoneScopedS(depth)
        #define XCORE_PERF_SCOPED_NS(name, depth)                               ZoneScopedNS(name, depth)
        #define XCORE_PERF_SCOPED_CS(color, depth)                              ZoneScopedCS(color, depth)
        #define XCORE_PERF_SCOPED_NCS(name, color, depth)                       ZoneScopedNCS(name, color, depth)
               
        #define XCORE_PERF_ALLOC_S(ptr, size, depth)                            TracyAllocS(ptr, size, depth)
        #define XCORE_PERF_FREE_S(ptr, depth)                                   TracyFreeS(ptr, depth)


        //#define XCORE_PERF_CUSTOM_LOCK_DEF(type, varname)         tracy::LockableCtx varname{ [] () -> const tracy::SourceLocationData* { static const tracy::SourceLocationData srcloc { nullptr, #type " " #varname, __FILE__, __LINE__, 0 }; return &srcloc; }() };
        //#define XCORE_PERF_CUSTOM_SHARE_LOCK_DEF(type, varname)   mutable tracy::SharedLockableCtx varname{ [] () -> const tracy::SourceLocationData* { static const tracy::SourceLocationData srcloc { nullptr, #type " " #varname, __FILE__, __LINE__, 0 }; return &srcloc; }() };

        namespace xcore::profile { using namespace ::tracy; };

        #define XCORE_CMD_PROFILER(...) __VA_ARGS__

    #else

        #define XCORE_PERF_ZONE_NAMED(varname, active)                        
        #define XCORE_PERF_ZONE_NAMED_N(varname, name, active)                
        #define XCORE_PERF_ZONE_NAMED_C(varname, color, active)               
        #define XCORE_PERF_ZONE_NAMED_NC(varname, name, color, active)        
       
        #define XCORE_PERF_ZONE_SCOPED()                                      
        #define XCORE_PERF_ZONE_SCOPED_N(name)                                
        #define XCORE_PERF_ZONE_SCOPED_C(color)                               
        #define XCORE_PERF_ZONE_SCOPED_NC(name, color)                        
       
        #define XCORE_PERF_ZONE_TEXT(txt, size)                               
        #define XCORE_PERF_ZONE_NAME(txt, size)                               
       
        #define XCORE_PERF_FRAME_MARK()                                       
        #define XCORE_PERF_FRAME_MARK_NAME(name)                              
        #define XCORE_PERF_FRAME_MARK_START(name)                             
        #define XCORE_PERF_FRAME_MARK_END(name)                               
       
        #define XCORE_PERF_FRAME_IMAGE(image, width, height, offset, flip)    
       
        #define XCORE_PERF_LOCKABLE( type, varname )                          
        #define XCORE_PERF_LOCKABLE_N( TYPE, VAR_NAME, DESC )                 
        #define XCORE_PERF_SHARED_LOCKABLE( TYPE, VAR_NAME )                  
        #define XCORE_PERF_SHARED_LOCKABLE_N( TYPE, VAR_NAME, DESC )          
        #define XCORE_PERF_LOCKABLE_BASE( TYPE )                              
        #define XCORE_PERF_SHARED_LOCKABLE_BASE( TYPE )                       
        #define XCORE_PERF_LOCK_MARK(varname)                                 
       
        #define XCORE_PERF_PLOT(name, val)                                    
        #define XCORE_PERF_PLOT_CONFIG(name,type)                             
       
        #define XCORE_PERF_MESSAGE(txt, size)                                 
        #define XCORE_PERF_MESSAGE_L(txt)                                     
        #define XCORE_PERF_MESSAGE_C(txt, size, color)                        
        #define XCORE_PERF_MESSAGE_LC(txt, color)                             
        #define XCORE_PERF_APP_INFO(txt, size)                                
       
        #define XCORE_PERF_ALLOC(ptr, size)                                   
        #define XCORE_PERF_FREE(ptr)                                          
       
        #define XCORE_PERF_ZONE_NAMED_S(varname, depth, active)               
        #define XCORE_PERF_ZONE_NAMED_NS(varname, name, depth, active)        
        #define XCORE_PERF_ZONE_NAMED_CS(varname, color, depth, active)       
        #define XCORE_PERF_ZONE_NAMED_NCS(varname, name, color, depth, active)
       
        #define XCORE_PERF_SCOPED_S(depth)                                    
        #define XCORE_PERF_SCOPED_NS(name, depth)                             
        #define XCORE_PERF_SCOPED_CS(color, depth)                            
        #define XCORE_PERF_SCOPED_NCS(name, color, depth)                     
       
        #define XCORE_PERF_ALLOC_S(ptr, size, depth)                          
        #define XCORE_PERF_FREE_S(ptr, depth)                                 

      //  #define XCORE_PERF_CUSTOM_LOCK_DEF(type, varname)
      //  #define XCORE_PERF_CUSTOM_SHARE_LOCK_DEF(type, varname)

        #define XCORE_CMD_PROFILER(...)

    #endif
#endif