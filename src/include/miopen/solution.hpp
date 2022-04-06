/*******************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2022 Advanced Micro Devices, Inc.
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

#pragma once

#include <miopen/miopen.h>

#include <miopen/errors.hpp>
#include <miopen/object.hpp>
#include <miopen/problem.hpp>
#include <miopen/solver_id.hpp>
#include <miopen/tensor.hpp>

#include <boost/optional.hpp>

#include <unordered_map>

namespace miopen {

struct Handle;

struct Solution : miopenSolution
{
    struct RunInput
    {
        boost::optional<TensorDescriptor> descriptor;
        Data_t buffer;
    };

    std::size_t GetSize() const;
    void Save(char* data) const;
    void Load(const char* data, std::size_t size);
    float GetTime() const { return time; }
    void SetTime(float value) { time = value; }
    std::size_t GetWorkspaceSize() const { return workspace_required; }
    void SetWorkspaceSize(std::size_t value) { workspace_required = value; }
    solver::Id GetSolver() const { return solver; }
    void SetSolver(solver::Id value) { solver = value; }
    Problem GetProblem() const { return problem; }
    void SetProblem(Problem value) { problem = std::move(value); }

    void Run(Handle& handle,
             const std::unordered_map<miopenTensorName_t, RunInput>& inputs,
             Data_t workspace,
             size_t workspace_size);

private:
    float time;
    std::size_t workspace_required;
    solver::Id solver;
    Problem problem;
};

} // namespace miopen

inline std::ostream& operator<<(std::ostream& stream, const miopen::Solution& solution)
{
    // Todo: sane printing
    stream << &solution;
    return stream;
}

MIOPEN_DEFINE_OBJECT(miopenSolution, miopen::Solution);
