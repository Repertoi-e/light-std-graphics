#pragma once

//
// A header which defines macros for wrapping this library in a namespace
// Define LSTD_NAMESPACE as a preprocessor definition to the value you want the namespace to be called.
// (By default the libary has the namespace "lstd").
//
// * If you want to build this library without a namespace, define LSTD_NO_NAMESPACE.
//

#if !defined LSTD_NAMESPACE and !defined LSTD_NO_NAMESPACE
#define LSTD_NAMESPACE lstd
#endif

#if defined LSTD_NO_NAMESPACE
#undef LSTD_NAMESPACE
#endif

#if defined LSTD_NAMESPACE
#define LSTD_BEGIN_NAMESPACE namespace LSTD_NAMESPACE {
#define LSTD_END_NAMESPACE }
#define LSTD_USING_NAMESPACE using namespace LSTD_NAMESPACE
#else
#define LSTD_NAMESPACE
#define LSTD_BEGIN_NAMESPACE
#define LSTD_END_NAMESPACE
#define LSTD_USING_NAMESPACE
#endif

// "module : private" must be in the global namespace.
// Any modification in the private module fragment doesn't trigger recompilation
// of stuff that imports the module (just like .cpp files), which is awesome.
//
// clang-format off
#define LSTD_MODULE_PRIVATE \
    LSTD_END_NAMESPACE      \
                            \
    module :private;        \
                            \
    LSTD_BEGIN_NAMESPACE
