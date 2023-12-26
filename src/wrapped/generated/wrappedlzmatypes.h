/*******************************************************************
 * File automatically generated by rebuild_wrappers.py (v2.3.0.19) *
 *******************************************************************/
#ifndef __wrappedlzmaTYPES_H_
#define __wrappedlzmaTYPES_H_

#ifndef LIBNAME
#error You should only #include this file inside a wrapped*.c file
#endif
#ifndef ADDED_FUNCTIONS
#define ADDED_FUNCTIONS() 
#endif

typedef void (*vFp_t)(void*);
typedef void (*vFpp_t)(void*, void*);
typedef int32_t (*iFpi_t)(void*, int32_t);
typedef int32_t (*iFpU_t)(void*, uint64_t);
typedef int32_t (*iFpp_t)(void*, void*);
typedef int32_t (*iFpui_t)(void*, uint32_t, int32_t);
typedef int32_t (*iFpUi_t)(void*, uint64_t, int32_t);
typedef int32_t (*iFppi_t)(void*, void*, int32_t);
typedef int32_t (*iFpppL_t)(void*, void*, void*, uintptr_t);
typedef int32_t (*iFpppppL_t)(void*, void*, void*, void*, void*, uintptr_t);
typedef int32_t (*iFpupppLppL_t)(void*, uint32_t, void*, void*, void*, uintptr_t, void*, void*, uintptr_t);

#define SUPER() ADDED_FUNCTIONS() \
	GO(lzma_end, vFp_t) \
	GO(lzma_index_end, vFpp_t) \
	GO(lzma_code, iFpi_t) \
	GO(lzma_alone_decoder, iFpU_t) \
	GO(lzma_alone_encoder, iFpp_t) \
	GO(lzma_raw_decoder, iFpp_t) \
	GO(lzma_raw_encoder, iFpp_t) \
	GO(lzma_stream_encoder_mt, iFpp_t) \
	GO(lzma_easy_encoder, iFpui_t) \
	GO(lzma_stream_decoder, iFpUi_t) \
	GO(lzma_stream_encoder, iFppi_t) \
	GO(lzma_properties_decode, iFpppL_t) \
	GO(lzma_index_buffer_decode, iFpppppL_t) \
	GO(lzma_stream_buffer_decode, iFpupppLppL_t)

#endif // __wrappedlzmaTYPES_H_
