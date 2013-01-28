#!/usr/bin/python


import sys, os, logging, time

import pykaldi as pyk


def get_features(fin):
  """
  Get a dictionary of feature matrices indexed by the file indices.
  
  :param string fin: scp feature filename
  
  :returns: a dictionary containing feature matrices
  
  """
  
  try:
    
    f = open(fin)
    
    fname, fext  = os.path.splitext(fin)
    
    rxfilename  = fin
    if fext == ".scp":
      rxfilename = 'scp:'+fin
    
    fts=pyk.feature_provider(rxfilename)
    
    return fts;
    
    
  except IOError as e:
    logging.error(" The file %s does not seem to exist"%(fin))
    exit(1)
  
    

def init_decoder(
      model,
      fst,
      words         = "",
      ac_scale      = 0.1,
      allow_partial = True,
      beam          = 16
   ):
    """
    Initialize the decoder with a transition model and a transducer.

    :param string model: path to a Kaldi transition model
    :param string fst: path to an FST (e.g. HCLG)
    :param string words: path to a symbol table (should match output symbols of fst)
    :param float ac_scale: acoustic scale
    :param bool allow_partial: whether to allow partial outputs
    :param float beam: beam to use for beam search decoding

    :rtype: dict
    :returns: a dictionary with necessary elements for decoding to take place

    """
    decoder_dict  = pyk.decoder_faster_mapped(
			      model,
			      fst,words,
			      ac_scale,
			      allow_partial,
			      beam)

    return decoder_dict
     



def decode_with_decoder(features,decoder_dict):
  """
  Use a decoder, initialized for example with `init_decoder` to decode 
  likelihoods.
  
  :param list features: likelihhod matrix (list of lists of floats)
  :param dict decoder_dict: initialized decoder
  
  :rtype: dict
  :returns: a dictionary containing results
  
  """
  result_dict = pyk.decode_faster_mapped_oneutt(
                          features,
                          decoder_dict["decoder"],
                          decoder_dict["trans_model"],
                          decoder_dict["word_syms"],
                          decoder_dict["acoustic_scale"])
  return result_dict
    


def test_getfeatures():
  get_features('data/testlks.scp')

def test_initdecoder():
  init_decoder("data/test.mdl","data/test.fst","data/words.txt")

def test_decode():
  fts        = get_features('data/testlks.scp')
  decoder    = init_decoder("data/test.mdl","data/test.fst","data/words.txt")
  
  start  = time.time()
  tmconv = 0
  for k in fts:
    print "Decoding %s..."%k
    #sstart = time.time()
    #pyk.tlm(fts[k])
    #tmconv += time.time() - sstart
    result  = decode_with_decoder(fts[k],decoder)
    print result
    print
    print
  print "Elapsed ", (time.time() - start)
  #print "Time conv ", tmconv
  #print "Est. time", time.time() - start - 2*tmconv



if __name__ == '__main__':
  test_decode()