import pykaldi as pk

print
print "feat-to-len"
print pk.feat_to_len("scp:data/test.scp")

print
print "gmm-decode-faster (1)"
pk.gmm_decode_faster("data/test.mdl","data/test.fst","scp:data/test.scp")

print
print "gmm-decode-faster (2)"
pk.gmm_decode_faster("data/test.mdl","data/test.fst","scp:data/test.scp","data/words.txt",0.0833)

#pk.decode_faster_mapped("data/test.mdl","data/test.fst","scp:data/testlks.scp")

#import pykaldi as pk

fts=pk.feature_provider('scp:data/testlks.scp')
k=fts.keys()[0]
f=fts[k]
dec=pk.decoder_faster("data/test.mdl","data/test.fst")
pk.decode_one(f,dec['decoder'],dec['trans_model'])

#dec=pk.decoder_faster("test.mdl","test.fst","scp:testlks.scp")
#pk.decode_one("scp:testlks.scp",dec['decoder'],dec['trans_model'])