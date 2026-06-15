#include <fstream>
#include <sstream>
#include <streampu.hpp>

#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator_file.hpp"
#include "Tools/general_utils.h"

using namespace aff3ct::tools;

Pretransform_generator_file ::Pretransform_generator_file(const int K, const int N, const std::string& filename)
  : Pretransform_generator(K, N)
  , filename(filename)
{
}

Pretransform_generator_file ::Pretransform_generator_file(const int K, const int N)
  : Pretransform_generator(K, N)
  , filename("")
{
}

Pretransform_generator_file*
Pretransform_generator_file ::clone() const
{
    auto t = new Pretransform_generator_file(*this);
    return t;
}

void
Pretransform_generator_file ::evaluate()
{
    if (!load_pretransform_file(this->filename, this->preTransform, this->info_bits_loc))
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, "'" + filename + "' file does not exist.");
}

bool
Pretransform_generator_file ::load_channels_file(const std::string& filename, std::vector<uint32_t>& best_channels)
{
    std::ifstream in_code(filename.c_str());

    if (in_code.is_open())
    {
        std::string trash;
        in_code >> trash; // N

        try
        {
            std::stoi(trash);
        }
        catch (std::exception&)
        {
            std::stringstream message;
            message << "'std::stoi' did not work, something went wrong when reading the file.";
            throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
        }

        if ((size_t)std::stoi(trash) != best_channels.size())
        {
            std::stringstream message;
            message << "'trash' has to be equal to 'N' ('trash' = " << trash << ", 'N' = " << this->N << ").";
            throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
        }

        in_code >> trash; // type
        in_code >> trash; // sigma

        for (unsigned i = 0; i < best_channels.size(); i++)
            in_code >> best_channels[i];

        in_code.close();
        return true;
    }
    else
        return false;
}

bool
Pretransform_generator_file::load_pretransform_file(const std::string& filename,
                                                    std::map<uint32_t, std::vector<uint32_t>>& pretransform,
                                                    std::vector<uint32_t>& info_bits_loc)
{
    std::ifstream in_pretransform(filename);

    if (in_pretransform.is_open())
    {
        std::string line;
        // int k = 0;
        while (!in_pretransform.eof())
        {

            tools::getline(in_pretransform, line);

            auto values = tools::split(line);
            if (values.size() > 0)
            {
                unsigned int current_key = std::stoi(values.at(0));
                // std::cout << "size: " << current_key << std::endl;
                // if (k < this->K) info_bits_loc[k++] = current_key;
                // this->pre_tranform[current_key] = std::vector<unsigned int>(values.size() - 1);
                pretransform[current_key] = std::vector<uint32_t>(values.size() - 1);
                int idx = 0;
                for (auto it = values.begin() + 1; it != values.end(); ++it)
                {
                    // pretransform[current_key].push_back(std::stoi(*it));
                    pretransform[current_key][idx++] = std::stoi(*it);
                    // this->pre_tranform[current_key].at(idx++) = std::stoi(*it);
                    // std::cout << *it << ",";
                }
                // std::cout << std::endl;
            }
        }
        in_pretransform.close();
        // for (const auto& vk : pretransform)
        // {
        //     std::cout << "Key: " << vk.first << ":";
        //     for (const auto& v : vk.second)
        //         std::cout << v << ",";
        //     std::cout << std::endl;
        // }

        return true;
    }
    else
        return false;
}

void
Pretransform_generator_file ::check_noise()
{
    Pretransform_generator::check_noise();
}
