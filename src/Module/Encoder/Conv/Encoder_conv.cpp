#include <sstream>
#include <streampu.hpp>
#include <string>

#include "Module/Encoder/Conv/Encoder_conv.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template<typename B>
Encoder_conv<B>::Encoder_conv(const int K, const int N, const std::vector<int>& poly)
  : Encoder<B>(K, N)
  , n_ff(0)
  , n_poly(static_cast<int>(poly.size()))
  , n_states(0)
  , poly(poly)
  , next_state_table()
  , output_table()
{
    const std::string name = "Encoder_conv";
    this->set_name(name);
    for (auto& t : this->tasks)
        t->set_replicability(true);

    if (n_poly < 2)
    {
        std::stringstream message;
        message << "'poly' must have at least 2 elements ('poly.size()' = " << n_poly << ").";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    // Determine n_ff from the maximum bit width of the polynomials
    int max_poly = 0;
    for (auto g : poly)
        if (g > max_poly) max_poly = g;

    // n_ff = floor(log2(max_poly)), i.e. the number of memory elements
    int tmp = max_poly;
    int bits = 0;
    while (tmp > 0)
    {
        bits++;
        tmp >>= 1;
    }
    // const_cast to set the const member after computation
    const_cast<int&>(this->n_ff) = bits - 1;
    const_cast<int&>(this->n_states) = 1 << this->n_ff;

    if (this->n_ff <= 0)
    {
        std::stringstream message;
        message << "'n_ff' has to be greater than 0 ('n_ff' = " << this->n_ff << ").";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    if (N != n_poly * (K + this->n_ff))
    {
        std::stringstream message;
        message << "'N' must equal 'n_poly' * ('K' + 'n_ff') ('N' = " << N << ", 'n_poly' = " << n_poly
                << ", 'K' = " << K << ", 'n_ff' = " << this->n_ff << ").";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    // Set info_bits_pos: positions of the "systematic" bit in interleaved output
    // For rate 1/n_poly, info bits are at positions [0, n_poly, 2*n_poly, ...]
    for (auto k = 0; k < this->K; k++)
        this->info_bits_pos[k] = n_poly * k;

    build_tables();
}

template<typename B>
Encoder_conv<B>*
Encoder_conv<B>::clone() const
{
    auto m = new Encoder_conv(*this);
    m->deep_copy(*this);
    return m;
}

template<typename B>
void
Encoder_conv<B>::build_tables()
{
    next_state_table.resize(2 * n_states);
    output_table.resize(2 * n_states);

    for (int s = 0; s < n_states; s++)
    {
        for (int b = 0; b < 2; b++)
        {
            // full_reg: input bit in MSB position, then the shift register state
            int full_reg = (b << n_ff) | s;

            // Next state: shift right by 1 (newest bit enters from left)
            next_state_table[2 * s + b] = full_reg >> 1;

            // Compute output for each polynomial
            int packed_output = 0;
            for (int p = 0; p < n_poly; p++)
            {
                int masked = full_reg & poly[p];
                // popcount mod 2
                int bit = 0;
                while (masked)
                {
                    bit ^= (masked & 1);
                    masked >>= 1;
                }
                packed_output = (packed_output << 1) | bit;
            }
            output_table[2 * s + b] = packed_output;
        }
    }
}

template<typename B>
int
Encoder_conv<B>::tail_length() const
{
    return n_poly * n_ff;
}

template<typename B>
void
Encoder_conv<B>::_encode(const B* U_K, B* X_N, const size_t /*frame_id*/)
{
    int state = 0;

    // Encode K information bits
    for (int i = 0; i < this->K; i++)
    {
        int b = static_cast<int>(U_K[i]);
        int packed = output_table[2 * state + b];
        state = next_state_table[2 * state + b];

        // Unpack n_poly output bits into interleaved positions
        for (int p = 0; p < n_poly; p++)
            X_N[i * n_poly + p] = static_cast<B>((packed >> (n_poly - 1 - p)) & 1);
    }

    // Tail bits: drive state back to 0 by feeding 0
    for (int i = 0; i < n_ff; i++)
    {
        int b = 0;
        int packed = output_table[2 * state + b];
        state = next_state_table[2 * state + b];

        for (int p = 0; p < n_poly; p++)
            X_N[(this->K + i) * n_poly + p] = static_cast<B>((packed >> (n_poly - 1 - p)) & 1);
    }

    if (state != 0)
    {
        std::stringstream message;
        message << "'state' should be equal to 0 after tail bits ('state' = " << state << ").";
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
    }
}

template<typename B>
std::vector<std::vector<int>>
Encoder_conv<B>::get_trellis()
{
    std::vector<std::vector<int>> trellis(10, std::vector<int>(n_states));

    // Fill trellis[0..5] for BCJR compatibility (same convention as RSC encoder)
    std::vector<bool> occurrence(n_states, false);

    for (int i = 0; i < n_states; i++)
    {
        // input = 0
        int next0 = next_state_table[2 * i + 0];
        int out0 = output_table[2 * i + 0];

        trellis[0 + (occurrence[next0] ? 3 : 0)][next0] = i;    // initial state
        trellis[1 + (occurrence[next0] ? 3 : 0)][next0] = +1;   // gamma coeff
        trellis[2 + (occurrence[next0] ? 3 : 0)][next0] = out0; // gamma (packed output)

        trellis[6][i] = next0; // next state, input = 0
        trellis[7][i] = out0;  // packed output, input = 0

        occurrence[next0] = true;

        // input = 1
        int next1 = next_state_table[2 * i + 1];
        int out1 = output_table[2 * i + 1];

        trellis[0 + (occurrence[next1] ? 3 : 0)][next1] = i;    // initial state
        trellis[1 + (occurrence[next1] ? 3 : 0)][next1] = -1;   // gamma coeff
        trellis[2 + (occurrence[next1] ? 3 : 0)][next1] = out1; // gamma (packed output)

        trellis[8][i] = next1; // next state, input = 1
        trellis[9][i] = out1;  // packed output, input = 1

        occurrence[next1] = true;
    }

    return trellis;
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Encoder_conv<B_8>;
template class aff3ct::module::Encoder_conv<B_16>;
template class aff3ct::module::Encoder_conv<B_32>;
template class aff3ct::module::Encoder_conv<B_64>;
#else
template class aff3ct::module::Encoder_conv<B>;
#endif
// ==================================================================================== explicit template instantiation
