/*!
 * \file
 * \brief Class module::Encoder_polar_PAC.
 */
#ifndef ENCODER_POLAR_PAC_HPP_
#define ENCODER_POLAR_PAC_HPP_

#include <vector>

#include "Module/Encoder/Polar/Encoder_polar.hpp"
#include "Tools/Interface/Interface_get_set_frozen_bits.hpp"

namespace aff3ct
{
namespace module
{
template<typename B = int>
class Encoder_polar_PAC : public Encoder_polar<B>
{
  protected:
    std::vector<uint8_t> conv_reg;
    std::vector<B> curState;

  public:
    Encoder_polar_PAC(const int& K, const int& N, const std::vector<bool>& frozen_bits, const std::string& conv);
    virtual ~Encoder_polar_PAC() = default;

    virtual Encoder_polar_PAC<B>* clone() const;

    bool is_codeword(const B* X_N);

  protected:
    virtual void _encode(const B* U_K, B* X_N, const size_t frame_id);

    B conv1bitEnc(B cbit);
    void convEnc(B* X_N);
};
}
}

#endif // ENCODER_POLAR_PAC_HPP_
