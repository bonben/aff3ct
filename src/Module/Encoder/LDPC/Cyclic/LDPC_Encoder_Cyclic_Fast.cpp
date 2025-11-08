#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "Tools/Exception/exception.hpp"

#include "Module/Encoder/LDPC/Cyclic/LDPC_Encoder_Cyclic_Fast.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template<typename B>
LDPC_Encoder_Cyclic_Fast<B>::LDPC_Encoder_Cyclic_Fast(const int K,
                                                      const int N,
                                                      const int Zc,
                                                      const char* file_name,
                                                      const int K_ldpc)
  : LDPC_Encoder_Cyclic<B>(K, N, Zc, file_name)
  , K_ldpc(K_ldpc)
{
    const std::string name = "LDPC_Encoder_Cyclic_Fast";
    this->set_name(name);
    // Allocate filled_input
    this->filled_input.resize(this->K_ldpc);
    this->G = LDPC_Encoder_Cyclic<B>::read_G_file(this->file_name);
    this->_fill_Rot();
}

template<typename B>
LDPC_Encoder_Cyclic_Fast<B>*
LDPC_Encoder_Cyclic_Fast<B>::clone() const
{
    auto m = new LDPC_Encoder_Cyclic_Fast(*this);
    m->deep_copy(*this);
    return m;
}

template<typename B>
void
LDPC_Encoder_Cyclic_Fast<B>::_fill_Rot()
{
    std::vector<B> v(this->Zc);
    for (int j = 0; j < this->K_ldpc / this->Zc; j++)
    {
        for (int i = 0; i < (this->N - this->K_ldpc) / this->Zc; i++)
        {
            std::copy(this->G[j].begin() + i * this->Zc, this->G[j].begin() + (i + 1) * this->Zc, v.begin());
            this->Rot.push_back(std::vector<B>());
            this->Rot.reserve(this->Rot.empty() ? 0 : this->Rot.front().size());
            this->Rot[i + (this->N - this->K_ldpc) / this->Zc * j] = _findItems(v, 1);
        }
    }
}

template<typename B>
std::vector<B>
LDPC_Encoder_Cyclic_Fast<B>::_findItems(std::vector<B> v, int target)
{
    std::vector<B> indices;
    auto it = v.begin();
    while ((it = std::find_if(it, v.end(), [target](B& e) { return e == target; })) != v.end())
    {
        indices.push_back(std::distance(v.begin(), it));
        it++;
    }
    return indices;
}

template<typename B>
std::vector<B>
LDPC_Encoder_Cyclic_Fast<B>::_CSRAA(B* vect, const int beg)
{
    std::vector<B> info(this->Zc, 0);
    std::vector<B> res(this->N - this->K_ldpc, 0);
    for (int i = 0; i < (this->N - this->K_ldpc) / this->Zc; i++)
    {
        for (long unsigned int j = 0; j < this->Rot[i + (this->N - this->K_ldpc) / this->Zc * beg].size(); j++)
        {
            for (int k = 0; k < this->Zc; k++)
            {
                info[k] = vect[k];
            }
            std::rotate(
              info.begin(), info.end() - this->Rot[i + (this->N - this->K_ldpc) / this->Zc * beg][j], info.end());
            std::transform(info.begin(),
                           info.end(),
                           res.begin() + i * this->Zc,
                           res.begin() + i * this->Zc,
                           [](B& c, B& b) { return (c + b) % 2; });
        }
    }
    return res;
}

template<typename B>
void
LDPC_Encoder_Cyclic_Fast<B>::_encode(const B* U_K, B* X_N, const size_t frame_id)
{
    // Going from K to K_ldpc by filling with zeros
    std::memcpy(this->filled_input.data(), U_K, sizeof(B) * this->K);
    std::memset(this->filled_input.data() + this->K, 0, sizeof(B) * (this->K_ldpc - this->K));
    std::memcpy(X_N, this->filled_input.data(), sizeof(B) * this->K_ldpc);

    std::vector<B> vect(this->N - this->K_ldpc, 0);
    std::vector<B> res(this->N - this->K_ldpc, 0);
    for (int i = 0; i < this->K_ldpc / this->Zc; i++)
    {
        res = this->_CSRAA((B*)(this->filled_input.data()) + i * this->Zc, i);
        std::transform(vect.begin(), vect.end(), res.begin(), vect.begin(), [](B& c, B& b) { return (c + b) % 2; });
    }

    for (int i = this->K_ldpc; i < this->N; i++)
    {
        X_N[i] = vect[i - this->K_ldpc];
    }
}

// ==================================================================================== explicit template instantiation
#include "Module/Encoder/LDPC/Cyclic/LDPC_Encoder_Cyclic_Fast.hpp"
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::LDPC_Encoder_Cyclic_Fast<B_8>;
template class aff3ct::module::LDPC_Encoder_Cyclic_Fast<B_16>;
template class aff3ct::module::LDPC_Encoder_Cyclic_Fast<B_32>;
template class aff3ct::module::LDPC_Encoder_Cyclic_Fast<B_64>;
#else
template class aff3ct::module::LDPC_Encoder_Cyclic_Fast<B>;
#endif
// ==================================================================================== explicit template instantiation