/*ckwg +29
 * Copyright 2015 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * \brief compute a match matrix from a track file
 */

#include <iostream>
#include <fstream>
#include <exception>
#include <string>
#include <vector>

#include <maptk/match_matrix.h>
#include <vital/exceptions.h>
#include <vital/io/track_set_io.h>

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;


/// Report usage message to std::cerr
void usage(int const& argc, char const* argv[],
           bpo::options_description const& opt_desc,
           bpo::variables_map const& vm)
{
    std::cerr << "Usage: " << argv[0] << " track_file match_matrix_file\n"
              << "\n"
              << "Read a track file and compute the match matrix\n"
              << "\n"
              << "Options:\n"
              << opt_desc << std::endl;
}


void
write_match_matrix(std::ostream& os,
                   const Eigen::SparseMatrix<unsigned int>& mm,
                   const std::vector<kwiver::vital::frame_id_t>& frames)
{
  for( unsigned i=0; i<frames.size(); ++i )
  {
    os << frames[i] << " ";
  }
  os << std::endl;
  os << mm << std::endl;
}


static int maptk_main(int argc, char const* argv[])
{
  using namespace kwiver;
  //
  // CLI Options (boost)
  //
  bpo::positional_options_description pos_desc;
  pos_desc.add("input-tracks", 1);
  pos_desc.add("output-matrix", 1);

  bpo::options_description opt_desc;
  opt_desc.add_options()
    ("input-tracks,i", "input track file")
    ("output-matrix,o", "output match matrix file")
    ("help,h", "output help message and exit");
  bpo::variables_map vm;
  // try to parse the command-line
  try
  {
    bpo::store(bpo::command_line_parser(argc, argv).
               options(opt_desc).positional(pos_desc).run(), vm);
  }
  catch (bpo::unknown_option const& e)
  {
    std::cerr << e.what() << std::endl
              << std::endl;
    usage(argc, argv, opt_desc, vm);
    return EXIT_FAILURE;
  }
  bpo::notify(vm);

  if(vm.count("help"))
  {
    usage(argc, argv, opt_desc, vm);
    return EXIT_SUCCESS;
  }

  if(!vm.count("input-tracks"))
  {
    usage(argc, argv, opt_desc, vm);
    return EXIT_FAILURE;
  }

  // test the output file
  if(vm.count("output-matrix"))
  {
    vital::path_t outfile(vm["output-matrix"].as<std::string>());

    // If the given path is a directory, we obviously can't write to it.
    if(bfs::is_directory(outfile))
    {
      throw vital::file_write_exception(outfile, "Path given is a directory, "
                                                 "can not write file.");
    }

    // Check that the directory of the given file path exists,
    // creating necessary directories where needed.
    vital::path_t parent_dir = bfs::absolute(outfile.parent_path());
    if(!bfs::is_directory(parent_dir))
    {
      if(!bfs::create_directories(parent_dir))
      {
        throw vital::file_write_exception(parent_dir, "Attempted directory creation, "
                                                      "but no directory created! No "
                                                      "idea what happened here...");
      }
    }
    std::ofstream ofs(outfile.string().c_str());
  }

  // load the tracks
  std::string infile = vm["input-tracks"].as<std::string>();
  std::cout << "loading: "<< infile << std::endl;
  vital::track_set_sptr tracks = vital::read_track_file(infile);

  // compute the match matrix
  std::cout << "computing matching matrix" <<std::endl;
  std::vector<vital::frame_id_t> frames;
  Eigen::SparseMatrix<unsigned int> mm = maptk::match_matrix(tracks, frames);

  // write output
  if(vm.count("output-matrix"))
  {
    vital::path_t outfile(vm["output-matrix"].as<std::string>());
    std::cout << "writing to: "<< outfile << std::endl;
    std::ofstream ofs(outfile.string().c_str());
    write_match_matrix(ofs, mm, frames);
  }
  else
  {
    write_match_matrix(std::cout, mm, frames);
  }

  return EXIT_SUCCESS;
}


int main(int argc, char const* argv[])
{
  try
  {
    return maptk_main(argc, argv);
  }
  catch (std::exception const& e)
  {
    std::cerr << "Exception caught: " << e.what() << std::endl;

    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Unknown exception caught" << std::endl;

    return EXIT_FAILURE;
  }
}