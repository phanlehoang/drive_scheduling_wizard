// Copyright 2010-2022 Google LLC
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A pybind11 wrapper for pdlp.

#include <optional>
#include <stdexcept>
#include <utility>

#include "Eigen/Core"
#include "Eigen/SparseCore"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "ortools/linear_solver/linear_solver.pb.h"
#include "ortools/pdlp/primal_dual_hybrid_gradient.h"
#include "ortools/pdlp/quadratic_program.h"
#include "ortools/pdlp/quadratic_program_io.h"
#include "ortools/pdlp/solvers.pb.h"
#include "pybind11/eigen.h"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/stl.h"

namespace pdlp = ::operations_research::pdlp;
using ::operations_research::MPModelProto;
using ::operations_research::pdlp::QuadraticProgram;
using ::pybind11::arg;

// TODO(user): The interface uses serialized protos because of issues building
// pybind11_protobuf. See
// https://github.com/protocolbuffers/protobuf/issues/9464. After
// pybind11_protobuf is working, this workaround can be removed.

// A mirror of pdlp::SolverResult except with a serialized SolveLog.
struct PywrapSolverResult {
  Eigen::VectorXd primal_solution;
  Eigen::VectorXd dual_solution;
  Eigen::VectorXd reduced_costs;
  pybind11::bytes solve_log_str;
};

PYBIND11_MODULE(pywrap_pdlp, m) {
  // ---------------------------------------------------------------------------
  // quadratic_program.h
  // ---------------------------------------------------------------------------

  // It's ok to read and assign to the fields of QuadraticProgram. Attempts to
  // mutate the fields will likely fail silently because of the copies back and
  // forth from Python and C++.
  pybind11::class_<QuadraticProgram>(m, "QuadraticProgram")
      .def(pybind11::init<>())
      .def("resize_and_initialize", &QuadraticProgram::ResizeAndInitialize)
      .def("apply_objective_scaling_and_offset",
           &QuadraticProgram::ApplyObjectiveScalingAndOffset)
      .def_readwrite("objective_vector", &QuadraticProgram::objective_vector)
      .def_readwrite("constraint_matrix", &QuadraticProgram::constraint_matrix)
      .def_readwrite("constraint_lower_bounds",
                     &QuadraticProgram::constraint_lower_bounds)
      .def_readwrite("constraint_upper_bounds",
                     &QuadraticProgram::constraint_upper_bounds)
      .def_readwrite("variable_lower_bounds",
                     &QuadraticProgram::variable_lower_bounds)
      .def_readwrite("variable_upper_bounds",
                     &QuadraticProgram::variable_upper_bounds)
      .def_readwrite("problem_name", &QuadraticProgram::problem_name)
      .def_readwrite("variable_names", &QuadraticProgram::variable_names)
      .def_readwrite("constraint_names", &QuadraticProgram::constraint_names)
      .def_readwrite("objective_offset", &QuadraticProgram::objective_offset)
      .def_readwrite("objective_scaling_factor",
                     &QuadraticProgram::objective_scaling_factor)
      // It appears that pybind11/eigen.h provides only a C++ -> Python
      // converter for DiagonalMatrix, so we can't expose objective_matrix as a
      // readwrite field. Below are extra methods for setting the objective
      // matrix.
      .def_readonly("objective_matrix", &QuadraticProgram::objective_matrix)
      .def("set_objective_matrix_diagonal",
           [](QuadraticProgram& qp,
              const Eigen::VectorXd& objective_matrix_diagonal) {
             qp.objective_matrix.emplace();
             qp.objective_matrix->diagonal() = objective_matrix_diagonal;
           })
      .def("clear_objective_matrix",
           [](QuadraticProgram& qp) { qp.objective_matrix.reset(); });

  m.def("validate_quadratic_program_dimensions",
        [](const QuadraticProgram& qp) {
          const absl::Status status =
              pdlp::ValidateQuadraticProgramDimensions(qp);
          if (status.ok()) {
            return;
          } else {
            throw std::invalid_argument(absl::StrCat(status.message()));
          }
        });

  m.def("is_linear_program", &pdlp::IsLinearProgram);

  m.def(
      "qp_from_mpmodel_proto",
      [](absl::string_view proto_str, bool relax_integer_variables,
         bool include_names) {
        MPModelProto proto;
        if (!proto.ParseFromString(std::string(proto_str))) {
          throw std::invalid_argument("Unable to parse input proto");
        }
        absl::StatusOr<QuadraticProgram> qp = pdlp::QpFromMpModelProto(
            proto, relax_integer_variables, include_names);
        if (qp.ok()) {
          return *qp;
        } else {
          throw std::invalid_argument(absl::StrCat(qp.status().message()));
        }
      },
      arg("proto_str"), arg("relax_integer_variables"),
      arg("include_names") = false);

  m.def("qp_to_mpmodel_proto", [](const QuadraticProgram& qp) {
    absl::StatusOr<MPModelProto> proto = pdlp::QpToMpModelProto(qp);
    if (proto.ok()) {
      return pybind11::bytes(proto->SerializeAsString());
    } else {
      throw std::invalid_argument(absl::StrCat(proto.status().message()));
    }
  });

  // ---------------------------------------------------------------------------
  // quadratic_program_io.h
  // ---------------------------------------------------------------------------

  m.def("read_quadratic_program_or_die", &pdlp::ReadQuadraticProgramOrDie,
        arg("filename"), arg("include_names") = false);

  // ---------------------------------------------------------------------------
  // primal_dual_hybrid_gradient.h
  // ---------------------------------------------------------------------------

  pybind11::class_<pdlp::PrimalAndDualSolution>(m, "PrimalAndDualSolution")
      .def(pybind11::init<>())
      .def_readwrite("primal_solution",
                     &pdlp::PrimalAndDualSolution::primal_solution)
      .def_readwrite("dual_solution",
                     &pdlp::PrimalAndDualSolution::dual_solution);

  pybind11::class_<PywrapSolverResult>(m, "SolverResult")
      .def(pybind11::init<>())
      .def_readwrite("primal_solution", &PywrapSolverResult::primal_solution)
      .def_readwrite("dual_solution", &PywrapSolverResult::dual_solution)
      .def_readwrite("reduced_costs", &PywrapSolverResult::reduced_costs)
      .def_readwrite("solve_log_str", &PywrapSolverResult::solve_log_str);

  // TODO(user): Expose interrupt_solve and iteration_stats_callback.
  m.def(
      "primal_dual_hybrid_gradient",
      [](QuadraticProgram qp, absl::string_view params_str,
         std::optional<pdlp::PrimalAndDualSolution> initial_solution) {
        pdlp::PrimalDualHybridGradientParams params;
        if (!params.ParseFromString(std::string(params_str))) {
          throw std::invalid_argument("Unable to parse input params");
        }
        pdlp::SolverResult result = pdlp::PrimalDualHybridGradient(
            std::move(qp), params, std::move(initial_solution));
        return PywrapSolverResult{
            .primal_solution = std::move(result.primal_solution),
            .dual_solution = std::move(result.dual_solution),
            .reduced_costs = std::move(result.reduced_costs),
            .solve_log_str = result.solve_log.SerializeAsString()};
      },
      arg("qp"), arg("params"), arg("initial_solution") = std::nullopt);
}
