#include <algorithm>
#include <cmath>
#include <sstream>
#include <streampu.hpp>
#include <string>

#include "Module/Encoder/Polar/Encoder_polar_bitpacked.hpp"
#include "Tools/Code/Polar/Pattern_polar_parser.hpp"
#include "Tools/Code/Polar/Patterns/Pattern_polar_r0.hpp"
#include "Tools/Code/Polar/Patterns/Pattern_polar_r1.hpp"
#include "Tools/Code/Polar/Patterns/Pattern_polar_rep.hpp"
#include "Tools/Code/Polar/Patterns/Pattern_polar_spc.hpp"
#include "Tools/Code/Polar/Patterns/Pattern_polar_std.hpp"
#include "Tools/Code/Polar/fb_assert.h"

using namespace aff3ct::module;
using namespace aff3ct::tools;

template<typename B>
Encoder_polar_bitpacked<B>::Encoder_polar_bitpacked(const int& K, const int& N, const std::vector<bool>& frozen_bits)
  : Encoder_polar<B>(K, N, frozen_bits)
  , packed_frozen_bits(N >= 64 ? (N >> 6) : 0, 0)
  , pack_buffer(N >= 64 ? (N >> 6) : 0, 0)
{
    const std::string name = "Encoder_polar_bitpacked";
    this->set_name(name);

    this->set_frozen_bits(frozen_bits);
}

template<typename B>
Encoder_polar_bitpacked<B>*
Encoder_polar_bitpacked<B>::clone() const
{
    auto m = new Encoder_polar_bitpacked(*this);
    m->deep_copy(*this);
    return m;
}

template<typename B>
void
Encoder_polar_bitpacked<B>::set_frozen_bits(const std::vector<bool>& frozen_bits)
{
    Encoder_polar<B>::set_frozen_bits(frozen_bits);

    if (this->N >= 64)
    {
        const size_t n_words = this->N >> 6;
        this->packed_frozen_bits.assign(n_words, 0);
        this->pack_buffer.assign(n_words, 0);

        for (size_t w = 0; w < n_words; ++w)
        {
            uint64_t word = 0;
            const size_t base = w << 6;
            for (size_t j = 0; j < 64; ++j)
            {
                word = (word << 1) | (!this->frozen_bits[base + j] ? 1ULL : 0ULL);
            }
            this->packed_frozen_bits[w] = word;
        }

        build_tree_execution_plan();
    }
}

template<typename B>
void
Encoder_polar_bitpacked<B>::build_tree_execution_plan()
{
    this->execution_plan.clear();

    std::vector<aff3ct::tools::Pattern_polar_i*> patterns = { new aff3ct::tools::Pattern_polar_r0(),
                                                              new aff3ct::tools::Pattern_polar_r1(),
                                                              new aff3ct::tools::Pattern_polar_rep(),
                                                              new aff3ct::tools::Pattern_polar_spc(),
                                                              new aff3ct::tools::Pattern_polar_std() };

    aff3ct::tools::Pattern_polar_parser parser(this->frozen_bits, patterns, 0, 1, true);
    const auto& leaves = parser.get_leaves_pattern_types();

    int bit_offset = 0;

    for (const auto& leaf : leaves)
    {
        polar_node_t p_type = static_cast<polar_node_t>(leaf.first);
        int p_size = leaf.second;

        Tree_node_info info;
        info.type = static_cast<int>(p_type);
        info.size = p_size;
        info.off_bit = bit_offset;

        this->execution_plan.push_back(info);

        bit_offset += p_size;
    }
}

template<typename B>
void
Encoder_polar_bitpacked<B>::encode_tree_bitpacked(const B* U_K, uint64_t* pack_data)
{
    const size_t n_words = this->N >> 6;
    std::fill(pack_data, pack_data + n_words, 0ULL);

    int u_idx = 0;

    for (const auto& node : this->execution_plan)
    {
        polar_node_t p_type = static_cast<polar_node_t>(node.type);

        if (p_type == polar_node_t::RATE_0 ||
            p_type == polar_node_t::RATE_0_LEFT) // R0 Node: all frozen (0s). Complete skip!
        {
            continue;
        }
        else if (p_type == polar_node_t::REP || p_type == polar_node_t::REP_LEFT) // REP Node: last bit is info bit
        {
            uint64_t u_last = static_cast<uint64_t>(U_K[u_idx++]) & 1u;
            int pos = node.off_bit + node.size - 1;
            size_t w = pos >> 6;
            size_t bit_pos = 63 - (pos & 63);
            pack_data[w] |= (u_last << bit_pos);
        }
        else if (p_type == polar_node_t::RATE_1) // R1 Node: all bits are information bits
        {
            if (node.size >= 64)
            {
                pack(U_K + u_idx, pack_data + (node.off_bit >> 6), node.size);
                u_idx += node.size;
            }
            else
            {
                uint64_t word_accum = 0;
                for (int i = 0; i < node.size; ++i)
                {
                    uint64_t b = static_cast<uint64_t>(U_K[u_idx++]) & 1u;
                    size_t bit_pos = 63 - ((node.off_bit + i) & 63);
                    word_accum |= (b << bit_pos);
                }
                pack_data[node.off_bit >> 6] |= word_accum;
            }
        }
        else if (p_type == polar_node_t::SPC) // SPC Node: first bit is frozen, remaining are info bits
        {
            if (node.size <= 64)
            {
                uint64_t word_accum = 0;
                for (int i = 1; i < node.size; ++i)
                {
                    uint64_t b = static_cast<uint64_t>(U_K[u_idx++]) & 1u;
                    size_t bit_pos = 63 - ((node.off_bit + i) & 63);
                    word_accum |= (b << bit_pos);
                }
                pack_data[node.off_bit >> 6] |= word_accum;
            }
            else
            {
                uint64_t word_accum0 = 0;
                for (int i = 1; i < 64; ++i)
                {
                    uint64_t b = static_cast<uint64_t>(U_K[u_idx++]) & 1u;
                    size_t bit_pos = 63 - i;
                    word_accum0 |= (b << bit_pos);
                }
                pack_data[node.off_bit >> 6] |= word_accum0;
                pack(U_K + u_idx, pack_data + (node.off_bit >> 6) + 1, node.size - 64);
                u_idx += (node.size - 64);
            }
        }
        else // STANDARD Node: mixed frozen/info node
        {
            if (node.size <= 64)
            {
                uint64_t word_accum = 0;
                for (int i = 0; i < node.size; ++i)
                {
                    int pos = node.off_bit + i;
                    if (!this->frozen_bits[pos])
                    {
                        uint64_t b = static_cast<uint64_t>(U_K[u_idx++]) & 1u;
                        size_t bit_pos = 63 - (pos & 63);
                        word_accum |= (b << bit_pos);
                    }
                }
                pack_data[node.off_bit >> 6] |= word_accum;
            }
            else
            {
                const int n_sub_words = node.size >> 6;
                const size_t base_word = node.off_bit >> 6;
                for (int w = 0; w < n_sub_words; ++w)
                {
                    uint64_t word_accum = 0;
                    const int base_bit = (w << 6);
                    for (int i = 0; i < 64; ++i)
                    {
                        int pos = node.off_bit + base_bit + i;
                        if (!this->frozen_bits[pos])
                        {
                            uint64_t b = static_cast<uint64_t>(U_K[u_idx++]) & 1u;
                            size_t bit_pos = 63 - i;
                            word_accum |= (b << bit_pos);
                        }
                    }
                    pack_data[base_word + w] |= word_accum;
                }
            }
        }
    }

    // Single SIMD Bitpacked Butterfly Transformation pass over the packed sequence
    this->transform_packed(pack_data, this->N);
}

template<typename B>
void
Encoder_polar_bitpacked<B>::pack(const B* bits_in, uint64_t* pack_out, const size_t N)
{
    const size_t n_words = N >> 6;
    for (size_t w = 0; w < n_words; ++w)
    {
        const B* in = bits_in + (w << 6);
        uint64_t symb = 0;
        for (int b = 0; b < 8; ++b)
        {
            const B* p = in + (b << 3);
            uint64_t byte_val =
              ((static_cast<uint64_t>(p[0]) & 1ULL) << 7) | ((static_cast<uint64_t>(p[1]) & 1ULL) << 6) |
              ((static_cast<uint64_t>(p[2]) & 1ULL) << 5) | ((static_cast<uint64_t>(p[3]) & 1ULL) << 4) |
              ((static_cast<uint64_t>(p[4]) & 1ULL) << 3) | ((static_cast<uint64_t>(p[5]) & 1ULL) << 2) |
              ((static_cast<uint64_t>(p[6]) & 1ULL) << 1) | ((static_cast<uint64_t>(p[7]) & 1ULL) << 0);
            symb = (symb << 8) | byte_val;
        }
        pack_out[w] = symb;
    }
}

template<typename B>
void
Encoder_polar_bitpacked<B>::unpack(const uint64_t* pack_in, B* bits_out, const size_t N)
{
    constexpr int W = mipp::N<B>();

    if (W > 1 && W <= 64)
    {
        B mask_arr[W];
        for (int i = 0; i < W; ++i)
            mask_arr[i] = static_cast<B>(1ULL << (W - 1 - i));

        const mipp::Reg<B> bit_masks = mask_arr;
        const mipp::Reg<B> zero = static_cast<B>(0);
        const mipp::Reg<B> one = static_cast<B>(1);

        const size_t n_chunks = N / W;
        for (size_t c = 0; c < n_chunks; ++c)
        {
            const size_t bit_idx = c * W;
            const size_t word_idx = bit_idx >> 6;
            const size_t bit_pos = bit_idx & 63;
            const uint64_t s = pack_in[word_idx];
            const uint64_t chunk_val = (s >> (64 - bit_pos - W)) & ((W == 64) ? ~0ULL : ((1ULL << W) - 1ULL));

            const mipp::Reg<B> v = static_cast<B>(chunk_val);
            const mipp::Reg<B> test = v & bit_masks;
            const mipp::Msk<W> is_one = (test != zero);
            const mipp::Reg<B> res = mipp::blend<B>(one.r, zero.r, is_one.m);
            res.storeu(bits_out + bit_idx);
        }
    }
    else
    {
        const size_t n_words = N >> 6;
        for (size_t w = 0; w < n_words; ++w)
        {
            const uint64_t s = pack_in[w];
            B* out = bits_out + (w << 6);
            for (size_t b = 0; b < 8; ++b)
            {
                const uint32_t byte = static_cast<uint32_t>((s >> ((7 - b) << 3)) & 0xFF);
                out[(b << 3) + 0] = static_cast<B>((byte >> 7) & 1);
                out[(b << 3) + 1] = static_cast<B>((byte >> 6) & 1);
                out[(b << 3) + 2] = static_cast<B>((byte >> 5) & 1);
                out[(b << 3) + 3] = static_cast<B>((byte >> 4) & 1);
                out[(b << 3) + 4] = static_cast<B>((byte >> 3) & 1);
                out[(b << 3) + 5] = static_cast<B>((byte >> 2) & 1);
                out[(b << 3) + 6] = static_cast<B>((byte >> 1) & 1);
                out[(b << 3) + 7] = static_cast<B>((byte >> 0) & 1);
            }
        }
    }
}

template<typename B>
void
Encoder_polar_bitpacked<B>::transform_packed(uint64_t* pack_data, const size_t N)
{
    const size_t n_words = N >> 6;
    if (n_words == 0) return;

    static const uint64_t masks[6] = {
        0xAAAAAAAAAAAAAAAAULL, // d = 1
        0xCCCCCCCCCCCCCCCCULL, // d = 2
        0xF0F0F0F0F0F0F0F0ULL, // d = 4
        0xFF00FF00FF00FF00ULL, // d = 8
        0xFFFF0000FFFF0000ULL, // d = 16
        0xFFFFFFFF00000000ULL  // d = 32
    };

    constexpr int W_reg = mipp::N<uint64_t>();
    const size_t n_simd = n_words / W_reg;

    if (n_simd > 0)
    {
        const mipp::Reg<uint64_t> m0 = masks[0];
        const mipp::Reg<uint64_t> m1 = masks[1];
        const mipp::Reg<uint64_t> m2 = masks[2];
        const mipp::Reg<uint64_t> m3 = masks[3];
        const mipp::Reg<uint64_t> m4 = masks[4];
        const mipp::Reg<uint64_t> m5 = masks[5];

        for (size_t r = 0; r < n_simd; ++r)
        {
            uint64_t* ptr = pack_data + r * W_reg;
            mipp::Reg<uint64_t> reg;
            reg.loadu(ptr);
            reg ^= ((reg << 1) & m0);
            reg ^= ((reg << 2) & m1);
            reg ^= ((reg << 4) & m2);
            reg ^= ((reg << 8) & m3);
            reg ^= ((reg << 16) & m4);
            reg ^= ((reg << 32) & m5);
            reg.storeu(ptr);
        }

        for (size_t d_words = 1; d_words < n_words; d_words <<= 1)
        {
            if (d_words < static_cast<size_t>(W_reg))
            {
                const size_t block_size = d_words << 1;
                for (size_t b = 0; b < n_words; b += block_size)
                {
                    for (size_t i = 0; i < d_words; ++i)
                    {
                        pack_data[b + i] ^= pack_data[b + d_words + i];
                    }
                }
            }
            else
            {
                const size_t d_regs = d_words / W_reg;
                const size_t block_regs = d_regs << 1;
                for (size_t b = 0; b < n_simd; b += block_regs)
                {
                    for (size_t i = 0; i < d_regs; ++i)
                    {
                        uint64_t* ptr_top = pack_data + (b + i) * W_reg;
                        uint64_t* ptr_bot = pack_data + (b + d_regs + i) * W_reg;
                        mipp::Reg<uint64_t> reg_top, reg_bot;
                        reg_top.loadu(ptr_top);
                        reg_bot.loadu(ptr_bot);
                        reg_top ^= reg_bot;
                        reg_top.storeu(ptr_top);
                    }
                }
            }
        }
    }
    else
    {
        for (int s = 0; s < 6; ++s)
        {
            const int d = 1 << s;
            const uint64_t mask = masks[s];
            for (size_t w = 0; w < n_words; ++w)
            {
                pack_data[w] ^= ((pack_data[w] << d) & mask);
            }
        }
        for (size_t d_words = 1; d_words < n_words; d_words <<= 1)
        {
            const size_t block_size = d_words << 1;
            for (size_t b = 0; b < n_words; b += block_size)
            {
                for (size_t i = 0; i < d_words; ++i)
                {
                    pack_data[b + i] ^= pack_data[b + d_words + i];
                }
            }
        }
    }
}

template<typename B>
void
Encoder_polar_bitpacked<B>::_encode(const B* U_K, B* X_N, const size_t /*frame_id*/)
{
    if (this->N < 64)
    {
        Encoder_polar<B>::_encode(U_K, X_N, 0);
        return;
    }

    encode_tree_bitpacked(U_K, this->pack_buffer.data());
    unpack(this->pack_buffer.data(), X_N, this->N);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Encoder_polar_bitpacked<B_8>;
template class aff3ct::module::Encoder_polar_bitpacked<B_16>;
template class aff3ct::module::Encoder_polar_bitpacked<B_32>;
template class aff3ct::module::Encoder_polar_bitpacked<B_64>;
#else
template class aff3ct::module::Encoder_polar_bitpacked<B>;
#endif
// ==================================================================================== explicit template instantiation
