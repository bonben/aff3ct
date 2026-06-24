#include "Factory/Tools/Codec/Conv/Codec_conv.hpp"
#include "Factory/Module/Decoder/Conv/Decoder_conv.hpp"
#include "Factory/Module/Encoder/Conv/Encoder_conv.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Codec_conv_name = "Codec CONV";
const std::string aff3ct::factory::Codec_conv_prefix = "cdc";

Codec_conv ::Codec_conv(const std::string& prefix)
  : Codec_SIHO(Codec_conv_name, prefix)
{
    Codec::set_enc(new Encoder_conv("enc"));
    Codec::set_dec(new Decoder_conv("dec"));
}

Codec_conv*
Codec_conv ::clone() const
{
    return new Codec_conv(*this);
}

void
Codec_conv ::get_description(cli::Argument_map_info& args) const
{
    Codec_SIHO::get_description(args);

    enc->get_description(args);
    dec->get_description(args);

    auto pdec = dec->get_prefix();

    args.erase({ pdec + "-cw-size", "N" });
    args.erase({ pdec + "-info-bits", "K" });
    args.erase({ pdec + "-poly" });
}

void
Codec_conv ::store(const cli::Argument_map_value& vals)
{
    Codec_SIHO::store(vals);

    auto enc_conv = dynamic_cast<Encoder_conv*>(enc.get());
    auto dec_conv = dynamic_cast<Decoder_conv*>(dec.get());

    enc->store(vals);

    dec_conv->K = enc_conv->K;
    dec_conv->N_cw = enc_conv->N_cw;
    dec_conv->poly = enc_conv->poly;

    dec->store(vals);

    K = enc->K;
    N_cw = enc->N_cw;
    N = enc->N_cw;
    tail_length = enc->tail_length;
}

void
Codec_conv ::get_headers(std::map<std::string, tools::header_list>& headers, const bool full) const
{
    Codec_SIHO::get_headers(headers, full);

    enc->get_headers(headers, full);
    dec->get_headers(headers, full);
}

template<typename B, typename Q>
tools::Codec_conv<B, Q>*
Codec_conv ::build(const module::CRC<B>* crc) const
{
    return new tools::Codec_conv<B, Q>(
      dynamic_cast<const Encoder_conv&>(*enc), dynamic_cast<const Decoder_conv&>(*dec), crc);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template aff3ct::tools::Codec_conv<B_8, Q_8>*
aff3ct::factory::Codec_conv::build<B_8, Q_8>(const aff3ct::module::CRC<B_8>*) const;
template aff3ct::tools::Codec_conv<B_16, Q_16>*
aff3ct::factory::Codec_conv::build<B_16, Q_16>(const aff3ct::module::CRC<B_16>*) const;
template aff3ct::tools::Codec_conv<B_32, Q_32>*
aff3ct::factory::Codec_conv::build<B_32, Q_32>(const aff3ct::module::CRC<B_32>*) const;
template aff3ct::tools::Codec_conv<B_64, Q_64>*
aff3ct::factory::Codec_conv::build<B_64, Q_64>(const aff3ct::module::CRC<B_64>*) const;
#else
template aff3ct::tools::Codec_conv<B, Q>*
aff3ct::factory::Codec_conv::build<B, Q>(const aff3ct::module::CRC<B>*) const;
#endif
// ==================================================================================== explicit template instantiation
