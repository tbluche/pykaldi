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


#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/python.hpp>

namespace bp = boost::python;



using namespace kaldi;
bp::list Matrix2List(kaldi::Matrix<float> mat){
  bp::list ans;
  for (int32 i=0; i<mat.NumRows(); i++){
    bp::list tmp;
    for (int32 j=0; j<mat.NumCols(); j++){
      tmp.append(mat(i,j));
    }
    ans.append(tmp);
  }
  return ans;
}


kaldi::Matrix<float> List2MatrixFloat(bp::list mat){
  int r = bp::len(mat);
  int c = bp::len(mat[0]);
  Matrix<float> ans(r, c);
  for (int32 i = 0; i < r; i++) {
    for (int32 j = 0; j < c; j++) {
      ans(i, j) = bp::extract<float>(mat[i][j]);
    }
  }
  return ans;
}

void TestList2MatrixFloat(bp::list mat){
  kaldi::Matrix<float> m = List2MatrixFloat(mat);
//   return Matrix2List(m);
}

void PyKaldi_ExportUtils() {
  
  using namespace boost::python;
  def("tlm", &TestList2MatrixFloat);
  
}