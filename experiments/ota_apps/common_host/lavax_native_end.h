//
//  @file lavax_native_end.h
//  @brief LavaX 原生编译类型兼容头文件（结束部分）
//
//  取消 begin 头里对用户源码区域启用的类型映射。
//

#ifndef LAVAX_NATIVE_BEGIN_H
#error "Must include lavax_native_begin.h before lavax_native_end.h"
#endif

#undef char
#undef int
#undef long
#undef float
#undef addr
