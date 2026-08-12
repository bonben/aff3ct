#include <fstream>
#include <sstream>
#include <streampu.hpp>
#include <sys/types.h>

#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator_file.hpp"
#include "Tools/general_utils.h"

using namespace aff3ct::tools;

Pretransform_generator_file::Pretransform_generator_file(const int K, const int N, const std::string& filename)
  : Pretransform_generator(K, N)
  , filename(filename)
{
}

Pretransform_generator_file::Pretransform_generator_file(const int K, const int N)
  : Pretransform_generator(K, N)
  , filename("")
{
}

Pretransform_generator_file*
Pretransform_generator_file::clone() const
{
    auto t = new Pretransform_generator_file(*this);
    return t;
}

void
Pretransform_generator_file::evaluate()
{
    if (!load_pretransform_file(this->filename, this->preTransform, this->info_bits_loc))
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, "'" + filename + "' file does not exist.");
}

bool
Pretransform_generator_file::load_channels_file(const std::string& filename, std::vector<uint32_t>& best_channels)
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
        pretransform.clear();
        while (!in_pretransform.eof())
        {

            tools::getline(in_pretransform, line);

            auto values = tools::split(line);
            if (values.empty()) continue;

            uint32_t current_key;
            try
            {
                current_key = std::stoi(values.at(0));
            }
            catch (const std::exception& e)
            {
                throw spu::tools::runtime_error(
                  __FILE__, __LINE__, __func__, "Failed to convert string to uint32_t: " + std::string(e.what()));
            }

            if ((int)current_key >= this->N)
            {
                std::stringstream message;
                message << "Pre-transform key is out of bounds: " + std::to_string(current_key) +
                             " is greater than or equal to N: " + std::to_string(this->N);
                throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
            }

            pretransform[current_key] = std::vector<uint32_t>(values.size() - 1);
            int idx = 0;
            for (auto it = values.begin() + 1; it != values.end(); ++it)
            {
                uint32_t index;
                try
                {
                    index = (uint32_t)std::stoi(*it);
                }
                catch (const std::exception& e)
                {
                    throw spu::tools::runtime_error(
                      __FILE__, __LINE__, __func__, "Failed to convert string to uint32_t: " + std::string(e.what()));
                }

                if ((int)index >= this->K)
                {
                    std::stringstream message;
                    message << "Pre-transform index is out of bounds: " + std::to_string(index) +
                                 " is greater than or equal to K: " + std::to_string(this->K);
                    throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
                }
                pretransform[current_key][idx++] = index;
            }
        }
        in_pretransform.close();

        return true;
    }
    else
        return false;
}

void
Pretransform_generator_file::check_noise()
{
    Pretransform_generator::check_noise();
}
