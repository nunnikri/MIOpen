/*******************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2017 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/

#define MIOPEN

#include <miopen/config.h>
#include <miopen/convolution.hpp>
#include <miopen/db.hpp>
#include <miopen/env.hpp>
#include <miopen/find_solution.hpp>
#include <miopen/gcn_asm_utils.hpp>
#include <miopen/mlo_internal.hpp>
#include <miopen/mlo_utils.hpp>
#include <miopen/solver.hpp>
#include <miopen/readonlyramdb.hpp>
#include <miopen/datatype.hpp>
#include <miopen/version.h>
#include <miopen/stringutils.hpp>

#include <cmath>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>

MIOPEN_DECLARE_ENV_VAR(MIOPEN_DEBUG_GCN_ASM_KERNELS)
MIOPEN_DECLARE_ENV_VAR(MIOPEN_DEBUG_AMD_ROCM_PRECOMPILED_BINARIES)

namespace miopen {
namespace test {
boost::optional<std::string>& db_path_override()
{
    static boost::optional<std::string> path = boost::none;
    return path;
}
} // namespace test
} // namespace miopen

miopen::PerfDb mlo_construct_base::GetDb() const
{
    return {{db_path(), _search_params.GetUserPerfDbPath()}};
}

miopen::PerfDb miopen::GetDb(const ConvolutionContext& ctx)
{
    if(test::db_path_override())
        return {{*test::db_path_override(), *test::db_path_override()}};
    else
        return {{ctx.GetPerfDbPath(), ctx.GetUserPerfDbPath()}};
}

miopen::solver::ConvSolution
mlo_construct_direct2D_fusion::FindSolution(const std::vector<miopen::ConvSolver*>& solvers)
{
    return miopen::solver::SearchForSolution(solvers, _search_params);
}

static const auto& GetDirectSolvers()
{
    static const auto inst = std::vector<miopen::ConvSolver*>{{
        &miopen::StaticContainer<miopen::solver::ConvAsm3x3U>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvAsm1x1U>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvAsm1x1UV2>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvAsm5x10u2v2f1>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvAsm7x7c3h224w224k64u2v2p3q3f1>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvAsm5x10u2v2b1>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclDirectFwd11x11>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclDirectFwdGen>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclDirectFwd3x3>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclDirectFwd1x1>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclDirectFwd>::Instance(),
    }};
    return inst;
}

static const auto& GetImplicitGemmSolvers()
{
    static const auto inst = std::vector<miopen::ConvSolver*>{{
        &miopen::StaticContainer<miopen::solver::ConvHipImplicitGemmV4Fwd>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvHipImplicitGemmV4_1x1>::Instance(),
    }};
    return inst;
}

static const auto& GetWindogradSolvers()
{
    static const auto inst = std::vector<miopen::ConvSolver*>{{
        &miopen::StaticContainer<miopen::solver::ConvBinWinograd3x3U>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvBinWinogradRxSf3x2>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvBinWinogradRxS>::Instance(),
    }};
    return inst;
}

static const auto& GetWindogradWrWSolvers()
{
    static const auto inst = std::vector<miopen::ConvSolver*>{{
        &miopen::StaticContainer<miopen::solver::ConvBinWinogradRxS>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvWinograd3x3MultipassWrW>::Instance(),
    }};
    return inst;
}

static const auto& GetBwdWrW2DSolvers()
{
    static const auto inst = std::vector<miopen::ConvSolver*>{{
        &miopen::StaticContainer<miopen::solver::ConvAsmBwdWrW1x1>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvAsmBwdWrW3x3>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclBwdWrW2<1>>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclBwdWrW2<2>>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclBwdWrW2<4>>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclBwdWrW2<8>>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclBwdWrW2<16>>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclBwdWrW2NonTunable>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclBwdWrW53>::Instance(),
        &miopen::StaticContainer<miopen::solver::ConvOclBwdWrW1x1>::Instance(),
    }};
    return inst;
}

static const auto& GetFwdSCGemmSolvers()
{
    static const auto inst = std::vector<miopen::ConvSolver*>{
        &miopen::StaticContainer<miopen::solver::ConvSCGemmFwd<miopen::SCGemmOpFGemm>>::Instance(),
    };
    return inst;
}

std::vector<miopen::solver::ConvSolution>
FindAllDirectSolutions(const miopen::ConvolutionContext& ctx)
{
    return SearchForAllSolutions(GetDirectSolvers(), ctx);
}

std::vector<miopen::solver::ConvSolution>
FindAllImplicitGemmSolutions(const miopen::ConvolutionContext& ctx)
{
    return SearchForAllSolutions(GetImplicitGemmSolvers(), ctx);
}

std::vector<miopen::solver::ConvSolution>
FindAllWinogradSolutions(const miopen::ConvolutionContext& ctx)
{
    return SearchForAllSolutions(GetWindogradSolvers(), ctx);
}

miopen::solver::ConvSolution FindWinogradSolution(const miopen::ConvolutionContext& ctx)
{
    return SearchForSolution(GetWindogradSolvers(), ctx);
}

std::vector<miopen::solver::ConvSolution>
FindWinogradWrWAllSolutions(const miopen::ConvolutionContext& ctx)
{
    return SearchForAllSolutions(GetWindogradWrWSolvers(), ctx);
}

std::vector<miopen::solver::ConvSolution>
FindAllBwdWrW2DSolutions(const miopen::ConvolutionContext& ctx)
{
    return SearchForAllSolutions(GetBwdWrW2DSolvers(), ctx);
}

std::vector<miopen::solver::ConvSolution>
FindAllFwdSCGemmSolutions(const miopen::ConvolutionContext& ctx)
{
    return SearchForAllSolutions(GetFwdSCGemmSolvers(), ctx);
}

#if MIOPEN_BACKEND_OPENCL
static bool IsTokenWithin(const std::string& s, const char* delimiters, const std::string& find_tok)
{
    assert(delimiters);
    std::size_t cursor = 0;
    do
    {
        const std::size_t tok_begin = s.find_first_not_of(delimiters, cursor);
        if(tok_begin == std::string::npos)
        {
            break;
        }
        cursor            = s.find_first_of(delimiters, tok_begin);
        std::string token = (cursor == std::string::npos) ? s.substr(tok_begin)
                                                          : s.substr(tok_begin, cursor - tok_begin);
        if(token == find_tok)
        {
            return true;
        }
    } while(cursor != std::string::npos);
    return false;
}

static bool IsAmdRocmOpencl(const miopen::ConvolutionContext& context)
{
    const auto dev             = miopen::GetDevice(context.GetStream().GetStream());
    const auto platform        = miopen::GetDeviceInfo<CL_DEVICE_PLATFORM>(dev);
    const auto platform_vendor = miopen::GetPlatformInfo<CL_PLATFORM_VENDOR>(platform);
    if(platform_vendor != "Advanced Micro Devices, Inc.")
    {
        return false;
    }
    const auto device_vendor_id = miopen::GetDeviceInfo<CL_DEVICE_VENDOR_ID>(dev);
    if(device_vendor_id != 0x1002) // AMD
    {
        return false;
    }
    const auto driver_version = miopen::GetDeviceInfo<CL_DRIVER_VERSION>(dev);
    const char* delimiters    = " (),*";                    // Specific for ROCm OCL driver version.
    return IsTokenWithin(driver_version, delimiters, "LC"); // Lightning Compiler.
}
#endif // MIOPEN_BACKEND_OPENCL

static std::ostream& operator<<(std::ostream& os, const rocm_meta_version& rmv)
{
    switch(rmv)
    {
    case rocm_meta_version::Unknown: return os << "Unknown";
    case rocm_meta_version::AMDHSA_1_0: return os << "AMDHSA_1_0";
    }
    return os << "<Error>";
}

static rocm_meta_version DetectAmdRocmMetadataVersion(const miopen::ConvolutionContext& context)
{
#if MIOPEN_BACKEND_OPENCL
    const auto dev                     = miopen::GetDevice(context.GetStream().GetStream());
    const auto platform                = miopen::GetDeviceInfo<CL_DEVICE_PLATFORM>(dev);
    const std::string platform_version = miopen::GetPlatformInfo<CL_PLATFORM_VERSION>(
        platform); // e.g. "OpenCL 2.0 AMD-APP.internal (2334.0)"
    size_t num_begin      = platform_version.find('(');
    rocm_meta_version rmv = rocm_meta_version::Unknown;
    if(num_begin != std::string::npos)
    {
        // int num = std::stoi(platform_version.substr(num_begin + 1));
        rmv = rocm_meta_version::AMDHSA_1_0;
    }
#else
    /// \todo Rework this using clang-ocl.
    (void)context;
    rocm_meta_version rmv = rocm_meta_version::Default;
#endif // MIOPEN_BACKEND_OPENCL
    MIOPEN_LOG_I(
        "ROCm MD version "
        << rmv
        << ", MIOpen version " MIOPEN_STRINGIZE(MIOPEN_VERSION_MAJOR) "." MIOPEN_STRINGIZE(
               MIOPEN_VERSION_MINOR) "." MIOPEN_STRINGIZE(MIOPEN_VERSION_PATCH) "." MIOPEN_STRINGIZE(MIOPEN_VERSION_TWEAK));
    return rmv;
}

static bool mloIsAmdRocmOpencl(miopen::ConvolutionContext& context)
{
    static const bool ret_bool =
#if MIOPEN_BACKEND_OPENCL
        IsAmdRocmOpencl(context);
#else
        true;
#endif // MIOPEN_BACKEND_OPENCL
    if(ret_bool)
    {
        static const rocm_meta_version ret_rmv = DetectAmdRocmMetadataVersion(context);
        context.rmv                            = ret_rmv;
    }
    return ret_bool;
}

void miopen::ConvolutionContext::SetupFloats()
{
    if(IsFp32() || IsFp16() || IsBfp16())
    {
        general_compile_options += GetDataTypeKernelParams(in_data_type);
    }
    else
    {
        MIOPEN_LOG_W(
            "Unsupported data types configuration: " << miopen::GetDataTypeName(in_data_type) << "x"
                                                     << miopen::GetDataTypeName(weights_data_type)
                                                     << "x"
                                                     << miopen::GetDataTypeName(out_data_type));
    }
}

void mlo_construct_activ_lrn_pooling_common::setupFloats()
{
    if(_search_params.in_data_type == miopenFloat && _search_params.out_data_type == miopenFloat)
    {
        _search_params.general_compile_options += " -DMIOPEN_USE_FP32=1 -DMIOPEN_USE_FP16=0";
    }
    else if(_search_params.in_data_type == miopenHalf && _search_params.out_data_type == miopenHalf)
    {
        _search_params.general_compile_options += " -DMIOPEN_USE_FP32=0 -DMIOPEN_USE_FP16=1";
    }
    else
    {
        MIOPEN_LOG_W("Unsupported data types configuration: "
                     << miopen::GetDataTypeName(_search_params.in_data_type)
                     << "x"
                     << miopen::GetDataTypeName(_search_params.out_data_type));
    }
}

void miopen::ConvolutionContext::DetectRocm()
{
    // Detect assembly kernels
    use_binaries    = false;
    use_asm_kernels = false;
    rmv             = rocm_meta_version::Default;
    if(mloIsAmdRocmOpencl(*this))
    {
        use_asm_kernels =
            !miopen::IsDisabled(MIOPEN_DEBUG_GCN_ASM_KERNELS{}) && ValidateGcnAssembler();
#ifndef HIP_OC_FINALIZER
        use_binaries = !miopen::IsDisabled(MIOPEN_DEBUG_AMD_ROCM_PRECOMPILED_BINARIES{});
#endif
    }
}
