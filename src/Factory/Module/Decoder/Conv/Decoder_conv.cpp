#include <cmath>
#include <cstdio>
#include <streampu.hpp>
#include <utility>

#include "Factory/Module/Decoder/Conv/Decoder_conv.hpp"
#include "Module/Decoder/Conv/Viterbi/Decoder_Viterbi_SIHO.hpp"
#include "Module/Decoder/Conv/Viterbi_list/Decoder_Viterbi_list_parallel.hpp"
#include "Tools/Documentation/documentation.h"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Decoder_conv_name = "Decoder CONV";
const std::string aff3ct::factory::Decoder_conv_prefix = "dec";

Decoder_conv ::Decoder_conv(const std::string& prefix)
  : Decoder(Decoder_conv_name, prefix)
{
    this->type = "VITERBI";
    this->implem = "STD";
    this->systematic = false;
}

Decoder_conv*
Decoder_conv ::clone() const
{
    return new Decoder_conv(*this);
}

void
Decoder_conv ::get_description(cli::Argument_map_info& args) const
{
    Decoder::get_description(args);

    auto p = this->get_prefix();
    const std::string class_name = "factory::Decoder_conv::";

    args.erase({ p + "-cw-size", "N" });

    cli::add_options(args.at({ p + "-type", "D" }), 0, "VITERBI", "PLVA");
    cli::add_options(args.at({ p + "-implem" }), 0, "STD");

    tools::add_arg(args, p, class_name + "p+poly", cli::Text());

    tools::add_arg(args, p, class_name + "p+lists,L", cli::Integer(cli::Positive(), cli::Non_zero()));
}

void
Decoder_conv ::store(const cli::Argument_map_value& vals)
{
    Decoder::store(vals);

    auto p = this->get_prefix();

    if (vals.exist({ p + "-type", "D" })) this->type = vals.at({ p + "-type", "D" });
    if (vals.exist({ p + "-lists", "L" })) this->L = vals.to_int({ p + "-lists", "L" });

    if (vals.exist({ p + "-poly" }))
    {
        auto poly_str = vals.at({ p + "-poly" });

#ifdef _MSC_VER
        sscanf_s(poly_str.c_str(), "{%o,%o}", &this->poly[0], &this->poly[1]);
#else
        std::sscanf(poly_str.c_str(), "{%o,%o}", &this->poly[0], &this->poly[1]);
#endif
    }

    const int n_ff = (int)std::floor(std::log2((float)std::max(this->poly[0], this->poly[1])));
    this->tail_length = 2 * n_ff;
    this->N_cw = 2 * this->K + this->tail_length;
    this->R = (float)this->K / (float)this->N_cw;
}

void
Decoder_conv ::get_headers(std::map<std::string, tools::header_list>& headers, const bool full) const
{
    Decoder::get_headers(headers, full);

    if (this->type != "ML" && this->type != "CHASE")
    {
        auto p = this->get_prefix();

        if (this->tail_length && full)
            headers[p].push_back(std::make_pair("Tail length", std::to_string(this->tail_length)));

        std::stringstream poly;
        poly << "{0" << std::oct << this->poly[0] << ",0" << std::oct << this->poly[1] << "}";
        headers[p].push_back(std::make_pair(std::string("Polynomials"), poly.str()));

        if (this->type == "PLVA") headers[p].push_back(std::make_pair("Num. of lists (L)", std::to_string(this->L)));
    }
}

template<typename B, typename Q>
module::Decoder_SIHO<B, Q>*
Decoder_conv ::build_viterbi(const std::vector<std::vector<int>>& trellis) const
{
    return new module::Decoder_Viterbi_SIHO<B, Q>(this->K, trellis, true);
}

template<typename B, typename Q>
module::Decoder_SIHO<B, Q>*
Decoder_conv ::build_viterbi_list(const std::vector<std::vector<int>>& trellis, const module::CRC<B>* crc) const
{
    return new module::Decoder_Viterbi_list_parallel<B, Q>(this->K, this->N_cw, this->L, *crc, trellis, true);
}

template<typename B, typename Q>
module::Decoder_SIHO<B, Q>*
Decoder_conv ::build(const std::vector<std::vector<int>>& trellis,
                     const module::CRC<B>* crc,
                     module::Encoder<B>* encoder) const
{
    try
    {
        return Decoder::build<B, Q>(encoder);
    }
    catch (spu::tools::cannot_allocate const&)
    {
        if (this->type == "VITERBI") return build_viterbi<B, Q>(trellis);
        if (this->type == "PLVA" && crc != nullptr) return build_viterbi_list<B, Q>(trellis, crc);
    }

    throw spu::tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template aff3ct::module::Decoder_SIHO<B_8, Q_8>*
aff3ct::factory::Decoder_conv::build<B_8, Q_8>(const std::vector<std::vector<int>>&,
                                               const module::CRC<B_8>*,
                                               module::Encoder<B_8>*) const;
template aff3ct::module::Decoder_SIHO<B_16, Q_16>*
aff3ct::factory::Decoder_conv::build<B_16, Q_16>(const std::vector<std::vector<int>>&,
                                                 const module::CRC<B_16>*,
                                                 module::Encoder<B_16>*) const;
template aff3ct::module::Decoder_SIHO<B_32, Q_32>*
aff3ct::factory::Decoder_conv::build<B_32, Q_32>(const std::vector<std::vector<int>>&,
                                                 const module::CRC<B_32>*,
                                                 module::Encoder<B_32>*) const;
template aff3ct::module::Decoder_SIHO<B_64, Q_64>*
aff3ct::factory::Decoder_conv::build<B_64, Q_64>(const std::vector<std::vector<int>>&,
                                                 const module::CRC<B_64>*,
                                                 module::Encoder<B_64>*) const;
#else
template aff3ct::module::Decoder_SIHO<B, Q>*
aff3ct::factory::Decoder_conv::build<B, Q>(const std::vector<std::vector<int>>&,
                                           const module::CRC<B>*,
                                           module::Encoder<B>*) const;
#endif
// ==================================================================================== explicit template instantiation
