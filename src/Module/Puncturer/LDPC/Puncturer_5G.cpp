#include <algorithm>
#include <sstream>
#include <streampu.hpp>
#include <string>

#include "Module/Puncturer/LDPC/Puncturer_5G.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template<typename B, typename Q>
Puncturer_5G<B, Q>::Puncturer_5G(const int& K, const int& N, const int& N_cw, const std::vector<bool>& pattern)
  : Puncturer<B, Q>(K, N, N_cw)
{
    const std::string name = "Puncturer_LDPC_5G";
    this->set_name(name);
    this->base_graph = tools::build_5G_base_graph(K, N);
    for (auto& t : this->tasks)
        t->set_replicability(true);
}

template<typename B, typename Q>
Puncturer_5G<B, Q>*
Puncturer_5G<B, Q>::clone() const
{
    auto m = new Puncturer_5G(*this);
    m->deep_copy(*this);
    return m;
}

template<typename B, typename Q>
void
Puncturer_5G<B, Q>::_puncture(const B* X_N1, B* X_N2, const size_t /*frame_id*/) const
{
    int k = 0;
    int j = 0;
    while (k < this->N)
    {
        if (!(j % (this->N_cw - 2 * this->base_graph.Zc) + 2 * this->base_graph.Zc < this->base_graph.K_LDPC &&
              this->K <= j % (this->N_cw - 2 * this->base_graph.Zc) + 2 * this->base_graph.Zc))
        {
            X_N2[k] = X_N1[j % (this->N_cw - 2 * this->base_graph.Zc) + 2 * this->base_graph.Zc];
            k++;
        }
        j++;
    }
}

template<typename B, typename Q>
void
Puncturer_5G<B, Q>::_depuncture(const Q* Y_N1, Q* Y_N2, const size_t /*frame_id*/) const
{
    int k = 0;
    int j = 0;
    while (k < this->N)
    {
        if (!(j % (this->N_cw - 2 * this->base_graph.Zc) + 2 * this->base_graph.Zc < this->base_graph.K_LDPC &&
              this->K <= j % (this->N_cw - 2 * this->base_graph.Zc) + 2 * this->base_graph.Zc))
        {
            Y_N2[j % (this->N_cw - 2 * this->base_graph.Zc) + 2 * this->base_graph.Zc] = Y_N1[k];
            k++;
        }
        j++;
    }
    std::fill(Y_N2, Y_N2 + 2 * this->base_graph.Zc, (Q)0);
    if (std::is_integral<Q>::value)
        std::fill(Y_N2 + this->K, Y_N2 + this->K + this->base_graph.K_LDPC - this->K, std::numeric_limits<Q>::max());
    else
        std::fill(
          Y_N2 + this->K, Y_N2 + this->K + this->base_graph.K_LDPC - this->K, std::numeric_limits<Q>::infinity());
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Puncturer_5G<B_8, Q_8>;
template class aff3ct::module::Puncturer_5G<B_16, Q_16>;
template class aff3ct::module::Puncturer_5G<B_32, Q_32>;
template class aff3ct::module::Puncturer_5G<B_64, Q_64>;
#else
template class aff3ct::module::Puncturer_5G<B, Q>;
#endif
// ==================================================================================== explicit template instantiation
