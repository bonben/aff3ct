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

#include "Module/Encoder/LDPC/QC/Encoder_LDPC_QC.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template<typename B>
Encoder_LDPC_QC<B>::Encoder_LDPC_QC(const int K, const int N, const int Zc, const char* file_name)
  : Encoder_LDPC<B>(K, N)
  , Zc(Zc)
  , file_name(file_name)
  , G(K / Zc)
{
    const std::string name = "Encoder_LDPC_QC";
    this->set_name(name);
    this->G = this->read_G_file(this->file_name);
}

template<typename B>
Encoder_LDPC_QC<B>*
Encoder_LDPC_QC<B>::clone() const
{
    auto m = new Encoder_LDPC_QC(*this);
    m->deep_copy(*this);
    return m;
}

template<typename B>
std::vector<std::vector<B>>
Encoder_LDPC_QC<B>::read_G_file(const char* file_name)
{
    std::ifstream fichier(file_name, std::ios::in);
    std::string ligne;
    std::istream_iterator<int> stream_end;
    std::vector<std::vector<B>> P;
    if (fichier)
    {
        while (std::getline(fichier, ligne))
        {
            P.push_back(std::vector<B>());
            P.reserve(P.empty() ? 0 : P.front().size());
            std::istringstream ligne_stream(ligne);
            std::copy(std::istream_iterator<int>(ligne_stream), stream_end, std::back_inserter(P.back()));
        }
    }
    return P;
}

template<typename B>
void
Encoder_LDPC_QC<B>::_MultiplyAdd(std::vector<B>& v, int k, std::vector<B>& r)
{
    std::vector<B> i(v.size(), 0);
    std::transform(v.begin(), v.end(), i.begin(), [k](B& c) { return c * k; });
    std::transform(i.begin(), i.end(), r.begin(), r.begin(), [](B& c, B& b) { return (c + b) % 2; });
}

template<typename B>
std::vector<B>
Encoder_LDPC_QC<B>::_CSRAA(const int Zc, std::vector<B> Gen, const B* vect, const int K, const int N)
{
    int u;
    std::vector<B> res(N - K, 0);
    for (int i = 0; i < Zc; i++)
    {
        u = vect[i];
        this->_MultiplyAdd(Gen, u, res);
        for (int j = 0; j < (N - K) / Zc; j++)
        {
            std::rotate(Gen.begin() + j * Zc, Gen.begin() + (j + 1) * Zc - 1, Gen.begin() + (j + 1) * Zc);
        }
    }
    return res;
}

template<typename B>
void
Encoder_LDPC_QC<B>::_encode(const B* U_K, B* X_N, const size_t frame_id)
{
    std::memcpy(X_N, U_K, sizeof(B) * this->K);
    std::vector<B> vect(this->N - this->K, 0);
    std::vector<B> res(this->N - this->K, 0);
    for (int i = 0; i < this->K / this->Zc; i++)
    {
        res = this->_CSRAA(this->Zc, this->G[i], U_K + i * this->Zc, this->K, this->N);
        std::transform(vect.begin(), vect.end(), res.begin(), vect.begin(), [](B& c, B& b) { return (c + b) % 2; });
    }

    for (int i = this->K; i < this->N; i++)
    {
        X_N[i] = vect[i - this->K];
    }
}

// ==================================================================================== explicit template instantiation
#include "Module/Encoder/LDPC/QC/Encoder_LDPC_QC.hpp"
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Encoder_LDPC_QC<B_8>;
template class aff3ct::module::Encoder_LDPC_QC<B_16>;
template class aff3ct::module::Encoder_LDPC_QC<B_32>;
template class aff3ct::module::Encoder_LDPC_QC<B_64>;
#else
template class aff3ct::module::Encoder_LDPC_QC<B>;
#endif
// ==================================================================================== explicit template instantiation