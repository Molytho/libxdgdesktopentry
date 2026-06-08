#ifndef LIB_XDGDESKTOPENTRYPP_HELPER_H
#define LIB_XDGDESKTOPENTRYPP_HELPER_H

#if defined _WIN32 || defined __CYGWIN__
# ifdef BUILDING_LIBXDGDESKTOPENTRY
#  define LIBXDGDESKTOPENTRY_PUBLIC __declspec(dllexport)
# else
#  define LIBXDGDESKTOPENTRY_PUBLIC __declspec(dllimport)
# endif
#elif defined __OS2__
# ifdef BUILDING_LIBXDGDESKTOPENTRY
#  define LIBXDGDESKTOPENTRY_PUBLIC __declspec(dllexport)
# else
#  define LIBXDGDESKTOPENTRY_PUBLIC
# endif
#else
# ifdef BUILDING_LIBXDGDESKTOPENTRY
#  define LIBXDGDESKTOPENTRY_PUBLIC __attribute__((visibility("default")))
# else
#  define LIBXDGDESKTOPENTRY_PUBLIC
# endif
#endif

#endif