#include <sstream>
#include <streampu.hpp>

#include "Tools/Code/LDPC/Standard/5G/base_graph_selector_5G.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

base_graph_5G
aff3ct::tools::build_5G_base_graph(const int K, const int N)
{
    // Checking basic errors
    if (K > 8448 || K < 12)
    {
        std::stringstream message;
        message << "K = " << K << " is not valid (K must be <= 8448 and >= 12).";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }
    if (N > (316 * 384) || N < 0)
    {
        std::stringstream message;
        message << "N = " << N << " is not valid (N must be <= " << (316 * 384) << " and >= 0).";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    base_graph_5G base_graph;

    // Choosing the base graph according to K and R
    auto R = static_cast<float>(K) / static_cast<float>(N);
    if (K <= 292 || (K <= 3824 && R <= 0.67f) || R <= 0.25f)
    {
        base_graph.Bg = 2;
        base_graph.Kldpc = 10;
        base_graph.Nldpc = 52;
    }
    else
    {
        base_graph.Bg = 1;
        base_graph.Kldpc = 10;
        base_graph.Nldpc = 68;
    }

    // Dealing with edge cases
    if (base_graph.Bg == 1 && K > 8448)
    {
        std::stringstream message;
        message << "K = " << K << " is not valid for base graph BG1 (K must be <= 8448).";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }
    if (base_graph.Bg == 1 && R < 1 / 3.0f)
    {
        std::stringstream message;
        message << "K = " << K << " is not valid for base graph BG1 (R must be >= 1/3).";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }
    if (base_graph.Bg == 2 && K > 3824)
    {
        std::stringstream message;
        message << "K = " << K << " is not valid for base graph BG2 (K must be <= 3824).";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }
    if (base_graph.Bg == 2 && R < 1 / 5.0f)
    {
        std::stringstream message;
        message << "K = " << K << " is not valid for base graph BG2 (R must be >= 1/5).";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    // Choosing the Zc value
    int Kb;
    if (base_graph.Bg == 1)
        Kb = 22;
    else
    {
        if (K > 640)
            Kb = 10;
        else if (K > 560 && K <= 640)
            Kb = 8;
        else if (K > 192 && K <= 560)
            Kb = 8;
        else
            Kb = 6;
    }
    std::vector<std::vector<int>> Z_list = { { 2, 4, 8, 16, 32, 64, 128, 256 }, { 3, 6, 12, 24, 48, 96, 192, 384 },
                                             { 5, 10, 20, 40, 80, 160, 320 },   { 7, 14, 28, 56, 112, 224 },
                                             { 9, 18, 36, 72, 144, 288 },       { 11, 22, 44, 88, 176, 352 },
                                             { 13, 26, 52, 104, 208 },          { 15, 30, 60, 120, 240 } };

    int min_val = 100000, i = -1;
    for (auto& row_z : Z_list)
    {
        i++;
        for (auto& val_z : row_z)
        {
            int curr_k = Kb * val_z;
            if (curr_k >= K)
            {
                if (curr_k < min_val)
                {
                    min_val = curr_k;
                    base_graph.Zc = val_z;
                    base_graph.index_list = i;
                }
            }
        }
    }
    // Checking errors
    if (min_val == 100000)
    {
        std::stringstream message;
        message << "No valid Zc found for K = " << K << ".";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }
    // Updating Nldpc et Kldpc with the choosen Zc
    base_graph.Kldpc = base_graph.Kldpc * base_graph.Zc;
    base_graph.Nldpc = base_graph.Nldpc * base_graph.Zc;

    return base_graph;
}