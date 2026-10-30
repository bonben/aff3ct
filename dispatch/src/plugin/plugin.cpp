// Entry points of an AFF3CT SIMD plugin (compiled once per instruction set, with the same options as the rest of the
// plugin). See 'aff3ct_plugin_abi.h' for the rules of the frontier.

#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>
#include <mipp.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <streampu.hpp>

#include "Factory/Tools/Codec/BCH/Codec_BCH.hpp"
#include "Factory/Tools/Codec/Conv/Codec_conv.hpp"
#include "Factory/Tools/Codec/LDPC/Codec_LDPC.hpp"
#include "Factory/Tools/Codec/RA/Codec_RA.hpp"
#include "Factory/Tools/Codec/RS/Codec_RS.hpp"
#include "Factory/Tools/Codec/RSC/Codec_RSC.hpp"
#include "Factory/Tools/Codec/RSC_DB/Codec_RSC_DB.hpp"
#include "Factory/Tools/Codec/Repetition/Codec_repetition.hpp"
#include "Factory/Tools/Codec/Turbo/Codec_turbo.hpp"
#include "Factory/Tools/Codec/Turbo_DB/Codec_turbo_DB.hpp"
#include "Factory/Tools/Codec/Turbo_product/Codec_turbo_product.hpp"
#include "Factory/Tools/Codec/Uncoded/Codec_uncoded.hpp"
#include "Tools/Codec/Codec_SISO.hpp"
#include "Tools/Factory/Command_parser.hpp"
#include "Tools/types.h"

#include "aff3ct_plugin_abi.h"

#ifndef AFF3CT_SIMD_VARIANT
#error "AFF3CT_SIMD_VARIANT has to be defined (name of the SIMD variant of this plugin)."
#endif
#ifndef AFF3CT_DISPATCH_VERSION
#error "AFF3CT_DISPATCH_VERSION has to be defined."
#endif

int
aff3ct_sim_main(int argc, char** argv); // defined in 'src/main.cpp'

using namespace aff3ct;

namespace
{
void
set_error(char* err, size_t err_len, const std::string& msg)
{
    if (err != nullptr && err_len > 0) std::snprintf(err, err_len, "%s", msg.c_str());
}

// --------------------------------------------------------------------------------------------- type erased codec
struct Codec_holder
{
    virtual ~Codec_holder() = default;
    virtual spu::module::Module* module(const std::string& name) = 0;
    virtual void set_n_frames(const size_t n_frames) = 0;
};

template<typename B, typename Q>
struct Codec_holder_t : public Codec_holder
{
    std::unique_ptr<tools::Codec_SIHO<B, Q>> codec;
    const factory::Codec* params;

    Codec_holder_t(tools::Codec_SIHO<B, Q>* codec, const factory::Codec* params)
      : codec(codec)
      , params(params)
    {
        this->reseed();
    }

    // same as the simulator ('Simulation_BFER_std'): set_seed() generates the interleaver lookup table
    void reseed()
    {
        if (this->params->itl != nullptr) this->codec->get_interleaver().set_seed(this->params->itl->core->seed);
    }

    static spu::module::Module* as_module(spu::module::Module& m) { return &m; }

    spu::module::Module* module(const std::string& name)
    {
        if (name == "encoder") return as_module(this->codec->get_encoder());
        if (name == "decoder_siho") return as_module(this->codec->get_decoder_siho());
        if (name == "decoder_siso")
        {
            auto siso = dynamic_cast<tools::Codec_SISO<B, Q>*>(this->codec.get());
            return siso ? as_module(siso->get_decoder_siso()) : nullptr;
        }
        if (name == "puncturer") return as_module(this->codec->get_puncturer());
        if (name == "extractor") return as_module(this->codec->get_extractor());
        throw std::invalid_argument("unknown codec module '" + name +
                                    "' (expected 'encoder', 'decoder_siho', 'decoder_siso', 'puncturer' or "
                                    "'extractor').");
    }

    void set_n_frames(const size_t n_frames)
    {
        this->codec->set_n_frames(n_frames);
        // Interleaver_core::set_n_frames() regenerates the lookup table without resetting the seed
        this->reseed();
    }
};

// -------------------------------------------------------------------------------------- codec factories by name
struct Codec_entry
{
    const char* name;
    factory::Codec* (*make)();
    Codec_holder* (*build[4])(const factory::Codec&); // 8, 16, 32 and 64-bit
};

template<class F>
factory::Codec*
make_factory()
{
    return new F();
}

template<class F, typename B, typename Q>
Codec_holder*
build_codec(const factory::Codec& params)
{
    return new Codec_holder_t<B, Q>(static_cast<const F&>(params).template build<B, Q>(), &params);
}

#ifdef AFF3CT_MULTI_PREC
#define AFF3CT_CODEC_ENTRY(NAME, F)                                                                                    \
    {                                                                                                                  \
        NAME, &make_factory<F>,                                                                                        \
        {                                                                                                              \
            &build_codec<F, B_8, Q_8>, &build_codec<F, B_16, Q_16>, &build_codec<F, B_32, Q_32>,                       \
              &build_codec<F, B_64, Q_64>                                                                              \
        }                                                                                                              \
    }
#else
#define AFF3CT_CODEC_ENTRY(NAME, F)                                                                                    \
    {                                                                                                                  \
        NAME, &make_factory<F>,                                                                                        \
        {                                                                                                              \
            &build_codec<F, B, Q>, &build_codec<F, B, Q>, &build_codec<F, B, Q>, &build_codec<F, B, Q>                 \
        }                                                                                                              \
    }
#endif

// Polar codes are not listed: their frozen bits have to be generated from a noise (simulator only for now).
const Codec_entry codec_entries[] = {
    AFF3CT_CODEC_ENTRY("TURBO", factory::Codec_turbo),
    AFF3CT_CODEC_ENTRY("TURBO_DB", factory::Codec_turbo_DB),
    AFF3CT_CODEC_ENTRY("TPC", factory::Codec_turbo_product),
    AFF3CT_CODEC_ENTRY("LDPC", factory::Codec_LDPC),
    AFF3CT_CODEC_ENTRY("RSC", factory::Codec_RSC),
    AFF3CT_CODEC_ENTRY("RSC_DB", factory::Codec_RSC_DB),
    AFF3CT_CODEC_ENTRY("BCH", factory::Codec_BCH),
    AFF3CT_CODEC_ENTRY("RS", factory::Codec_RS),
    AFF3CT_CODEC_ENTRY("REP", factory::Codec_repetition),
    AFF3CT_CODEC_ENTRY("RA", factory::Codec_RA),
    AFF3CT_CODEC_ENTRY("CONV", factory::Codec_conv),
    AFF3CT_CODEC_ENTRY("UNCODED", factory::Codec_uncoded),
};
} // namespace

struct aff3ct_plugin_codec
{
    std::unique_ptr<factory::Codec> params;
    std::unique_ptr<Codec_holder> holder;
};

extern "C"
{
    int aff3ct_plugin_abi_version(void)
    {
        return AFF3CT_PLUGIN_ABI_VERSION;
    }

    const char* aff3ct_plugin_version(void)
    {
        return AFF3CT_DISPATCH_VERSION;
    }

    const char* aff3ct_plugin_isa(void)
    {
        return AFF3CT_SIMD_VARIANT;
    }

    const char* aff3ct_plugin_simd_name(void)
    {
        return mipp::InstructionFullType.c_str();
    }

    size_t aff3ct_plugin_simd_lanes(size_t elem_bytes)
    {
        switch (elem_bytes)
        {
            case 1:
                return (size_t)mipp::N<int8_t>();
            case 2:
                return (size_t)mipp::N<int16_t>();
            case 4:
                return (size_t)mipp::N<int32_t>();
            case 8:
                return (size_t)mipp::N<int64_t>();
            default:
                return 0;
        }
    }

    int aff3ct_plugin_sim_main(int argc, char** argv)
    {
        try
        {
            return aff3ct_sim_main(argc, argv);
        }
        catch (std::exception const& e)
        {
            std::fprintf(stderr, "%s\n", e.what());
            return EXIT_FAILURE;
        }
    }

    aff3ct_plugin_codec* aff3ct_plugin_codec_create(const char* code,
                                                    int prec,
                                                    int argc,
                                                    const char* const* argv,
                                                    char* err,
                                                    size_t err_len)
    {
        try
        {
            if (code == nullptr) throw std::invalid_argument("'code' is NULL.");

            const Codec_entry* entry = nullptr;
            for (auto& e : codec_entries)
                if (std::string(e.name) == code) entry = &e;
            if (entry == nullptr)
            {
                std::string list;
                for (auto& e : codec_entries)
                    list += std::string(list.empty() ? "" : ", ") + e.name;
                throw std::invalid_argument("unsupported code type '" + std::string(code) + "' (supported: " + list +
                                            ").");
            }

            int prec_id;
            switch (prec)
            {
                case 8:
                    prec_id = 0;
                    break;
                case 16:
                    prec_id = 1;
                    break;
                case 32:
                    prec_id = 2;
                    break;
                case 64:
                    prec_id = 3;
                    break;
                default:
                    throw std::invalid_argument("'prec' has to be 8, 16, 32 or 64 ('prec' = " + std::to_string(prec) +
                                                ").");
            }

            std::unique_ptr<aff3ct_plugin_codec> c(new aff3ct_plugin_codec());
            c->params.reset(entry->make());

            std::vector<std::string> args = { "aff3ct" };
            for (int a = 0; a < argc; a++)
                args.push_back(argv[a] ? argv[a] : "");
            for (auto& a : args)
                if (a.compare(0, 6, "--pct-") == 0)
                {
                    c->params->enable_puncturer();
                    break;
                }

            std::vector<char*> cargv;
            for (auto& a : args)
                cargv.push_back(const_cast<char*>(a.c_str()));
            cargv.push_back(nullptr);

            std::vector<factory::Factory*> factories = { c->params.get() };
            std::ostringstream err_stream;
            tools::Command_parser cp((int)args.size(), cargv.data(), factories, false, err_stream);
            if (cp.parsing_failed())
            {
                cp.print_errors();
                throw std::invalid_argument("invalid AFF3CT arguments:\n" + err_stream.str());
            }

            c->holder.reset(entry->build[prec_id](*c->params));
            return c.release();
        }
        catch (std::exception const& e)
        {
            set_error(err, err_len, e.what());
            return nullptr;
        }
    }

    void aff3ct_plugin_codec_destroy(aff3ct_plugin_codec* codec)
    {
        delete codec;
    }

    void aff3ct_plugin_codec_sizes(const aff3ct_plugin_codec* codec, int* K, int* N_cw, int* N)
    {
        if (K) *K = codec->params->K;
        if (N_cw) *N_cw = codec->params->N_cw;
        if (N) *N = codec->params->N;
    }

    void* aff3ct_plugin_codec_module(aff3ct_plugin_codec* codec, const char* name, char* err, size_t err_len)
    {
        try
        {
            return static_cast<void*>(codec->holder->module(name ? name : ""));
        }
        catch (std::exception const& e)
        {
            set_error(err, err_len, e.what());
            return nullptr;
        }
    }

    int aff3ct_plugin_codec_set_n_frames(aff3ct_plugin_codec* codec, size_t n_frames, char* err, size_t err_len)
    {
        try
        {
            codec->holder->set_n_frames(n_frames);
            return 0;
        }
        catch (std::exception const& e)
        {
            set_error(err, err_len, e.what());
            return 1;
        }
    }
}
