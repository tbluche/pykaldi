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











int gmm_decode_faster(
  std::string model_rxfilename,
  std::string fst_rxfilename, 
  std::string feature_rspecifier,
  std::string word_syms_filename = "",
  float ac_scale = 0.1,
  bool b_allow_partial = true,
  float beam = 16.0
		) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    
    bool allow_partial = b_allow_partial;
    BaseFloat acoustic_scale = ac_scale;
    
    FasterDecoderOptions decoder_opts;
    decoder_opts.beam = beam;

    
    TransitionModel trans_model;
    AmDiagGmm am_gmm;
    {
      bool binary;
      Input ki(model_rxfilename, &binary);
      trans_model.Read(ki.Stream(), binary);
      am_gmm.Read(ki.Stream(), binary);
    }

    fst::SymbolTable *word_syms = NULL;
    if (word_syms_filename != "") 
      if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
        KALDI_ERR << "Could not read symbol table from file "
                   << word_syms_filename;

    SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);

    fst::Fst<fst::StdArc> *decode_fst = ReadNetwork(fst_rxfilename);
    
    BaseFloat tot_like = 0.0;
    kaldi::int64 frame_count = 0;
    int num_success = 0, num_fail = 0;
    FasterDecoder decoder(*decode_fst, decoder_opts);

    Timer timer;

    for (; !feature_reader.Done(); feature_reader.Next()) {
      std::string key = feature_reader.Key();
      Matrix<BaseFloat> features (feature_reader.Value());
      feature_reader.FreeCurrent();
      if (features.NumRows() == 0) {
        KALDI_WARN << "Zero-length utterance: " << key;
        num_fail++;
        continue;
      }

      DecodableAmDiagGmmScaled gmm_decodable(am_gmm, trans_model, features,
                                             acoustic_scale);
      decoder.Decode(&gmm_decodable);

      fst::VectorFst<LatticeArc> decoded;  // linear FST.

      if ( (allow_partial || decoder.ReachedFinal())
           && decoder.GetBestPath(&decoded) ) {
        if (!decoder.ReachedFinal())
          KALDI_WARN << "Decoder did not reach end-state, "
                    << "outputting partial traceback since --allow-partial=true";
        num_success++;
        if (!decoder.ReachedFinal())
          KALDI_WARN << "Decoder did not reach end-state, outputting partial traceback.";
        std::vector<int32> alignment;
        std::vector<int32> words;
        LatticeWeight weight;
        frame_count += features.NumRows();

        GetLinearSymbolSequence(decoded, &alignment, &words, &weight);
          
        if (word_syms != NULL) {
          std::cerr << key << ' ';
          for (size_t i = 0; i < words.size(); i++) {
            std::string s = word_syms->Find(words[i]);
            if (s == "")
              KALDI_ERR << "Word-id " << words[i] <<" not in symbol table.";
            std::cerr << s << ' ';
          }
          std::cerr << '\n';
        }
        BaseFloat like = -weight.Value1() -weight.Value2();
        tot_like += like;
        KALDI_LOG << "Log-like per frame for utterance " << key << " is "
                  << (like / features.NumRows()) << " over "
                  << features.NumRows() << " frames.";
        KALDI_VLOG(2) << "Cost for utterance " << key << " is "
                      << weight.Value1() << " + " << weight.Value2();
      } else {
        num_fail++;
        KALDI_WARN << "Did not successfully decode utterance " << key
                   << ", len = " << features.NumRows();
      }
    }

    double elapsed = timer.Elapsed();
    KALDI_LOG << "Time taken [excluding initialization] "<< elapsed
              << "s: real-time factor assuming 100 frames/sec is "
              << (elapsed*100.0/frame_count);
    KALDI_LOG << "Done " << num_success << " utterances, failed for "
              << num_fail;
    KALDI_LOG << "Overall log-likelihood per frame is " << (tot_like/frame_count) << " over "
              << frame_count<<" frames.";

    if (word_syms) delete word_syms;    
    delete decode_fst;
    return (num_success != 0 ? 0 : 1);
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}


#include "decoder/decodable-matrix.h"

int decode_faster_mapped(std::string model_in_filename,std::string fst_in_filename, std::string loglikes_rspecifier) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    using fst::SymbolTable;
    using fst::VectorFst;
    using fst::StdArc;

    bool binary = true;
    BaseFloat acoustic_scale = 0.1;
    bool allow_partial = true;
    std::string word_syms_filename = "words.txt";
    FasterDecoderOptions decoder_opts;
    /*
    decoder_opts.Register(&po, true);  // true == include obscure settings.
    po.Register("binary", &binary, "Write output in binary mode");
    po.Register("acoustic-scale", &acoustic_scale, "Scaling factor for acoustic likelihoods");
    po.Register("allow-partial", &allow_partial, "Produce output even when final state was not reached");
    po.Register("word-symbol-table", &word_syms_filename, "Symbol table for words [for debug output]");
   
    po.Read(argc, argv);

    std::string model_in_filename = po.GetArg(1),
        fst_in_filename = po.GetArg(2),
        loglikes_rspecifier = po.GetArg(3),
        words_wspecifier = po.GetArg(4),
        alignment_wspecifier = po.GetOptArg(5);
 */
    
    TransitionModel trans_model;
    ReadKaldiObject(model_in_filename, &trans_model);

//     Int32VectorWriter words_writer(words_wspecifier);

//     Int32VectorWriter alignment_writer(alignment_wspecifier);

    fst::SymbolTable *word_syms = NULL;
    if (word_syms_filename != "") {
      word_syms = fst::SymbolTable::ReadText(word_syms_filename);
      if (!word_syms)
        KALDI_ERR << "Could not read symbol table from file "<<word_syms_filename;
    }

    SequentialBaseFloatMatrixReader loglikes_reader(loglikes_rspecifier);

    // It's important that we initialize decode_fst after loglikes_reader, as it
    // can prevent crashes on systems installed without enough virtual memory.
    // It has to do with what happens on UNIX systems if you call fork() on a
    // large process: the page-table entries are duplicated, which requires a
    // lot of virtual memory.
    VectorFst<StdArc> *decode_fst = NULL;
    {
      Input ki(fst_in_filename.c_str());
      decode_fst =
          VectorFst<StdArc>::Read(ki.Stream(), fst::FstReadOptions(fst_in_filename));
      if (decode_fst == NULL) // fst code will warn.
        exit(1);
    }

    BaseFloat tot_like = 0.0;
    kaldi::int64 frame_count = 0;
    int num_success = 0, num_fail = 0;
    FasterDecoder decoder(*decode_fst, decoder_opts);

//     Timer timer;

    for (; !loglikes_reader.Done(); loglikes_reader.Next()) {
      std::string key = loglikes_reader.Key();
      const Matrix<BaseFloat> &loglikes (loglikes_reader.Value());

      if (loglikes.NumRows() == 0) {
        KALDI_WARN << "Zero-length utterance: " << key;
        num_fail++;
        continue;
      }

      DecodableMatrixScaledMapped decodable(trans_model, loglikes, acoustic_scale);
      decoder.Decode(&decodable);

      VectorFst<LatticeArc> decoded;  // linear FST.

      if ( (allow_partial || decoder.ReachedFinal())
           && decoder.GetBestPath(&decoded) ) {
        num_success++;
        if (!decoder.ReachedFinal())
          KALDI_WARN << "Decoder did not reach end-state, outputting partial traceback.";

        std::vector<int32> alignment;
        std::vector<int32> words;
        LatticeWeight weight;
        frame_count += loglikes.NumRows();

        GetLinearSymbolSequence(decoded, &alignment, &words, &weight);

//         words_writer.Write(key, words);
//         if (alignment_writer.IsOpen())
//           alignment_writer.Write(key, alignment);
        if (word_syms != NULL) {
          std::cerr << key << ' ';
          for (size_t i = 0; i < words.size(); i++) {
            std::string s = word_syms->Find(words[i]);
            if (s == "")
              KALDI_ERR << "Word-id " << words[i] <<" not in symbol table.";
            std::cerr << s << ' ';
          }
          std::cerr << '\n';
        }
        BaseFloat like = -weight.Value1() -weight.Value2();
        tot_like += like;
        KALDI_LOG << "Log-like per frame for utterance " << key << " is "
                  << (like / loglikes.NumRows()) << " over "
                  << loglikes.NumRows() << " frames.";

      } else {
        num_fail++;
        KALDI_WARN << "Did not successfully decode utterance " << key
                   << ", len = " << loglikes.NumRows();
      }
    }

    /*
    double elapsed = timer.Elapsed();
    KALDI_LOG << "Time taken [excluding initialization] "<< elapsed
              << "s: real-time factor assuming 100 frames/sec is "
              << (elapsed*100.0/frame_count);
    KALDI_LOG << "Done " << num_success << " utterances, failed for "
              << num_fail;
    KALDI_LOG << "Overall log-likelihood per frame is " << (tot_like/frame_count)
              << " over " << frame_count << " frames.";
    */
	      
    if (word_syms) delete word_syms;
    delete decode_fst;
    if (num_success != 0) return 0;
    else return 1;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}




typedef boost::shared_ptr<kaldi::FasterDecoder> faster_decoder_ptr;
typedef boost::shared_ptr<kaldi::TransitionModel> trans_model_ptr;
typedef boost::shared_ptr<fst::SymbolTable> sym_table_ptr;



bp::dict decoder_faster_mapped(
	    std::string model_in_filename,
	    std::string fst_in_filename,
	    std::string word_syms_filename = "",
	    float ac_scale = 0.1,
	    bool b_allow_partial = true,
	    float beam = 16.0
			      ) {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    using fst::SymbolTable;
    using fst::VectorFst;
    using fst::StdArc;

    bool binary = true;
    
    bool allow_partial = b_allow_partial;
    BaseFloat acoustic_scale = ac_scale;
    
    FasterDecoderOptions decoder_opts;
    decoder_opts.beam = beam;
    
    
//     std::string word_syms_filename = "data/words.txt";
    

    // Read word symbol table
    // FIXME : shared ptr and return it as well
    fst::SymbolTable *word_syms = NULL;
    if (word_syms_filename != "") {
      word_syms = fst::SymbolTable::ReadText(word_syms_filename);
      if (!word_syms)
        KALDI_ERR << "Could not read symbol table from file "<<word_syms_filename;
    }
    
    
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
    // Read transition model
    Input ki(model_in_filename, &binary);
    (*transModelPtr).Read(ki.Stream(), binary);
    ki.Close();
    
    
    // Create dictionnary
    bp::dict d;
    d["decoder"]        = decoderPtr;   // creat
    d["trans_model"]    = transModelPtr;// creat
    d["acoustic_scale"] = ac_scale;     // param
    d["beam"]           = beam;         // param
    d["word_syms"]      = word_syms_filename;
    
    
    return d;
}



// FIXME move to utils
template<class T>
bp::list std_vector_to_py_list(const std::vector<T>& v)
{
    bp::object get_iter = bp::iterator<std::vector<T> >();
    bp::object iter = get_iter(v);
    bp::list l(iter);
    return l;
}


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


bp::dict decode_faster_mapped_oneutt(
	bp::list loglikes_pylist,
	kaldi::FasterDecoder &decoder, 
	const kaldi::TransitionModel &trans_model,
	std::string word_syms_filename = "",
	float ac_scale = 0.1,
	bool b_allow_partial = true
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
  
  // FIXME : should be an opt arg? (see decoder_faster_mapped)
  fst::SymbolTable *word_syms = NULL;
  if (word_syms_filename != "") {
    word_syms = fst::SymbolTable::ReadText(word_syms_filename);
    if (!word_syms)
      KALDI_ERR << "Could not read symbol table from file "<<word_syms_filename;
  }
  
  
//   std::string key = "--";  // FIXME (should be a (optional) arg?)


  // Convert python list to kaldi feature matrix
  const Matrix<BaseFloat> &loglikes (List2MatrixFloat(loglikes_pylist));

  
  if (loglikes.NumRows() == 0) {
    ans["failed"]  = true;
    ans["error"]   = "Zero-length utterance";
    return ans;
  }

  // Decode from likes
  DecodableMatrixScaledMapped decodable(trans_model, loglikes, acoustic_scale);
  decoder.Decode(&decodable);

  
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
    
    
    if (word_syms != NULL) {
      for (size_t i = 0; i < words.size(); i++) {
	std::string s = word_syms->Find(words[i]);
	if (s == "")
	  s = "!!ERR!!";
	words_list.push_back(s);
      }
    }
    
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
  gmm_decode_faster_overloads, 
  gmm_decode_faster, 
  3, 7);

BOOST_PYTHON_FUNCTION_OVERLOADS(
  decoder_faster_mapped_overloards, 
  decoder_faster_mapped, 
  2, 6);

BOOST_PYTHON_FUNCTION_OVERLOADS(
  decode_faster_mapped_oneutt_overloads,
  decode_faster_mapped_oneutt,
  3, 6);
  




void PyKaldi_ExportDecoder() {
    using namespace boost::python;
    
    // Standard decoders
    def("gmm_decode_faster", &gmm_decode_faster, gmm_decode_faster_overloads());
    def("decode_faster_mapped", &decode_faster_mapped);
    
    
    // Expose objects with shared pointers
    class_<kaldi::FasterDecoder, faster_decoder_ptr, boost::noncopyable>(
      "FasterDecoder",
      boost::python::no_init
    );
    
    class_<kaldi::TransitionModel, trans_model_ptr, boost::noncopyable>(
      "TransitionModel",
      boost::python::no_init
    );
    
    
    // More interesting stuff...
    
    to_python_converter<std::vector<int,class std::allocator<int> >, VecToList<int> >();
    to_python_converter<std::vector<std::string,class std::allocator<std::string> >, VecToList<std::string> >();
    
    def(
      "decoder_faster_mapped",
      &decoder_faster_mapped, 
      decoder_faster_mapped_overloards()
    );
    
    
    def(
      "decode_faster_mapped_oneutt",
      &decode_faster_mapped_oneutt,
      decode_faster_mapped_oneutt_overloads()
    );
}