#!/usr/bin/python
# 
# 
# Copyright 2013 T. Bluche
# 
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
# 
# 


import sys, os, logging, time

import pykaldi2 as pyk



class FasterDecoder():
  
  
  def __init__(self, model_file, fst_file, word_table_file):
    """
    :param string model_file: path to a Kaldi transition model
    :param string fst_file: path to an FST (e.g. HCLG)
    :param string word_table_file: path to a symbol table 
                                   (should match output symbols of fst)
    """
    
    self.model_file       = model_file
    self.fst_file         = fst_file
    self.word_table_file  = word_table_file
    self.use_gmm          = False
    self.decoder_dict     = {}
    
  def UseGmm(self):
    self.use_gmm          = True
  
  
  def Init(self,
           ac_scale      = 0.1,
           allow_partial = True,
           beam          = 16):
    """
    Initialize the decoder with a transition model and a transducer.
    
    :param float ac_scale: acoustic scale
    :param bool allow_partial: whether to allow partial outputs
    :param float beam: beam to use for beam search decoding

    """    
    
    # TODO check if files exist
    
    self.decoder_dict  = pyk.decoder_faster(self.model_file,
                                            self.fst_file,
                                            self.word_table_file,
                                            ac_scale,
                                            allow_partial,
                                            beam,
                                            self.use_gmm)
  
    
  def DecodeOne(self,
                features,
                decoder_dict,
                allow_partial  = True,
                use_gmm        = False,
                acoustic_scale = None):
    """
    
    :param list features: likelihood or feature matrix (list of lists of floats)
    :param dict decoder_dict: initialized decoder
    
    :rtype: dict
    :returns: a dictionary containing results
    
    """
    
    result_dict  = {}
    
    # TODO check if decoder was initialized
    
    if acoustic_scale is not None:
      ac_scale = acoustic_scale
    else:
      ac_scale = self.decoder_dict["acoustic_scale"]
    
    result_dict = pyk.decode_faster_oneutt(
                            features,
                            self.decoder_dict["decoder"],
                            self.decoder_dict["trans_model"],
                            self.decoder_dict["am_gmm"],
                            self.decoder_dict["word_syms"],
                            ac_scale,
                            allow_partial, use_gmm)
    return result_dict

    