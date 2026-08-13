#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ios>
#include <numeric>
#include <sstream>
#include <streampu.hpp>

#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator.hpp"
#include "Tools/Noise/noise_utils.h"

using namespace aff3ct;
using namespace aff3ct::tools;

std::thread::id Pretransform_generator::master_thread_id = std::this_thread::get_id();

Pretransform_generator ::Pretransform_generator(const int K,
                                                const int N,
                                                const std::string& dump_channels_path,
                                                const bool dump_channels_single_thread)
  : dump_channels_path(dump_channels_path)
  , dump_channels_single_thread(dump_channels_single_thread)
  , K(K)
  , N(N)
  , noise(nullptr)
  , best_channels(N)
  , info_bits_loc(K)
{
    std::iota(this->best_channels.begin(), this->best_channels.end(), 0);
    std::iota(this->info_bits_loc.begin(), this->info_bits_loc.end(), 0);
}

int
Pretransform_generator ::get_K() const
{
    return this->K;
}

int
Pretransform_generator ::get_N() const
{
    return this->N;
}

void
Pretransform_generator ::set_noise(const tools::Noise<>& noise)
{
    this->noise = &noise;
}

const tools::Noise<>&
Pretransform_generator ::get_noise() const
{
    if (this->noise == nullptr)
    {
        std::stringstream message;
        message << "'noise' should not be nullptr.";
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
    }

    return *this->noise;
}

void
Pretransform_generator ::generate(std::vector<bool>& dynamic_frozen_bits)
{
    if (dynamic_frozen_bits.size() != (unsigned)N)
    {
        std::stringstream message;
        message << "'dynamic_frozen_bits.size()' has to be equal to 'N' ('frozen_bits.size()' = "
                << dynamic_frozen_bits.size() << ", 'N' = " << N << ").";
        throw spu::tools::length_error(__FILE__, __LINE__, __func__, message.str());
    }

    this->evaluate();

    // init dynamic_frozen_bits vector, true means frozen bits, false means information bits
    std::fill(dynamic_frozen_bits.begin(), dynamic_frozen_bits.end(), false);
    // for (auto i = 0; i < K; i++)
    //     dynamic_frozen_bits[best_channels[i]] = false;
    // for (auto i = 0; i < K; i++)
    //     dynamic_frozen_bits[info_bits_loc[i]] = false;
    for (const auto& v : this->preTransform)
        dynamic_frozen_bits[v.first] = true;

    if (!dump_channels_path.empty() &&
        (!dump_channels_single_thread || this->master_thread_id == std::this_thread::get_id()))
    {
        std::string noise_type = "unkn";
        if (this->noise != nullptr)
        {
            switch (this->noise->get_type())
            {
                case tools::Noise_type::SIGMA:
                    noise_type = "awgn";
                    break;
                case tools::Noise_type::EP:
                    noise_type = "bec";
                    break;
                default:
                    break;
            };
        }

        std::stringstream noise;
        noise << std::fixed << std::setprecision(3) << this->noise->get_value();

        std::string dump_channels_full_path =
          dump_channels_path + "/N" + std::to_string(this->N) + "_" + noise_type + "_s" + noise.str() + ".pc";
        this->dump_best_channels(dump_channels_full_path);
    }
}

const std::vector<uint32_t>&
Pretransform_generator ::get_best_channels() const
{
    return best_channels;
}

void
Pretransform_generator ::check_noise()
{
    if (this->noise == nullptr)
    {
        std::stringstream message;
        message << "'noise' should not be nullptr.";
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
    }
    if (!this->noise->is_set())
    {
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, "'noise' is not set.");
    }
}

const std::map<uint32_t, std::vector<uint32_t>>&
Pretransform_generator::get_pre_transform() const
{
    return preTransform;
}

void
Pretransform_generator ::dump_best_channels(const std::string& dump_channels_full_path) const
{
    std::string noise_type = "unkn";
    if (this->noise != nullptr)
    {
        switch (this->noise->get_type())
        {
            case tools::Noise_type::SIGMA:
                noise_type = "awgn";
                break;
            case tools::Noise_type::EP:
                noise_type = "bec";
                break;
            default:
                break;
        };
    }

    std::ofstream file(dump_channels_full_path);

    if (!file.is_open())
    {
        std::stringstream message;
        message << "Impossible to open the '" << dump_channels_full_path << "' file to dump the Polar best channels.";
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
    }

    file << this->N << std::endl;
    file << noise_type << std::endl;
    file << std::fixed << std::setprecision(3) << this->noise->get_value() << std::endl;

    for (auto c : this->best_channels)
        file << c << " ";
    file << std::endl;
}
