/*******************************************************************
 * File automatically generated by rebuild_wrappers.py (v2.3.0.19) *
 *******************************************************************/
#ifndef __wrappedopenclTYPES_H_
#define __wrappedopenclTYPES_H_

#ifndef LIBNAME
#error You should only #include this file inside a wrapped*.c file
#endif
#ifndef ADDED_FUNCTIONS
#define ADDED_FUNCTIONS() 
#endif

typedef void* (*pFpuppp_t)(void*, uint32_t, void*, void*, void*);
typedef int32_t (*iFpupppp_t)(void*, uint32_t, void*, void*, void*, void*);
typedef void* (*pFpupppp_t)(void*, uint32_t, void*, void*, void*, void*);

#define SUPER() ADDED_FUNCTIONS() \
	GO(clCreateContextFromType, pFpuppp_t) \
	GO(clBuildProgram, iFpupppp_t) \
	GO(clCreateContext, pFpupppp_t)

#endif // __wrappedopenclTYPES_H_
