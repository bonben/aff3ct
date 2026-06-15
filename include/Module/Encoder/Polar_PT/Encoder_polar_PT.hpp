/*!
 * \file
 * \brief Class module::Encoder_polar_PT.
 */
#ifndef ENCODER_POLAR_PT_HPP_
#define ENCODER_POLAR_PT_HPP_

#include <map>
#include <vector>

#include "Module/Encoder/Encoder.hpp"
#include "Tools/Interface/Interface_get_set_dynamic_frozen_bits.hpp"
#include "Tools/Interface/Interface_get_set_frozen_bits.hpp"
#include "Tools/Interface/Interface_get_set_pretransform.hpp"

namespace aff3ct
{
namespace module
{
template<typename B = int>
class Encoder_polar_PT
  : public Encoder<B>
  , public tools::Interface_get_set_pretransform
  , public tools::Interface_get_set_dynamic_frozen_bits
{
  protected:
    const int m;                   // log_2 of code length
    std::vector<bool> frozen_bits; // true means frozen, false means set to 0/1
    std::vector<bool> dynamic_frozen_bits;
    std::vector<B> X_N_tmp;
    std::map<uint32_t, std::vector<uint32_t>> pre_transform;
    std::vector<int> info_bits_loc;

  public:
    Encoder_polar_PT(const int& K,
                     const int& N,
                     const std::vector<bool>& frozen_bits,
                     const std::vector<bool>& dynamic_frozen_bits,
                     const std::map<uint32_t, std::vector<uint32_t>>& pretransform);
    virtual ~Encoder_polar_PT() = default;

    virtual Encoder_polar_PT<B>* clone() const;

    void light_encode(B* bits);

    bool is_codeword(const B* X_N);

    virtual const std::vector<bool>& get_frozen_bits() const;
    virtual void set_frozen_bits(const std::vector<bool>& frozen_bits);

    virtual const std::map<uint32_t, std::vector<uint32_t>>& get_pretransform() const;
    virtual void set_pretransform(const std::map<uint32_t, std::vector<uint32_t>>& pretrasform);

    virtual const std::vector<bool>& get_dynamic_frozen_bits() const;
    virtual void set_dynamic_frozen_bits(const std::vector<bool>& frozen_bits);

  protected:
    virtual void _encode(const B* U_K, B* X_N, const size_t frame_id);
    void convert(const B* U_K, B* U_N);

    B preTransform1Bit(B cbit, const int index);
    void preTransform(const B* U_K, B* X_N);
};
}
}

#endif // ENCODER_POLAR_PT_HPP_
