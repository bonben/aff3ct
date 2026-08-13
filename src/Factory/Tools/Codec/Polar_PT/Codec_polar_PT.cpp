#include "Factory/Tools/Codec/Polar_PT/Codec_polar_PT.hpp"
#include "Factory/Module/Decoder/Polar_PT/Decoder_polar_PT.hpp"
#include "Factory/Module/Encoder/Polar_PT/Encoder_polar_PT.hpp"
#include "Factory/Module/Puncturer/Polar/Puncturer_polar.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Codec_polar_PT_name = "Codec Polar PT";
const std::string aff3ct::factory::Codec_polar_PT_prefix = "cdc";

Codec_polar_PT ::Codec_polar_PT(const std::string& prefix)
  : Codec_SISO(Codec_polar_PT_name, prefix)
{
    Codec::set_enc(new Encoder_polar_PT("enc"));
    Codec::set_dec(new Decoder_polar_PT("dec"));

    fbg = new Frozenbits_generator(enc->get_prefix() + "-fb");
    ptg = new Pretransform_generator(enc->get_prefix() + "-pt");
    fbg->type = "FILE"; // set FILE frozen bit by default in Polar PT
    ptg->type = "FILE"; // set the PT generator from file
}

Codec_polar_PT*
Codec_polar_PT ::clone() const
{
    return new Codec_polar_PT(*this);
}

std::vector<std::string>
Codec_polar_PT ::get_names() const
{
    auto n = Codec::get_names();
    std::vector<std::string> n2;

    for (size_t i = 0; i < n.size(); i++)
    {
        n2.push_back(n[i]);
        if (enc != nullptr && n[i] == enc->get_name() && fbg != nullptr)
        {
            auto nn = fbg->get_names();
            for (auto& x : nn)
                n2.push_back(x);
        }

        if (enc != nullptr && n[i] == enc->get_name() && ptg != nullptr)
        {
            auto nn = ptg->get_names();
            for (auto& x : nn)
                n2.push_back(x);
        }
    }

    return n2;
}

std::vector<std::string>
Codec_polar_PT ::get_short_names() const
{
    auto sn = Codec::get_short_names();
    std::vector<std::string> sn2;

    for (size_t i = 0; i < sn.size(); i++)
    {
        sn2.push_back(sn[i]);
        if (enc != nullptr && sn[i] == enc->get_short_name() && fbg != nullptr)
        {
            auto nn = fbg->get_short_names();
            for (auto& x : nn)
                sn2.push_back(x);
        }

        if (enc != nullptr && sn[i] == enc->get_short_name() && ptg != nullptr)
        {
            auto nn = ptg->get_short_names();
            for (auto& x : nn)
                sn2.push_back(x);
        }
    }

    return sn2;
}

std::vector<std::string>
Codec_polar_PT ::get_prefixes() const
{
    auto p = Codec::get_prefixes();
    std::vector<std::string> p2;

    for (size_t i = 0; i < p.size(); i++)
    {
        p2.push_back(p[i]);
        if (enc != nullptr && p[i] == enc->get_prefix() && fbg != nullptr)
        {
            auto nn = fbg->get_prefixes();
            for (auto& x : nn)
                p2.push_back(x);
        }

        if (enc != nullptr && p[i] == enc->get_prefix() && ptg != nullptr)
        {
            auto nn = ptg->get_prefixes();
            for (auto& x : nn)
                p2.push_back(x);
        }
    }

    return p2;
}

void
Codec_polar_PT ::get_description(cli::Argument_map_info& args) const
{
    Codec_SISO::get_description(args);

    enc->get_description(args);
    fbg->get_description(args);
    dec->get_description(args);
    ptg->get_description(args);

    auto pdec = dec->get_prefix();
    auto pfbg = fbg->get_prefix();

    args.erase({ pdec + "-info-bits", "K" });
    args.erase({ pdec + "-no-sys" });
    args.erase({ pdec + "-cw-size", "N" });
    args.erase({ pfbg + "-cw-size", "N" });
    args.erase({ pfbg + "-info-bits", "K" });
}

void
Codec_polar_PT ::store(const cli::Argument_map_value& vals)
{
    Codec_SISO::store(vals);

    enc->store(vals);

    ptg->K = dec->K = fbg->K = enc->K;
    ptg->N_cw = dec->N_cw = fbg->N_cw = enc->N_cw;

    fbg->store(vals);
    ptg->store(vals);

    dec->systematic = enc->systematic;

    auto enc_pac = dynamic_cast<Encoder_polar_PT*>(enc.get());
    auto dec_pac = dynamic_cast<Decoder_polar_PT*>(dec.get());
    dec_pac->conv = enc_pac->conv;
    dec_pac->pt_path = enc_pac->pt_path;

    dec->store(vals);

    K = enc->K;
    N_cw = enc->N_cw;
    N = enc->N_cw;
}

void
Codec_polar_PT ::get_headers(std::map<std::string, tools::header_list>& headers, const bool full) const
{
    Codec_SISO::get_headers(headers, full);

    enc->get_headers(headers, full);
    fbg->get_headers(headers, full);
    ptg->get_headers(headers, full);
    dec->get_headers(headers, full);
}

template<typename B, typename Q>
tools::Codec_polar_PT<B, Q>*
Codec_polar_PT ::build(const module::CRC<B>* crc) const
{
    return new tools::Codec_polar_PT<B, Q>(
      *ptg, *fbg, dynamic_cast<const Encoder_polar_PT&>(*enc), dynamic_cast<const Decoder_polar_PT&>(*dec), crc);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template aff3ct::tools::Codec_polar_PT<B_8, Q_8>*
aff3ct::factory::Codec_polar_PT::build<B_8, Q_8>(const aff3ct::module::CRC<B_8>*) const;
template aff3ct::tools::Codec_polar_PT<B_16, Q_16>*
aff3ct::factory::Codec_polar_PT::build<B_16, Q_16>(const aff3ct::module::CRC<B_16>*) const;
template aff3ct::tools::Codec_polar_PT<B_32, Q_32>*
aff3ct::factory::Codec_polar_PT::build<B_32, Q_32>(const aff3ct::module::CRC<B_32>*) const;
template aff3ct::tools::Codec_polar_PT<B_64, Q_64>*
aff3ct::factory::Codec_polar_PT::build<B_64, Q_64>(const aff3ct::module::CRC<B_64>*) const;
#else
template aff3ct::tools::Codec_polar_PT<B, Q>*
aff3ct::factory::Codec_polar_PT::build<B, Q>(const aff3ct::module::CRC<B>*) const;
#endif
// ==================================================================================== explicit template instantiation
