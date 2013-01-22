// Copyright 2012  Johns Hopkins University (Author: Daniel Povey)

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.


#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "matrix/kaldi-matrix.h"
#include "hmm/transition-model.h"
#include "gmm/am-diag-gmm.h"
#include "fstext/fstext-lib.h"
#include "util/timer.h"


#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/python.hpp>

namespace bp = boost::python;


bp::list Matrix2List(kaldi::Matrix<float> mat);

bp::dict feat_to_len(std::string rspecifier) {
  bp::dict d;
  try {
    using namespace kaldi;
    
    SequentialBaseFloatMatrixReader kaldi_reader(rspecifier);
    for (; !kaldi_reader.Done(); kaldi_reader.Next())
      d[kaldi_reader.Key()]=kaldi_reader.Value().NumRows(); //
    
    return d;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return d;
  }
}


bp::dict feature_provider(std::string rspecifier) {
  bp::dict d;
  try {
    using namespace kaldi;
    
    SequentialBaseFloatMatrixReader kaldi_reader(rspecifier);
    for (; !kaldi_reader.Done(); kaldi_reader.Next())
      d[kaldi_reader.Key()]=Matrix2List(kaldi_reader.Value()); //.NumRows()
    
    return d;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return d;
  }
}





void PyKaldi_ExportFeatures() {
    using namespace boost::python;
    def("feat_to_len",&feat_to_len);
    def("feature_provider",&feature_provider);
}