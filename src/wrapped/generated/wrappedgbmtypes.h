/*******************************************************************
 * File automatically generated by rebuild_wrappers.py (v2.3.0.19) *
 *******************************************************************/
#ifndef __wrappedgbmTYPES_H_
#define __wrappedgbmTYPES_H_

#ifndef LIBNAME
#error You should only #include this file inside a wrapped*.c file
#endif
#ifndef ADDED_FUNCTIONS
#define ADDED_FUNCTIONS() 
#endif

typedef void (*vFppp_t)(void*, void*, void*);

#define SUPER() ADDED_FUNCTIONS() \
	GO(gbm_bo_set_user_data, vFppp_t)

#endif // __wrappedgbmTYPES_H_
