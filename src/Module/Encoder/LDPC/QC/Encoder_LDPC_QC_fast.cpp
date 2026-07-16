#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Tools/Exception/exception.hpp"

#include "Module/Encoder/LDPC/QC/Encoder_LDPC_QC_fast.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template<typename B>
Encoder_LDPC_QC_fast<B>::Encoder_LDPC_QC_fast(const int K,
                                              const int N,
                                              const int Zc,
                                              const char* file_name,
                                              const int K_LDPC)
  : Encoder_LDPC_QC<B>(K, N, Zc, file_name, K_LDPC)
{
    const std::string name = "Encoder_LDPC_QC_fast";
    this->set_name(name);
    this->G = Encoder_LDPC_QC<B>::read_G_file(this->file_name);
    this->_fill_rot();
}

template<typename B>
Encoder_LDPC_QC_fast<B>*
Encoder_LDPC_QC_fast<B>::clone() const
{
    auto m = new Encoder_LDPC_QC_fast(*this);
    m->deep_copy(*this);
    return m;
}

template<typename B>
void
Encoder_LDPC_QC_fast<B>::_fill_rot()
{
    std::vector<B> v(this->Zc);
    for (int j = 0; j < this->K_LDPC / this->Zc; j++)
    {
        for (int i = 0; i < (this->N - this->K_LDPC) / this->Zc; i++)
        {
            std::copy(this->G[j].begin() + i * this->Zc, this->G[j].begin() + (i + 1) * this->Zc, v.begin());
            this->rot.push_back(std::vector<B>());
            this->rot.reserve(this->rot.empty() ? 0 : this->rot.front().size());
            this->rot[i + (this->N - this->K_LDPC) / this->Zc * j] = _find_items(v, 1);
        }
    }
}

template<typename B>
std::vector<B>
Encoder_LDPC_QC_fast<B>::_find_items(std::vector<B> v, int target)
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
void
Encoder_LDPC_QC_fast<B>::_encode(const B* U_K, B* X_N, const size_t frame_id)
{
    std::memcpy(X_N, U_K, sizeof(B) * this->K);
    const int n_zeros_pad = this->K_LDPC - this->K;
    std::memset(X_N + this->K, 0, sizeof(B) * n_zeros_pad);

    const int nb_blocks = (this->N - this->K_LDPC) / this->Zc;
    bool first_time = true;
    B* parity_bits = X_N + this->K_LDPC;
    for (int i = 0; i < this->K_LDPC / this->Zc; i++)
    {
        const B* input_ptr = X_N + i * this->Zc;
        const int base = nb_blocks * i;
        for (int j = 0; j < nb_blocks; j++)
        {
            B* parity_block = parity_bits + j * this->Zc;
            if (first_time) std::memset(parity_block, 0, sizeof(B) * this->Zc);

            const auto& shifts = this->rot[j + base];
            for (size_t k = 0; k < shifts.size(); k++)
            {
                const int shift = shifts[k];
                int start = this->Zc - shift;
                int l = 0;
                for (; l < shift; l++)
                    parity_block[l] ^= input_ptr[start + l];
                for (; l < this->Zc; l++)
                    parity_block[l] ^= input_ptr[l - shift];
            }
        }
        first_time = false;
    }
}

// ==================================================================================== explicit template instantiation
#include "Module/Encoder/LDPC/QC/Encoder_LDPC_QC_fast.hpp"
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Encoder_LDPC_QC_fast<B_8>;
template class aff3ct::module::Encoder_LDPC_QC_fast<B_16>;
template class aff3ct::module::Encoder_LDPC_QC_fast<B_32>;
template class aff3ct::module::Encoder_LDPC_QC_fast<B_64>;
#else
template class aff3ct::module::Encoder_LDPC_QC_fast<B>;
#endif
// ==================================================================================== explicit template instantiation