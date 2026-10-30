// Host side of the AFF3CT SIMD dispatch: CPU detection, plugin loading and SIMD neutral C++ API.
// This file is compiled WITHOUT any SIMD option and does not include MIPP.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/stat.h>
#endif

#if (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86)))
#include <immintrin.h> // _xgetbv only, no vector code is generated
#include <intrin.h>
#endif

#define AFF3CT_PLUGIN_NO_PROTOTYPES
#include "aff3ct_plugin_abi.h"

#include "aff3ct_dispatch.hpp"

#ifndef AFF3CT_DISPATCH_VERSION
#error "AFF3CT_DISPATCH_VERSION has to be defined."
#endif

namespace aff3ct
{
namespace dispatch
{
namespace
{
// ------------------------------------------------------------------------------------------------ CPU detection
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define AFF3CT_DISPATCH_X86
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__)
#define AFF3CT_DISPATCH_ARM
#endif

#if defined(AFF3CT_DISPATCH_X86) && defined(_MSC_VER) && !defined(__clang__)
// MSVC: cpuid + xgetbv (not tested yet)
struct Cpu_features
{
    bool sse42 = false, popcnt = false, avx2 = false, fma = false, bmi2 = false, avx512 = false;
    Cpu_features()
    {
        int r[4];
        __cpuid(r, 0);
        const int max_leaf = r[0];
        __cpuid(r, 1);
        const bool osxsave = (r[2] >> 27) & 1, avx = (r[2] >> 28) & 1;
        sse42 = (r[2] >> 20) & 1;
        popcnt = (r[2] >> 23) & 1;
        fma = (r[2] >> 12) & 1;
        unsigned long long xcr0 = osxsave ? _xgetbv(0) : 0;
        const bool os_avx = (xcr0 & 0x6) == 0x6, os_avx512 = (xcr0 & 0xe6) == 0xe6;
        if (max_leaf >= 7)
        {
            __cpuidex(r, 7, 0);
            avx2 = avx && os_avx && ((r[1] >> 5) & 1);
            bmi2 = (r[1] >> 8) & 1;
            // AVX512F, DQ, CD, BW, VL
            avx512 = os_avx512 && ((r[1] >> 16) & 1) && ((r[1] >> 17) & 1) && ((r[1] >> 28) & 1) &&
                     ((r[1] >> 30) & 1) && ((r[1] >> 31) & 1);
        }
    }
};
#endif

bool
cpu_supports_impl(const std::string& v)
{
#if defined(AFF3CT_DISPATCH_X86)
#if defined(_MSC_VER) && !defined(__clang__)
    static const Cpu_features f;
    if (v == "sse2") return true;
    if (v == "sse4.2") return f.sse42 && f.popcnt;
    if (v == "avx2") return f.sse42 && f.popcnt && f.avx2 && f.fma && f.bmi2;
    if (v == "avx512") return f.sse42 && f.popcnt && f.avx2 && f.fma && f.bmi2 && f.avx512;
    return false;
#else
    // the variants are compiled with '-march=x86-64-v2|v3|v4' (see CMakeLists.txt), the same levels are checked here
    // (__builtin_cpu_supports also checks that the OS saves the AVX and AVX-512 registers)
    __builtin_cpu_init();
#if (defined(__clang__) && __clang_major__ >= 16) || (!defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 12)
    if (v == "sse2") return true;
    if (v == "sse4.2") return __builtin_cpu_supports("x86-64-v2");
    if (v == "avx2") return __builtin_cpu_supports("x86-64-v3");
    if (v == "avx512") return __builtin_cpu_supports("x86-64-v4");
#else
    const bool v2 = __builtin_cpu_supports("sse4.2") && __builtin_cpu_supports("popcnt") &&
                    __builtin_cpu_supports("ssse3");
    const bool v3 = v2 && __builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma") &&
                    __builtin_cpu_supports("bmi") && __builtin_cpu_supports("bmi2");
    const bool v4 = v3 && __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") &&
                    __builtin_cpu_supports("avx512cd") && __builtin_cpu_supports("avx512dq") &&
                    __builtin_cpu_supports("avx512vl");
    if (v == "sse2") return true;
    if (v == "sse4.2") return v2;
    if (v == "avx2") return v3;
    if (v == "avx512") return v4;
#endif
    return false;
#endif
#elif defined(AFF3CT_DISPATCH_ARM)
#if defined(__aarch64__) || defined(_M_ARM64)
    return v == "neon"; // NEON (ASIMD) is mandatory in ARMv8-A
#else
    return false;
#endif
#else
    (void)v;
    return false;
#endif
}

// ------------------------------------------------------------------------------------------------ dynamic loading
std::string
plugin_file_name(const std::string& variant)
{
#if defined(_WIN32)
    return "aff3ct-" AFF3CT_DISPATCH_VERSION "-" + variant + ".dll";
#elif defined(__APPLE__)
    return "libaff3ct-" AFF3CT_DISPATCH_VERSION "-" + variant + ".so"; // CMake MODULE libraries end in '.so' on macOS
#else
    return "libaff3ct-" AFF3CT_DISPATCH_VERSION "-" + variant + ".so";
#endif
}

bool
file_exists(const std::string& path)
{
#if defined(_WIN32)
    const DWORD a = GetFileAttributesA(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
#endif
}

// directory of the current shared object (the host library), the plugins are installed next to it
std::string
plugin_directory()
{
    const char* env = std::getenv("AFF3CT_PLUGIN_DIR");
    if (env != nullptr && env[0] != '\0') return std::string(env);
#if defined(_WIN32)
    HMODULE hm = nullptr;
    char path[MAX_PATH];
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&plugin_directory,
                           &hm) &&
        GetModuleFileNameA(hm, path, sizeof(path)))
    {
        std::string p(path);
        return p.substr(0, p.find_last_of("\\/"));
    }
    return ".";
#else
    Dl_info info;
    if (dladdr((void*)&plugin_directory, &info) && info.dli_fname != nullptr)
    {
        std::string p(info.dli_fname);
        const auto pos = p.find_last_of('/');
        return pos == std::string::npos ? std::string(".") : p.substr(0, pos);
    }
    return ".";
#endif
}

struct Plugin
{
    std::string variant;
    std::string path;
    void* handle = nullptr;

    aff3ct_plugin_abi_version_t abi_version = nullptr;
    aff3ct_plugin_version_t version = nullptr;
    aff3ct_plugin_isa_t isa = nullptr;
    aff3ct_plugin_simd_name_t simd_name = nullptr;
    aff3ct_plugin_simd_lanes_t simd_lanes = nullptr;
    aff3ct_plugin_sim_main_t sim_main = nullptr;
    aff3ct_plugin_codec_create_t codec_create = nullptr;
    aff3ct_plugin_codec_destroy_t codec_destroy = nullptr;
    aff3ct_plugin_codec_sizes_t codec_sizes = nullptr;
    aff3ct_plugin_codec_module_t codec_module = nullptr;
    aff3ct_plugin_codec_set_n_frames_t codec_set_n_frames = nullptr;

    void* sym(const char* name)
    {
#if defined(_WIN32)
        void* s = (void*)GetProcAddress((HMODULE)this->handle, name);
#else
        void* s = dlsym(this->handle, name);
#endif
        if (s == nullptr)
            throw std::runtime_error("aff3ct::dispatch: symbol '" + std::string(name) + "' not found in '" +
                                     this->path + "'.");
        return s;
    }

    template<typename F>
    void bind(F& f, const char* name)
    {
        f = reinterpret_cast<F>(this->sym(name));
    }

    void open(const std::string& variant, const std::string& path)
    {
        this->variant = variant;
        this->path = path;
#if defined(_WIN32)
        this->handle = (void*)LoadLibraryA(path.c_str());
        if (this->handle == nullptr)
            throw std::runtime_error("aff3ct::dispatch: LoadLibrary('" + path +
                                     "') failed (error = " + std::to_string(GetLastError()) + ").");
#else
        // RTLD_LOCAL: the symbols of the plugin are not visible from the other libraries (and the plugin exports
        // only its 'aff3ct_plugin_*' entry points anyway)
        this->handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (this->handle == nullptr)
        {
            const char* e = dlerror();
            throw std::runtime_error("aff3ct::dispatch: dlopen('" + path + "') failed: " + (e ? e : "?"));
        }
#endif
        this->bind(this->abi_version, "aff3ct_plugin_abi_version");
        if (this->abi_version() != AFF3CT_PLUGIN_ABI_VERSION)
            throw std::runtime_error("aff3ct::dispatch: '" + path + "' has the ABI version " +
                                     std::to_string(this->abi_version()) + " while the host expects " +
                                     std::to_string(AFF3CT_PLUGIN_ABI_VERSION) + ".");
        this->bind(this->version, "aff3ct_plugin_version");
        if (std::string(this->version()) != AFF3CT_DISPATCH_VERSION)
            throw std::runtime_error("aff3ct::dispatch: '" + path + "' is AFF3CT " + this->version() +
                                     " while the host is AFF3CT " AFF3CT_DISPATCH_VERSION ".");
        this->bind(this->isa, "aff3ct_plugin_isa");
        this->bind(this->simd_name, "aff3ct_plugin_simd_name");
        this->bind(this->simd_lanes, "aff3ct_plugin_simd_lanes");
        this->bind(this->sim_main, "aff3ct_plugin_sim_main");
        this->bind(this->codec_create, "aff3ct_plugin_codec_create");
        this->bind(this->codec_destroy, "aff3ct_plugin_codec_destroy");
        this->bind(this->codec_sizes, "aff3ct_plugin_codec_sizes");
        this->bind(this->codec_module, "aff3ct_plugin_codec_module");
        this->bind(this->codec_set_n_frames, "aff3ct_plugin_codec_set_n_frames");
    }
    // the plugin is never closed: modules created by it may be alive until the end of the process
};

std::once_flag plugin_once;
Plugin* the_plugin = nullptr;

std::string
join(const std::vector<std::string>& v)
{
    std::string s;
    for (auto& e : v)
        s += (s.empty() ? "" : ", ") + e;
    return s.empty() ? std::string("none") : s;
}

void
do_load()
{
    const std::string dir = plugin_directory();
    const char* forced = std::getenv("AFF3CT_SIMD");

    std::string variant;
    if (forced != nullptr && forced[0] != '\0')
    {
        variant = forced;
        bool known = false;
        for (auto& v : known_variants())
            known |= v == variant;
        if (!known)
            throw std::runtime_error("aff3ct::dispatch: AFF3CT_SIMD='" + variant + "' is unknown (known: " +
                                     join(known_variants()) + ").");
        if (!cpu_supports_impl(variant))
            throw std::runtime_error("aff3ct::dispatch: AFF3CT_SIMD='" + variant +
                                     "' is not supported by this CPU (supported and available: " +
                                     join(available_variants()) + ").");
        if (!file_exists(dir + "/" + plugin_file_name(variant)))
            throw std::runtime_error("aff3ct::dispatch: AFF3CT_SIMD='" + variant + "' but '" + dir + "/" +
                                     plugin_file_name(variant) + "' does not exist.");
    }
    else
    {
        for (auto& v : known_variants())
            if (cpu_supports_impl(v) && file_exists(dir + "/" + plugin_file_name(v)))
            {
                variant = v;
                break;
            }
        if (variant.empty())
            throw std::runtime_error("aff3ct::dispatch: no AFF3CT plugin usable on this CPU in '" + dir +
                                     "' (known variants: " + join(known_variants()) + ").");
    }

    std::unique_ptr<Plugin> p(new Plugin());
    p->open(variant, dir + "/" + plugin_file_name(variant));

    const char* verbose = std::getenv("AFF3CT_SIMD_VERBOSE");
    if (verbose != nullptr && std::string(verbose) == "1")
        std::cerr << "aff3ct::dispatch: variant '" << p->isa() << "' (" << p->simd_name() << "), '" << p->path << "'"
                  << (forced ? " [forced by AFF3CT_SIMD]" : "") << std::endl;

    the_plugin = p.release();
}

Plugin&
plugin()
{
    std::call_once(plugin_once, do_load); // if do_load throws, the next call tries again
    return *the_plugin;
}

std::vector<std::string>
split(const std::string& s)
{
    std::vector<std::string> words;
    std::istringstream iss(s);
    std::string w;
    while (iss >> w)
        words.push_back(w);
    return words;
}
} // namespace

std::vector<std::string>
known_variants()
{
#if defined(AFF3CT_DISPATCH_X86)
    return { "avx512", "avx2", "sse4.2", "sse2" };
#elif defined(AFF3CT_DISPATCH_ARM)
    return { "neon" };
#else
    return {};
#endif
}

std::vector<std::string>
available_variants()
{
    const std::string dir = plugin_directory();
    std::vector<std::string> list;
    for (auto& v : known_variants())
        if (cpu_supports_impl(v) && file_exists(dir + "/" + plugin_file_name(v))) list.push_back(v);
    return list;
}

bool
cpu_supports(const std::string& variant)
{
    return cpu_supports_impl(variant);
}

const std::string&
load()
{
    return plugin().variant;
}

const std::string&
isa()
{
    return plugin().variant;
}

std::string
simd_name()
{
    return plugin().simd_name();
}

const std::string&
plugin_path()
{
    return plugin().path;
}

size_t
simd_lanes(size_t elem_bytes)
{
    return plugin().simd_lanes(elem_bytes);
}

int
run_simulator(int argc, char** argv)
{
    return plugin().sim_main(argc, argv);
}

// ------------------------------------------------------------------------------------------------------- Codec
Codec::Codec(const std::string& code, const int prec, const std::vector<std::string>& args)
  : handle(nullptr)
  , K(0)
  , N_cw(0)
  , N(0)
{
    auto& p = plugin();
    std::vector<const char*> argv;
    for (auto& a : args)
        argv.push_back(a.c_str());
    char err[4096] = { 0 };
    this->handle = p.codec_create(code.c_str(), prec, (int)argv.size(), argv.data(), err, sizeof(err));
    if (this->handle == nullptr) throw std::runtime_error("aff3ct::dispatch::Codec: " + std::string(err));
    p.codec_sizes(this->handle, &this->K, &this->N_cw, &this->N);
}

Codec::Codec(const std::string& code, const int prec, const std::string& args)
  : Codec(code, prec, split(args))
{
}

Codec::~Codec()
{
    if (this->handle != nullptr) plugin().codec_destroy(this->handle);
}

int
Codec::get_K() const
{
    return this->K;
}

int
Codec::get_N_cw() const
{
    return this->N_cw;
}

int
Codec::get_N() const
{
    return this->N;
}

spu::module::Module*
Codec::module(const std::string& name)
{
    char err[4096] = { 0 };
    void* m = plugin().codec_module(this->handle, name.c_str(), err, sizeof(err));
    return static_cast<spu::module::Module*>(m);
}

spu::module::Module&
Codec::encoder()
{
    auto m = this->module("encoder");
    if (m == nullptr) throw std::runtime_error("aff3ct::dispatch::Codec: no encoder.");
    return *m;
}

spu::module::Module&
Codec::decoder_siho()
{
    auto m = this->module("decoder_siho");
    if (m == nullptr) throw std::runtime_error("aff3ct::dispatch::Codec: no SIHO decoder.");
    return *m;
}

void
Codec::set_n_frames(const size_t n_frames)
{
    char err[4096] = { 0 };
    if (plugin().codec_set_n_frames(this->handle, n_frames, err, sizeof(err)))
        throw std::runtime_error("aff3ct::dispatch::Codec::set_n_frames: " + std::string(err));
}
}
}
