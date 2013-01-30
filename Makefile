
KALDI_PATH=/home/theodore/src/kaldi
BOOST_PATH=/home/theodore
PYTHON_VER=2.7


KALDI_SRC=$(KALDI_PATH)/src
FSTROOT = $(KALDI_PATH)/tools/openfst
ATLASINC = $(KALDI_PATH)/tools/ATLAS/include


ATLASLIBS = -L/usr/lib -llapack -lcblas -latlas -lf77blas


KALDI_CXXFLAGS = -msse -msse2 -I$(KALDI_SRC) \
		-DKALDI_DOUBLEPRECISION=0 -DHAVE_POSIX_MEMALIGN \
		-Wno-sign-compare -Winit-self \
		-DHAVE_EXECINFO_H=1 -rdynamic -DHAVE_CXXABI_H \
		-DHAVE_ATLAS -I$(ATLASINC) \
		-I$(FSTROOT)/include \
		$(EXTRA_CXXFLAGS) -fPIC \
		-g -O0 -DKALDI_PARANOID 

EXTRA_LDLIBS =  -L/usr/lib/python$(PYTHON_VER)/config  -lpython$(PYTHON_VER) \
                -L$(BOOST_PATH)/lib -lboost_python \
                -L$(KALDI_SRC) -lkaldi

CXXFLAGS = -I$(BOOST_PATH)/include -I/usr/include/python$(PYTHON_VER) $(KALDI_CXXFLAGS)
LDFLAGS = -rdynamic -Wl,-no-undefined,-rpath,$(BOOST_PATH)/lib
LDLIBS = $(EXTRA_LDLIBS) $(FSTROOT)/lib/libfst.a -ldl $(ATLASLIBS) -lm -lpthread
CC = g++
CXX = g++
AR = ar
AS = as


KLIBS = $(KALDI_SRC)/lm/kaldi-lm.a $(KALDI_SRC)/decoder/kaldi-decoder.a \
	$(KALDI_SRC)/lat/kaldi-lat.a \
	$(KALDI_SRC)/hmm/kaldi-hmm.a $(KALDI_SRC)/transform/kaldi-transform.a \
	$(KALDI_SRC)/gmm/kaldi-gmm.a $(KALDI_SRC)/tree/kaldi-tree.a \
	$(KALDI_SRC)/matrix/kaldi-matrix.a \
	$(KALDI_SRC)/util/kaldi-util.a \
	$(KALDI_SRC)/base/kaldi-base.a $(KALDI_SRC)/feat/kaldi-feature.a

KLIBS2 = $(KALDI_SRC)/base/kaldi-base.a $(KALDI_SRC)/matrix/kaldi-matrix.a \
	 $(KALDI_SRC)/util/kaldi-util.a  $(KALDI_SRC)/feat/kaldi-feature.a \
	 $(KALDI_SRC)/tree/kaldi-tree.a $(KALDI_SRC)/gmm/kaldi-gmm.a \
	 $(KALDI_SRC)/hmm/kaldi-hmm.a $(KALDI_SRC)/lat/kaldi-lat.a \
	 $(KALDI_SRC)/transform/kaldi-transform.a $(KALDI_SRC)/thread/kaldi-thread.a\
	 $(FSTROOT)/lib/libfst.a $(FSTROOT)/lib/libfstscript.a


CCFILES = src/pykaldi_utils.cc  src/pykaldi_features.cc src/pykaldi_gmm.cc \
          src/pykaldi_decoder.cc src/pykaldi.cc


all: pykaldi

kaldi:
	-$(CXX) -shared -o libkaldi.so -Wl,--whole-archive $(KLIBS2) -Wl,--no-whole-archive

pykaldi: kaldi
	-$(CXX) $(CCFILES) $(KLIBS) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -o pykaldi2.so -shared

clean:
	-rm pykaldi2.so

test:
	-python -c "import pykaldi2"