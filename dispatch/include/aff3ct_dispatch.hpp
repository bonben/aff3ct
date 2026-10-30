/*!
 * \file
 * \brief SIMD neutral C++ API of AFF3CT (host side of the SIMD dispatch).
 *
 * A program that includes this header is compiled WITHOUT any SIMD option and does not include MIPP. At the first
 * use, the host detects the CPU, chooses the best AFF3CT plugin (or the one imposed by the environment variable
 * 'AFF3CT_SIMD=sse4.2|avx2|avx512|neon') and loads it. The AFF3CT modules are then manipulated through their
 * StreamPU interface ('spu::module::Module', 'spu::runtime::Task' and 'spu::runtime::Socket').
 *
 * Environment variables:
 *   - AFF3CT_SIMD:         imposes a variant (an error is raised if the CPU or the plugin does not support it);
 *   - AFF3CT_PLUGIN_DIR:   directory of the plugins (default: directory of the host library);
 *   - AFF3CT_SIMD_VERBOSE: if set to 1, prints the chosen variant on the standard error.
 */
#ifndef AFF3CT_DISPATCH_HPP_
#define AFF3CT_DISPATCH_HPP_

#include <cstddef>
#include <string>
#include <vector>

#include <streampu.hpp>

#if defined(_WIN32)
#if defined(AFF3CT_DISPATCH_BUILD)
#define AFF3CT_DISPATCH_API __declspec(dllexport)
#else
#define AFF3CT_DISPATCH_API __declspec(dllimport)
#endif
#else
#define AFF3CT_DISPATCH_API __attribute__((visibility("default")))
#endif

struct aff3ct_plugin_codec;

namespace aff3ct
{
namespace dispatch
{
/* Variants known by this host, from the most to the least powerful, for the current architecture. */
AFF3CT_DISPATCH_API std::vector<std::string>
known_variants();

/* Variants whose plugin exists in the plugin directory. */
AFF3CT_DISPATCH_API std::vector<std::string>
available_variants();

/* True if the running CPU (and OS) supports the instruction set of 'variant'. */
AFF3CT_DISPATCH_API bool
cpu_supports(const std::string& variant);

/* Loads the plugin (only one per process: the first call decides) and returns its variant name. */
AFF3CT_DISPATCH_API const std::string&
load();

/* Name of the loaded variant ('sse4.2', 'avx2', 'avx512'...), loads the plugin if needed. */
AFF3CT_DISPATCH_API const std::string&
isa();

/* MIPP instruction set of the loaded plugin ('SSE4.2', 'AVX2', 'AVX512'...). */
AFF3CT_DISPATCH_API std::string
simd_name();

/* Path of the loaded plugin. */
AFF3CT_DISPATCH_API const std::string&
plugin_path();

/* Number of SIMD lanes for an element of 'elem_bytes' bytes: this is the number of frames per wave of the INTER
 * decoders (in int16: 32 in AVX-512, 16 in AVX2, 8 in SSE4.2 or NEON). */
AFF3CT_DISPATCH_API size_t
simd_lanes(size_t elem_bytes);

template<typename T>
inline size_t
simd_lanes()
{
    return simd_lanes(sizeof(T));
}

/* Runs the AFF3CT simulator ('main' of the plugin). */
AFF3CT_DISPATCH_API int
run_simulator(int argc, char** argv);

/* An AFF3CT codec built from a command line, its modules are StreamPU modules owned by the codec. */
class AFF3CT_DISPATCH_API Codec
{
  public:
    /* code: 'TURBO', 'LDPC', 'RSC', 'BCH', 'RS', 'REP', ... (same names as the '-C' argument of the simulator);
     * prec: 8, 16, 32 or 64 (same as the '-p' argument of the simulator);
     * args: the other AFF3CT arguments, for instance { "-K", "424", "--enc-sub-std", "LTE", "-i", "6" }. */
    Codec(const std::string& code, const int prec, const std::vector<std::string>& args);
    /* Same with the arguments in a single string (split on the spaces). */
    Codec(const std::string& code, const int prec, const std::string& args);
    ~Codec();

    Codec(const Codec&) = delete;
    Codec& operator=(const Codec&) = delete;

    int get_K() const;
    int get_N_cw() const;
    int get_N() const;

    spu::module::Module& encoder();
    spu::module::Module& decoder_siho();
    /* Returns nullptr if the codec has no such module ('encoder', 'decoder_siho', 'decoder_siso', 'puncturer',
     * 'extractor'). */
    spu::module::Module* module(const std::string& name);

    /* Number of frames processed by each task call (for an INTER decoder, use a multiple of simd_lanes<Q>()). */
    void set_n_frames(const size_t n_frames);

  private:
    aff3ct_plugin_codec* handle;
    int K, N_cw, N;
};
}
}

#endif /* AFF3CT_DISPATCH_HPP_ */
