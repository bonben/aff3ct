/*!
 * \file
 * \brief C ABI between the AFF3CT dispatch host (compiled without any SIMD option) and the AFF3CT SIMD plugins
 *        (one shared object per instruction set: sse4.2, avx2, avx512, neon...).
 *
 * Rules of the frontier (they make it SIMD neutral):
 *   - only C types cross it: integers, C strings, opaque handles and pointers to StreamPU objects;
 *   - a StreamPU object (spu::module::Module) is given as a 'void*' that has been obtained in the plugin with
 *     'static_cast<void*>(static_cast<spu::module::Module*>(m))': the host casts it back to 'spu::module::Module*'.
 *     StreamPU contains no SIMD code and is a single shared library ('libstreampu.so') compiled with the base
 *     options, loaded once and common to the host and to the plugin: its classes have the same layout on both sides;
 *   - no C++ exception crosses these functions: errors are reported by a NULL handle or a non-zero return code plus
 *     a message copied in 'err' (at most 'err_len' bytes, NUL terminated);
 *   - every object created by the plugin is destroyed by the plugin ('*_destroy').
 *
 * The plugin must not be loaded before the host has checked that the CPU supports its instruction set: its static
 * initializers already contain SIMD instructions.
 */
#ifndef AFF3CT_PLUGIN_ABI_H_
#define AFF3CT_PLUGIN_ABI_H_

#include <stddef.h>

/* Increment each time a function below is added, removed or changes its signature or semantics. */
#define AFF3CT_PLUGIN_ABI_VERSION 1

#if defined(_WIN32)
#define AFF3CT_PLUGIN_EXPORT __declspec(dllexport)
#else
#define AFF3CT_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /* Opaque codec handle (an AFF3CT 'tools::Codec_SIHO<B,Q>' and its factory, owned by the plugin). */
    typedef struct aff3ct_plugin_codec aff3ct_plugin_codec;

    /* ----------------------------------------------------------------------------------------------- identity */
    /* AFF3CT_PLUGIN_ABI_VERSION the plugin was compiled with. */
    typedef int (*aff3ct_plugin_abi_version_t)(void);
    /* AFF3CT version string (must be equal to the host one). */
    typedef const char* (*aff3ct_plugin_version_t)(void);
    /* Variant name as given at configure time ('sse4.2', 'avx2', 'avx512', 'neon'...). */
    typedef const char* (*aff3ct_plugin_isa_t)(void);
    /* MIPP instruction set really used inside the plugin ('SSE4.2', 'AVX2', 'AVX512', 'NEONv2'...). */
    typedef const char* (*aff3ct_plugin_simd_name_t)(void);
    /* Number of SIMD lanes for an element of 'elem_bytes' bytes (1, 2, 4 or 8), that is mipp::N<T>(); this is the
     * number of frames processed together by the INTER (inter-frame SIMD) decoders. Returns 0 on bad size. */
    typedef size_t (*aff3ct_plugin_simd_lanes_t)(size_t elem_bytes);

    /* ---------------------------------------------------------------------------------------------- simulator */
    /* The 'main' function of the AFF3CT simulator. */
    typedef int (*aff3ct_plugin_sim_main_t)(int argc, char** argv);

    /* ------------------------------------------------------------------------------------------------- codecs */
    /* Build a codec from an AFF3CT command line (same arguments as the simulator, without '-C' and '-p').
     *   code: 'TURBO', 'TURBO_DB', 'LDPC', 'RSC', 'RSC_DB', 'BCH', 'RS', 'REP', 'RA', 'CONV', 'TPC', 'UNCODED';
     *   prec: 8, 16, 32 or 64 (bit and LLR types: 8 -> int8/int8, 16 -> int16/int16, 32 -> int32/float,
     *         64 -> int64/double);
     *   argv: 'argc' AFF3CT arguments ("-K", "424", "--enc-sub-std", "LTE", ...), argv[0] is NOT a program name.
     * A puncturer is enabled when one of the arguments starts with '--pct-'. */
    typedef aff3ct_plugin_codec* (*aff3ct_plugin_codec_create_t)(const char* code,
                                                                 int prec,
                                                                 int argc,
                                                                 const char* const* argv,
                                                                 char* err,
                                                                 size_t err_len);
    typedef void (*aff3ct_plugin_codec_destroy_t)(aff3ct_plugin_codec* codec);
    /* K: information bits, N_cw: codeword size, N: frame size after puncturing. */
    typedef void (*aff3ct_plugin_codec_sizes_t)(const aff3ct_plugin_codec* codec, int* K, int* N_cw, int* N);
    /* Returns a 'spu::module::Module*' (as a 'void*'), owned by the codec, or NULL if the codec has no such module.
     * name: 'encoder', 'decoder_siho', 'decoder_siso', 'puncturer', 'extractor'. */
    typedef void* (*aff3ct_plugin_codec_module_t)(aff3ct_plugin_codec* codec,
                                                  const char* name,
                                                  char* err,
                                                  size_t err_len);
    /* Set the number of frames processed by each task call of the codec modules (returns 0 on success). */
    typedef int (*aff3ct_plugin_codec_set_n_frames_t)(aff3ct_plugin_codec* codec,
                                                      size_t n_frames,
                                                      char* err,
                                                      size_t err_len);

#ifndef AFF3CT_PLUGIN_NO_PROTOTYPES
    AFF3CT_PLUGIN_EXPORT int aff3ct_plugin_abi_version(void);
    AFF3CT_PLUGIN_EXPORT const char* aff3ct_plugin_version(void);
    AFF3CT_PLUGIN_EXPORT const char* aff3ct_plugin_isa(void);
    AFF3CT_PLUGIN_EXPORT const char* aff3ct_plugin_simd_name(void);
    AFF3CT_PLUGIN_EXPORT size_t aff3ct_plugin_simd_lanes(size_t elem_bytes);
    AFF3CT_PLUGIN_EXPORT int aff3ct_plugin_sim_main(int argc, char** argv);
    AFF3CT_PLUGIN_EXPORT aff3ct_plugin_codec* aff3ct_plugin_codec_create(const char* code,
                                                                         int prec,
                                                                         int argc,
                                                                         const char* const* argv,
                                                                         char* err,
                                                                         size_t err_len);
    AFF3CT_PLUGIN_EXPORT void aff3ct_plugin_codec_destroy(aff3ct_plugin_codec* codec);
    AFF3CT_PLUGIN_EXPORT void aff3ct_plugin_codec_sizes(const aff3ct_plugin_codec* codec, int* K, int* N_cw, int* N);
    AFF3CT_PLUGIN_EXPORT void* aff3ct_plugin_codec_module(aff3ct_plugin_codec* codec,
                                                          const char* name,
                                                          char* err,
                                                          size_t err_len);
    AFF3CT_PLUGIN_EXPORT int aff3ct_plugin_codec_set_n_frames(aff3ct_plugin_codec* codec,
                                                              size_t n_frames,
                                                              char* err,
                                                              size_t err_len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* AFF3CT_PLUGIN_ABI_H_ */
