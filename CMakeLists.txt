cmake_minimum_required(VERSION 3.13)  # CMake version check
project(vargeno)               # Create project "vargeno"
set(CMAKE_CXX_STANDARD 14)            # Enable c++14 standard

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES src/qv.cc)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(vargeno ${SOURCE_FILES}
        src/allsome_util.cc
        src/BF.cc
        src/bit_check.cc
        src/BloomTree.cc
        src/Count.cc
        src/dict_filt.c
        src/dictgen.c
        src/fasta_parser.c
        src/gbf.cc
        src/generate_bf.cc
        src/kmer_counting.cc
        src/kmerc.cc
        src/Kmers.cc
        src/pileup.c
        src/sbt_util.cc
        src/test_bf.cc
        src/util.c
        src/vartype.cc
        )