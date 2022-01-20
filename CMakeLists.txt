# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.13)
project(perfetto-sdk-example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads)

# Define a static library for Perfetto. In this example, we expect the SDK
# (perfetto.cc and perfetto.h) to live in the top level sdk/ directory.
include_directories(third_party/perfetto)
add_library(perfetto STATIC third_party/perfetto/perfetto.cc)

############################ START #########################
# This section is experimental
# TODO: find out how is trace.gen.h/c generated, this is usefule to visualize the trace data
# by using trace.ParseFromArray https://github.com/google/perfetto/blob/v21/src/tracing/test/api_integrationtest.cc#L4427

# If you encounter some protobuf version issue on your system, define the protoc binary explicity
# set(Protobuf_PROTOC_EXECUTABLE /usr/bin/protoc)
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

# This is only for test purpose, because the protobuf parse functions are defined in perfetto_trace.pb.cc
include_directories(third_party/perfetto_proto)
add_library(perfetto_proto STATIC
    third_party/perfetto_proto/perfetto_trace.pb.cc
)
target_link_libraries(perfetto_proto ${PROTOBUF_LIBRARY})
############################ END #########################

# data generator
add_executable(data_generator_test
    data_generator_test.cc
    trace_categories.cc
    )
target_link_libraries(data_generator_test PUBLIC perfetto perfetto_proto
                      ${CMAKE_THREAD_LIBS_INIT})
