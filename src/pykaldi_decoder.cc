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



// Kaldi includes
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "matrix/kaldi-matrix.h"
#include "hmm/transition-model.h"
#include "gmm/am-diag-gmm.h"
#include "fstext/fstext-lib.h"
#include "util/timer.h"
#include "decoder/faster-decoder.h"
#include "decoder/decodable-am-diag-gmm.h"
#include "lat/kaldi-lattice.h" // for CompactLatticeArc
#include "decoder/decodable-matrix.h"


// Std includes
#include <string>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/python.hpp>





namespace bp = boost::python;



// Util functions
kaldi::Matrix<float> List2MatrixFloat(bp::list mat);



// Copied from Kaldi sources
namespace kaldi {

fst::Fst<fst::StdArc> *ReadNetwork(std::string filename) {
  // read decoding network FST
  Input ki(filename); // use ki.Stream() instead of is.
  if (!ki.Stream().good()) KALDI_ERR << "Could not open decoding-graph FST "
                                      << filename;

  fst::FstHeader hdr;
  if (!hdr.Read(ki.Stream(), "<unknown>")) {
    KALDI_ERR << "Reading FST: error reading FST header.";
  }
  if (hdr.ArcType() != fst::StdArc::Type()) {
    KALDI_ERR << "FST with arc type " << hdr.ArcType() << " not supported.\n";
  }
  fst::FstReadOptions ropts("<unspecified>", &hdr);

  fst::Fst<fst::StdArc> *decode_fst = NULL;

  if (hdr.FstType() == "vector") {
    decode_fst = fst::VectorFst<fst::StdArc>::Read(ki.Stream(), ropts);
  } else if (hdr.FstType() == "const") {
    decode_fst = fst::ConstFst<fst::StdArc>::Read(ki.Stream(), ropts);
  } else {
    KALDI_ERR << "Reading FST: unsupported FST type: " << hdr.FstType();
  }
  if (decode_fst == NULL) { // fst code will warn.
    KALDI_ERR << "Error reading FST (after reading header).";
    return NULL;
  } else {
    return decode_fst;
  }
}

}



// FIXME move to utils
template<class T>
bp::list std_vector_to_py_list(const std::vector<T>& v)
{
    bp::object get_iter = bp::iterator<std::vector<T> >();
    bp::object iter = get_iter(v);
    bp::list l(iter);
    return l;
};


template<class T>
struct VecToList
{
    static PyObject* convert(const std::vector<T>& vec)
    {
        boost::python::list* l = new boost::python::list();
        for(size_t i = 0; i < vec.size(); i++)
            (*l).append(vec[i]);

        return l->ptr();
    }
};







typedef boost::shared_ptr<kaldi::FasterDecoder> faster_decoder_ptr;
typedef boost::shared_ptr<kaldi::TransitionModel> trans_model_ptr;
typedef boost::shared_ptr<kaldi::AmDiagGmm> am_gmm_ptr;
typedef boost::shared_ptr<fst::SymbolTable> sym_table_ptr;



bp::dict decoder_faster(
	    std::string model_in_filename,
	    std::string fst_in_filename,
	    std::string word_syms_filename = "",
	    float ac_scale = 0.1,
	    bool b_allow_partial = true,
	    float beam = 16.0,
	    bool save_am = false
			      ) {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    using fst::SymbolTable;
    using fst::VectorFst;
    using fst::StdArc;

    bp::dict d;
    d["acoustic_scale"] = ac_scale;     // param
    d["beam"]           = beam;         // param
    
    bool binary = true;
    
    bool allow_partial = b_allow_partial;
    BaseFloat acoustic_scale = ac_scale;
    
    FasterDecoderOptions decoder_opts;
    decoder_opts.beam = beam;
    
    

    // Read word symbol table
    sym_table_ptr symTable (fst::SymbolTable::ReadText(word_syms_filename));
//     fst::SymbolTable *word_syms = NULL;
//     if (word_syms_filename != "") {
//       word_syms = fst::SymbolTable::ReadText(word_syms_filename);
//       symTable = fst::SymbolTable::ReadText(word_syms_filename) ;
//       if (!word_syms)
//         KALDI_ERR << "Could not read symbol table from file "<<word_syms_filename;
//     }
    
    
    // Read decode FST (like HCLG)
    VectorFst<StdArc> *decode_fst = NULL;
    {
      Input ki(fst_in_filename.c_str());
      decode_fst =
          VectorFst<StdArc>::Read(ki.Stream(), fst::FstReadOptions(fst_in_filename));
      if (decode_fst == NULL) // fst code will warn.
        exit(1);
    }
    
    // Create FasterDecoder
    faster_decoder_ptr decoderPtr (new FasterDecoder(*decode_fst, decoder_opts));
    
    
    trans_model_ptr transModelPtr ( new TransitionModel() );
    am_gmm_ptr      amGmmPtr      ( new AmDiagGmm()       );
    
    // Read transition model
    Input ki(model_in_filename, &binary);
    (*transModelPtr).Read(ki.Stream(), binary);
    if (save_am) {
      (*amGmmPtr).Read(ki.Stream(), binary);
    }
    ki.Close();
    
    
    // Create dictionnary
    d["decoder"]        = decoderPtr;   // creat
    d["trans_model"]    = transModelPtr;// creat
    d["word_syms"]      = symTable;
    d["am_gmm"]         = amGmmPtr;
    
    
    return d;
}






bp::dict decode_faster_oneutt(
	bp::list loglikes_pylist,
	kaldi::FasterDecoder &decoder, 
	const kaldi::TransitionModel &trans_model,
	const kaldi::AmDiagGmm &am_gmm,
	fst::SymbolTable word_syms_table,
	float ac_scale = 0.1,
	bool b_allow_partial = true,
	bool b_use_gmm = false
				    ) {
  
  
  using namespace kaldi;
  typedef kaldi::int32 int32;
  using fst::SymbolTable;
  using fst::VectorFst;
  using fst::StdArc;

  // prepare result
  bp::dict ans;
  ans["failed"]  = false;
  ans["partial"] = false;
  
  
  BaseFloat acoustic_scale = ac_scale;
  bool allow_partial       = b_allow_partial; 
  
  fst::SymbolTable word_syms = word_syms_table;
  
  if (b_use_gmm && am_gmm.NumGauss() == 0){
    KALDI_ERR << "No GMM acoustic model";
  }
  
  // FIXME : should be an opt arg? (see decoder_faster_mapped)
/*
  fst::SymbolTable *word_syms = NULL;
  if (word_syms_filename != "") {
    word_syms = fst::SymbolTable::ReadText(word_syms_filename);
    if (!word_syms)
      KALDI_ERR << "Could not read symbol table from file "<<word_syms_filename;
  }
*/  
  

  // Convert python list to kaldi feature matrix
  const Matrix<BaseFloat> &loglikes (List2MatrixFloat(loglikes_pylist));

  
  if (loglikes.NumRows() == 0) {
    ans["failed"]  = true;
    ans["error"]   = "Zero-length utterance";
    return ans;
  }

  // Decode from likes
  if ( b_use_gmm ){
    
    DecodableAmDiagGmmScaled gmm_decodable(am_gmm, trans_model, loglikes,
                                             acoustic_scale);
    decoder.Decode(&gmm_decodable);
    
  } else {
    
    DecodableMatrixScaledMapped decodable(trans_model, loglikes, 
                                             acoustic_scale);
    decoder.Decode(&decodable);
    
  }

  
  VectorFst<LatticeArc> decoded;  // linear FST.


  if ( (allow_partial || decoder.ReachedFinal())
	&& decoder.GetBestPath(&decoded) ) {
    
    
   
    if (!decoder.ReachedFinal())
      ans["partial"]  = true;
      

    
    std::vector<int32> alignment;
    std::vector<int32> words;
    LatticeWeight weight;
    
    // retrieve symbol sequence
    GetLinearSymbolSequence(decoded, &alignment, &words, &weight);

    ans["words_ids"]  = words;
    ans["aligments"]  = alignment;
    ans["like"]       = -weight.Value1() -weight.Value2();
    
    std::vector<std::string> words_list;
    std::vector<int32> pdf_list;
    
    
//     if (word_syms != NULL) {
      for (size_t i = 0; i < words.size(); i++) {
	std::string s = word_syms.Find(words[i]);
	if (s == "")
	  s = "!!ERR!!";
	words_list.push_back(s);
      }
//     }
    
    for (size_t i = 0; i < alignment.size(); i++)
        pdf_list.push_back(trans_model.TransitionIdToPdf(alignment[i]));
    
    ans["words"]  = words_list;
    ans["pdfs"]   = pdf_list;
    
    
    return ans;
  } else {
    ans["failed"]  = true;
    ans["error"]   = "Could not decode";
    return ans;
  }
    
}


// For optional arguments

BOOST_PYTHON_FUNCTION_OVERLOADS(
  decoder_faster_overloards, 
  decoder_faster, 
  2, 7);

BOOST_PYTHON_FUNCTION_OVERLOADS(
  decode_faster_oneutt_overloads,
  decode_faster_oneutt,
  5, 8);
  




void PyKaldi_ExportDecoder() {
    using namespace boost::python;
    
    // Standard decoders
//     def("gmm_decode_faster", &gmm_decode_faster, gmm_decode_faster_overloads());
//     def("decode_faster_mapped", &decode_faster_mapped);
    
    
    // Expose objects with shared pointers
    class_<kaldi::FasterDecoder, faster_decoder_ptr, boost::noncopyable>(
      "FasterDecoder",
      boost::python::no_init
    );
    
    class_<kaldi::TransitionModel, trans_model_ptr, boost::noncopyable>(
      "TransitionModel",
      boost::python::no_init
    );
    
    
    class_<kaldi::AmDiagGmm, am_gmm_ptr, boost::noncopyable>(
      "AmDiagGmm",
      boost::python::no_init
    );
    
    class_<fst::SymbolTable, sym_table_ptr, boost::noncopyable>(
      "FstSymbolTable",
      boost::python::no_init
    );
    
    
    // More interesting stuff...
    
    to_python_converter<std::vector<int,class std::allocator<int> >, VecToList<int> >();
    to_python_converter<std::vector<std::string,class std::allocator<std::string> >, VecToList<std::string> >();
    
    def(
      "decoder_faster",
      &decoder_faster, 
      decoder_faster_overloards()
    );
    
    
    def(
      "decode_faster_oneutt",
      &decode_faster_oneutt,
      decode_faster_oneutt_overloads()
    );
}