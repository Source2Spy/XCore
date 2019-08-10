#ifndef _XCORE_PROFILE_H
#define _XCORE_PROFILE_H
#pragma once

#include "../dependencies/tracy/Tracy.hpp"
#include "../dependencies/tracy/common/TracySystem.hpp"

#define XCORE_PROF_ZONE_NAMED(varname, active)                          ZoneNamed(varname, active)
#define XCORE_PROF_ZONE_NAMED_N(varname, name, active)                  ZoneNamedN(varname, name, active)
#define XCORE_PROF_ZONE_NAMED_C(varname, color, active)                 ZoneNamedC(varname, color, active)
#define XCORE_PROF_ZONE_NAMED_NC(varname, name, color, active)          ZoneNamedNC(varname, name, color, active)
               
#define XCORE_PROF_ZONE_SCOPED()                                        ZoneScoped
#define XCORE_PROF_ZONE_SCOPED_N(name)                                  ZoneScopedN(name)
#define XCORE_PROF_ZONE_SCOPED_C(color)                                 ZoneScopedC(color)
#define XCORE_PROF_ZONE_SCOPED_NC(name, color)                          ZoneScopedNC(name, color)
               
#define XCORE_PROF_ZONE_TEXT(txt, size)                                 ZoneText(txt, size)
#define XCORE_PROF_ZONE_NAME(txt, size)                                 ZoneName(txt, size)
               
#define XCORE_PROF_FRAME_MARK()                                         FrameMark
#define XCORE_PROF_FRAME_MARK_NAME(name)                                FrameMarkNamed(X)
#define XCORE_PROF_FRAME_MARK_START(name)                               FrameMarkStart(X)
#define XCORE_PROF_FRAME_MARK_END(name)                                 FrameMarkEnd(X)
               
#define XCORE_PROF_FRAME_IMAGE(image, width, height, offset, flip)      FrameImage(image, width, height, offset, flip)
               
#define XCORE_PROF_LOCKABLE( type, varname )                            TracyLockable( type, varname ) type varname;
#define XCORE_PROF_LOCKABLE_N( TYPE, VAR_NAME, DESC )                   TracyLockableN( TYPE, VAR_NAME, DESC ) TYPE VAR_NAME;
#define XCORE_PROF_SHARED_LOCKABLE( TYPE, VAR_NAME )                    TracySharedLockable( TYPE, VAR_NAME ) TYPE VAR_NAME;
#define XCORE_PROF_SHARED_LOCKABLE_N( TYPE, VAR_NAME, DESC )            TracySharedLockableN( TYPE, VAR_NAME, DESC ) TYPE VAR_NAME;
#define XCORE_PROF_LOCKABLE_BASE( TYPE )                                LockableBase( TYPE ) TYPE
#define XCORE_PROF_SHARED_LOCKABLE_BASE( TYPE )                         SharedLockableBase( TYPE ) TYPE
#define XCORE_PROF_LOCK_MARK(varname)                                   LockMark(varname) (void)varname;
               
#define XCORE_PROF_PLOT(name, val)                                      TracyPlot(name, val)
               
#define XCORE_PROF_MESSAGE(txt, size)                                   TracyMessage(txt, size)
#define XCORE_PROF_MESSAGE_L(txt)                                       TracyMessageL(txt)
#define XCORE_PROF_MESSAGE_C(txt, size, color)                          TracyMessageC(txt, size, color)
#define XCORE_PROF_MESSAGE_LC(txt, color)                               TracyMessageLC(txt, color)
#define XCORE_PROF_APP_INFO(txt, size)                                  TracyAppInfo(txt, size)
               
#define XCORE_PROF_ALLOC(ptr, size)                                     TracyAlloc(ptr, size)
#define XCORE_PROF_FREE(ptr)                                            TracyFree(ptr)
               
#define XCORE_PROF_ZONE_NAMED_S(varname, depth, active)                 ZoneNamedS(varname, depth, active)
#define XCORE_PROF_ZONE_NAMED_NS(varname, name, depth, active)          ZoneNamedNS(varname, name, depth, active)
#define XCORE_PROF_ZONE_NAMED_CS(varname, color, depth, active)         ZoneNamedCS(varname, color, depth, active)
#define XCORE_PROF_ZONE_NAMED_NCS(varname, name, color, depth, active)  ZoneNamedNCS(varname, name, color, depth, active)
               
#define XCORE_PROF_SCOPED_S(depth)                                      ZoneScopedS(depth)
#define XCORE_PROF_SCOPED_NS(name, depth)                               ZoneScopedNS(name, depth)
#define XCORE_PROF_SCOPED_CS(color, depth)                              ZoneScopedCS(color, depth)
#define XCORE_PROF_SCOPED_NCS(name, color, depth)                       ZoneScopedNCS(name, color, depth)
               
#define XCORE_PROF_ALLOC_S(ptr, size, depth)                            TracyAllocS(ptr, size, depth)
#define XCORE_PROF_FREE_S(ptr, depth)                                   TracyFreeS(ptr, depth)

namespace xcore::profile { using namespace ::tracy; };

#endif