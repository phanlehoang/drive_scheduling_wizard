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

#include "ortools/sat/parameters_validation.h"

#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "ortools/sat/sat_parameters.pb.h"

namespace operations_research {
namespace sat {

#define TEST_IN_RANGE(name, min, max)                                       \
  if (params.name() < min || params.name() > max) {                         \
    return absl::StrCat("parameter '", #name, "' should be in [", min, ",", \
                        max, "]. Current value is ", params.name());        \
  }

#define TEST_NON_NEGATIVE(name)                                         \
  if (params.name() < 0) {                                              \
    return absl::StrCat("Parameters ", #name, " must be non-negative"); \
  }

#define TEST_POSITIVE(name)                                         \
  if (params.name() <= 0) {                                         \
    return absl::StrCat("Parameters ", #name, " must be positive"); \
  }

#define TEST_NOT_NAN(name)                                 \
  if (std::isnan(params.name())) {                         \
    return absl::StrCat("parameter '", #name, "' is NaN"); \
  }

#define TEST_IS_FINITE(name)                                             \
  if (!std::isfinite(params.name())) {                                   \
    return absl::StrCat("parameter '", #name, "' is NaN or not finite"); \
  }

std::string ValidateParameters(const SatParameters& params) {
  // Test that all floating point parameters are not NaN or +/- infinity.
  TEST_IS_FINITE(random_polarity_ratio);
  TEST_IS_FINITE(random_branches_ratio);
  TEST_IS_FINITE(initial_variables_activity);
  TEST_IS_FINITE(clause_cleanup_ratio);
  TEST_IS_FINITE(pb_cleanup_ratio);
  TEST_IS_FINITE(variable_activity_decay);
  TEST_IS_FINITE(max_variable_activity_value);
  TEST_IS_FINITE(glucose_max_decay);
  TEST_IS_FINITE(glucose_decay_increment);
  TEST_IS_FINITE(clause_activity_decay);
  TEST_IS_FINITE(max_clause_activity_value);
  TEST_IS_FINITE(restart_dl_average_ratio);
  TEST_IS_FINITE(restart_lbd_average_ratio);
  TEST_IS_FINITE(blocking_restart_multiplier);
  TEST_IS_FINITE(strategy_change_increase_ratio);
  TEST_IS_FINITE(absolute_gap_limit);
  TEST_IS_FINITE(relative_gap_limit);
  TEST_IS_FINITE(log_frequency_in_seconds);
  TEST_IS_FINITE(model_reduction_log_frequency_in_seconds);
  TEST_IS_FINITE(probing_deterministic_time_limit);
  TEST_IS_FINITE(presolve_probing_deterministic_time_limit);
  TEST_IS_FINITE(propagation_loop_detection_factor);
  TEST_IS_FINITE(merge_no_overlap_work_limit);
  TEST_IS_FINITE(merge_at_most_one_work_limit);
  TEST_IS_FINITE(min_orthogonality_for_lp_constraints);
  TEST_IS_FINITE(cut_max_active_count_value);
  TEST_IS_FINITE(cut_active_count_decay);
  TEST_IS_FINITE(shaving_search_deterministic_time);
  TEST_IS_FINITE(mip_max_bound);
  TEST_IS_FINITE(mip_var_scaling);
  TEST_IS_FINITE(mip_wanted_precision);
  TEST_IS_FINITE(mip_check_precision);
  TEST_IS_FINITE(mip_max_valid_magnitude);
  TEST_IS_FINITE(mip_drop_tolerance);

  TEST_NOT_NAN(max_time_in_seconds);
  TEST_NOT_NAN(max_deterministic_time);

  // TODO(user): Consider using annotations directly in the proto for these
  // validation. It is however not open sourced.
  TEST_IN_RANGE(mip_max_activity_exponent, 1, 62);
  TEST_IN_RANGE(mip_max_bound, 0, 1e17);
  TEST_IN_RANGE(solution_pool_size, 1, std::numeric_limits<int32_t>::max());

  TEST_POSITIVE(glucose_decay_increment_period);

  TEST_NON_NEGATIVE(mip_wanted_precision);
  TEST_NON_NEGATIVE(max_time_in_seconds);
  TEST_NON_NEGATIVE(max_deterministic_time);
  TEST_NON_NEGATIVE(new_constraints_batch_size);
  TEST_NON_NEGATIVE(num_workers);
  TEST_NON_NEGATIVE(num_search_workers);
  TEST_NON_NEGATIVE(min_num_lns_workers);
  TEST_NON_NEGATIVE(interleave_batch_size);
  TEST_NON_NEGATIVE(probing_deterministic_time_limit);
  TEST_NON_NEGATIVE(presolve_probing_deterministic_time_limit);

  if (params.enumerate_all_solutions() &&
      (params.num_search_workers() > 1 || params.num_workers() > 1)) {
    return "Enumerating all solutions does not work in parallel";
  }

  if (params.enumerate_all_solutions() &&
      (!params.subsolvers().empty() || !params.extra_subsolvers().empty() ||
       !params.ignore_subsolvers().empty())) {
    return "Enumerating all solutions does not work with custom subsolvers";
  }

  if (params.num_search_workers() >= 1 && params.num_workers() >= 1) {
    return "Do not specify both num_search_workers and num_workers";
  }

  if (params.enumerate_all_solutions() && params.interleave_search()) {
    return "Enumerating all solutions does not work with interleaved search";
  }

  absl::flat_hash_set<std::string> valid_subsolvers({
      "auto",
      "core_default_lp",
      "core_max_lp",
      "core_or_no_lp",
      "core",
      "default_lp",
      "default",
      "fixed",
      "lb_tree_search",
      "less_encoding",
      "max_hs",
      "max_lp",
      "no_lp",
      "objective_lb_search_max_lp",
      "objective_lb_search_no_lp",
      "objective_lb_search",
      "probing_max_lp",
      "probing_no_lp",
      "probing",
      "quick_restart_max_lp",
      "quick_restart_no_lp",
      "quick_restart",
      "reduced_costs",
  });
  for (const SatParameters& new_subsolver : params.subsolver_params()) {
    if (new_subsolver.name().empty()) {
      return "New subsolver parameter defined without a name";
    }
    valid_subsolvers.insert(new_subsolver.name());
  }

  for (const std::string& subsolver : params.subsolvers()) {
    if (!valid_subsolvers.contains(subsolver)) {
      return absl::StrCat("subsolver \'", subsolver, "\' is not valid");
    }
  }

  for (const std::string& subsolver : params.extra_subsolvers()) {
    if (!valid_subsolvers.contains(subsolver)) {
      return absl::StrCat("subsolver \'", subsolver, "\' is not valid");
    }
  }

  for (const std::string& subsolver : params.ignore_subsolvers()) {
    if (!valid_subsolvers.contains(subsolver)) {
      return absl::StrCat("subsolver \'", subsolver, "\' is not valid");
    }
  }
  return "";
}

#undef TEST_IN_RANGE
#undef TEST_POSITIVE
#undef TEST_NON_NEGATIVE
#undef TEST_NOT_NAN
#undef TEST_IS_FINITE

}  // namespace sat
}  // namespace operations_research
