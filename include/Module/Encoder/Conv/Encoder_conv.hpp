/*!
 * \file
 * \brief Class module::Encoder_conv.
 */
#ifndef ENCODER_CONV_HPP_
#define ENCODER_CONV_HPP_

#include <vector>

#include "Module/Encoder/Encoder.hpp"
#include "Tools/Interface/Interface_get_trellis.hpp"

namespace aff3ct
{
namespace module
{
template<typename B = int>
class Encoder_conv
  : public Encoder<B>
  , public tools::Interface_get_trellis
{
  protected:
    const int n_ff;                    // number of memory elements (constraint_length - 1)
    const int n_poly;                  // number of generator polynomials (rate = 1/n_poly)
    const int n_states;                // number of states in the trellis (2^n_ff)
    std::vector<int> poly;             // generator polynomials (octal)
    std::vector<int> next_state_table; // precomputed next states, size: 2 * n_states
    std::vector<int> output_table;     // precomputed outputs (packed), size: 2 * n_states

  public:
    Encoder_conv(const int K, const int N, const std::vector<int>& poly);
    virtual ~Encoder_conv() = default;

    int tail_length() const override;
    std::vector<std::vector<int>> get_trellis() override;

  protected:
    void _encode(const B* U_K, B* X_N, const size_t frame_id) override;

  private:
    void build_tables();
};
}
}

#endif /* ENCODER_CONV_HPP_ */
