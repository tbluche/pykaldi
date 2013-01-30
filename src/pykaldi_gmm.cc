// Copyright 2009-2011  Microsoft Corporation - modified
// Copyright 2012  Johns Hopkins University (Author: Daniel Povey) - modified
// Copyright 2013 T. Bluche
// 
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


int gmm_info(std::string model_in_filename){
  
  try {
    
    using namespace kaldi;
    typedef kaldi::int32 int32;
    
    AmDiagGmm am_gmm;
    TransitionModel trans_model;
    {
      bool binary_read;
      Input ki(model_in_filename, &binary_read);
      trans_model.Read(ki.Stream(), binary_read);
      am_gmm.Read(ki.Stream(), binary_read);
      ki.Close();
    }

    std::cout << "number of phones " << trans_model.GetPhones().size() << '\n';
    std::cout << "number of pdfs " << trans_model.NumPdfs() << '\n';
    std::cout << "number of transition-ids " << trans_model.NumTransitionIds()
              << '\n';
    std::cout << "number of transition-states "
              << trans_model.NumTransitionStates() << '\n';

    std::cout << "feature dimension " << am_gmm.Dim() << '\n';
    std::cout << "number of gaussians " << am_gmm.NumGauss() << '\n';
    
    
    return 0;
    
  } catch(const std::exception &e) {
    std::cerr << e.what() << '\n';
    return -1;
  }
}


bp::dict gmm_compute_likes(std::string model_in_filename, std::string feature_rspecifier) {
  bp::dict d;
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    using fst::SymbolTable;
    using fst::VectorFst;
    using fst::StdArc;

    AmDiagGmm am_gmm;
    {
      bool binary;
      TransitionModel trans_model;  // not needed.
      Input ki(model_in_filename, &binary);
      trans_model.Read(ki.Stream(), binary);
      am_gmm.Read(ki.Stream(), binary);
    }
    
    SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);

    int32 num_done = 0;
    for (; !feature_reader.Done(); feature_reader.Next()) {
      std::string key = feature_reader.Key();
      const Matrix<BaseFloat> &features (feature_reader.Value());
      Matrix<BaseFloat> loglikes(features.NumRows(), am_gmm.NumPdfs());
      for (int32 i = 0; i < features.NumRows(); i++) {
        for (int32 j = 0; j < am_gmm.NumPdfs(); j++) {
          SubVector<BaseFloat> feat_row(features, i);
          loglikes(i, j) = am_gmm.LogLikelihood(j, feat_row);
        }
      }
      d[key]=Matrix2List(loglikes);
      num_done++;
    }

    KALDI_LOG << "gmm-compute-likes: computed likelihoods for " << num_done
              << " utterances.";
    return d;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return d;
  }
}

void PyKaldi_ExportGmm() {
    using namespace boost::python;
    def("gmm_info", &gmm_info);
    def("gmm_compute_likes", &gmm_compute_likes);
}