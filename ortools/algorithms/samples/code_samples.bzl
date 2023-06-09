# Copyright 2010-2022 Google LLC
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Helper macro to compile and test code samples."""

def code_sample_cc(name):
    native.cc_binary(
        name = name + "_cc",
        srcs = [name + ".cc"],
        deps = [
            "//ortools/algorithms:knapsack_solver_lib",
        ],
    )

    native.cc_test(
        name = name + "_cc_test",
        size = "small",
        srcs = [name + ".cc"],
        deps = [
            ":" + name + "_cc",
            "//ortools/algorithms:knapsack_solver_lib",
        ],
    )

def code_sample_java(name):
    native.java_test(
        name = name + "_java_test",
        size = "small",
        srcs = [name + ".java"],
        main_class = "com.google.ortools.algorithms.samples." + name,
        test_class = "com.google.ortools.algorithms.samples." + name,
        jvm_flags = select({
            "@platforms//os:windows": ["-Djava.library.path=../../../../java/com/google/ortools"],
            "//conditions:default": ["-Djava.library.path=ortools/java/com/google/ortools"],
        }),
        deps = [
            "//ortools/algorithms/java:knapsacksolver",
            "//ortools/java/com/google/ortools:Loader",
        ],
    )
