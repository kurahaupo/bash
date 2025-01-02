#if BUILDING_LIBINTL
/* HAVE_VISIBILITY is set or unset in config.h, so make sure that's already
 * been included */
# ifndef _CONFIG_H_
#  error "Must #include <config.h> first"
# endif
# if HAVE_VISIBILITY
#  define LIBINTL_DLL_EXPORTED __attribute__((__visibility__("default")))
# elif defined _MSC_VER
#  define LIBINTL_DLL_EXPORTED __declspec(dllexport)
# endif
#endif
#ifndef LIBINTL_DLL_EXPORTED
# define LIBINTL_DLL_EXPORTED
#endif
