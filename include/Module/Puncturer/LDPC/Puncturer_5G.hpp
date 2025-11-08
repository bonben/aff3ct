/*!
 * \file
 * \brief Class module::Puncturer_5G.
 */
#ifndef Puncturer_5G_HPP_
#define Puncturer_5G_HPP_

#include <vector>

#include "Module/Puncturer/Puncturer.hpp"
#include "Tools/Code/LDPC/Standard/5G/5G_base_graph.hpp"

namespace aff3ct
{
namespace module
{
template<typename B = int, typename Q = float>
class Puncturer_5G : public Puncturer<B, Q>
{
  protected:
    tools::Std_5G_base_graph base_graph;

  public:
    Puncturer_5G(const int& K, const int& N, const int& N_cw, const std::vector<bool>& pattern);
    virtual ~Puncturer_5G() = default;

    virtual Puncturer_5G<B, Q>* clone() const;

  protected:
    void _puncture(const B* X_N1, B* X_N2, const size_t frame_id) const;
    void _depuncture(const Q* Y_N1, Q* Y_N2, const size_t frame_id) const;
};
}
}

#endif /* Puncturer_5G_HPP_ */
