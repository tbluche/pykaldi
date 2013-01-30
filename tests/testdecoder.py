#!/usr/bin/python
#                                                                               
#  Copyright 2013 T. Bluche                                                     
#                                                                               
#  Licensed under the Apache License, Version 2.0 (the "License");              
#  you may not use this file except in compliance with the License.             
#  You may obtain a copy of the License at                                      
#                                                                               
#  http://www.apache.org/licenses/LICENSE-2.0                                   
#                                                                               
#  Unless required by applicable law or agreed to in writing, software          
#  distributed under the License is distributed on an "AS IS" BASIS,            
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.     
#  See the License for the specific language governing permissions and          
#  limitations under the License.                                               
#                                                                               


import sys, os, logging, time

import pykaldi2 as pyk2
import pykaldi  as pyk

from pykaldi.decoder.fasterdecoder import FasterDecoder



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
    
    fts=pyk2.feature_provider(rxfilename)
    
    return fts;
    
    
  except IOError as e:
    logging.error(" The file %s does not seem to exist"%(fin))
    exit(1)
  

  
  
  
  

def test_getfeatures():
  get_features('data/testlks.scp')

  
  
def test_initdecoder():
  d = FasterDecoder("data/test.mdl","data/test.fst","data/words.txt")
  d.Init()

  
  
def test_decode():
  print "DECODE FASTER MAPPED"
  
  fts        = get_features('data/testlks.scp')
  decoder    = FasterDecoder("data/test.mdl","data/test.fst","data/words.txt")
  decoder.UseGmm()
  decoder.Init()
  
  start  = time.time()
  for k in fts:
    print "Decoding %s..."%k
    result  = decoder.DecodeOne(fts[k],decoder)
    print result
  print "Elapsed ", (time.time() - start)
  
  
  print "DECODE FASTER GMM"
  fts        = get_features('data/test.scp')
  start  = time.time()
  for k in fts:
    print "Decoding %s..."%k
    result  = decoder.DecodeOne(fts[k],decoder,use_gmm=True)
    print result
  print "Elapsed ", (time.time() - start)


if __name__ == '__main__':
  test_decode()