#! /usr/bin/bash

KALDI_PATH=/home/bluche/Work/KALDI/kaldi-trunk
BOOST_PATH=/home/bluche/boost
PYTHON_VER=2.6

KALDI_PATH=/home/theodore/src/kaldi
BOOST_PATH=/home/theodore
PYTHON_VER=2.7


KALDI_SRC=${KALDI_PATH}/src

make_libkaldi=0
for param in `seq 1`
do
  if [[ $1 == "-K" ]]; then shift; make_libkaldi=1; shift; fi
done

if [[ $make_libkaldi -gt 0 ]]
then

(


echo "Entering ${KALDI_PATH}/src ..."
cd ${KALDI_PATH}/src


# klibs="lm/kaldi-lm.a decoder/kaldi-decoder.a lat/kaldi-lat.a \
#     hmm/kaldi-hmm.a transform/kaldi-transform.a \
#     gmm/kaldi-gmm.a tree/kaldi-tree.a matrix/kaldi-matrix.a util/kaldi-util.a \
#     base/kaldi-base.a "

klibs="base/kaldi-base.a matrix/kaldi-matrix.a util/kaldi-util.a  feat/kaldi-feature.a \
  tree/kaldi-tree.a gmm/kaldi-gmm.a hmm/kaldi-hmm.a lat/kaldi-lat.a \
  transform/kaldi-transform.a thread/kaldi-thread.a"

cmd="g++ -shared -o libkaldi.so -Wl,--whole-archive \
  ../tools/openfst/lib/libfst.a ../tools/openfst/lib/libfstscript.a \
  $klibs \
  -Wl,--no-whole-archive"

echo $cmd
$cmd

echo "Leaving ${KALDI_PATH}/src ..."

)


cp ${KALDI_PATH}/src/libkaldi.so .

fi


klibs="${KALDI_SRC}/lm/kaldi-lm.a ${KALDI_SRC}/decoder/kaldi-decoder.a ${KALDI_SRC}/lat/kaldi-lat.a \
    ${KALDI_SRC}/hmm/kaldi-hmm.a ${KALDI_SRC}/transform/kaldi-transform.a \
    ${KALDI_SRC}/gmm/kaldi-gmm.a ${KALDI_SRC}/tree/kaldi-tree.a ${KALDI_SRC}/matrix/kaldi-matrix.a \
    ${KALDI_SRC}/util/kaldi-util.a \
    ${KALDI_SRC}/base/kaldi-base.a ${KALDI_SRC}/feat/kaldi-feature.a"


ccfiles=" src/pykaldi_utils.cc  src/pykaldi_features.cc src/pykaldi_gmm.cc \
    src/pykaldi_decoder.cc src/pykaldi.cc"

cmd="g++ $ccfiles $klibs -I${BOOST_PATH}/include -I/usr/include/python${PYTHON_VER} \
 -I${KALDI_PATH}/src -DHAVE_ATLAS -I${KALDI_PATH}/tools/ATLAS/include \
 -I${KALDI_PATH}/tools/openfst/include \
 -Wl,-no-undefined,-rpath,${BOOST_PATH}/lib  \
  -L/usr/lib/python${PYTHON_VER}/config -L${BOOST_PATH}/lib -lpython${PYTHON_VER} \
  -lboost_python \
  -L${KALDI_PATH}/src -lkaldi \
  -L/usr/lib -llapack -lcblas -latlas -lf77blas \
  -DKALDI_DOUBLEPRECISION=0 -msse -msse2 \
  -DHAVE_POSIX_MEMALIGN -Wno-sign-compare -Winit-self \
  -DHAVE_EXECINFO_H=1 -rdynamic -DHAVE_CXXABI_H \
  -g -O0 -DKALDI_PARANOID   \
  -rdynamic \
  -lm -lpthread \
 -o pykaldi.so -shared "

echo $cmd
$cmd



echo "Trying to import pykaldi"
python -c "import pykaldi" 





# ========================================================================================



# ====== pykaldi.so inspiration ==========================================================

#g++ -I/usr/local/include -I/usr/include/boost/ -I/usr/include/python2.7 \
#   -I/home/theodore/src/kaldi/src -DHAVE_ATLAS \
#   -I/home/theodore/src/kaldi/tools/ATLAS/include \
#   -I/home/theodore/src/kaldi/tools/openfst/include \
#   -DKALDI_DOUBLEPRECISION=0 -DHAVE_POSIX_MEMALIGN -Wno-sign-compare  \
#   -fPIC -c pykaldi.cc -o pykaldi.o 

#g++ -shared -Wl,-soname,libpykaldi.so \
#   -o libpykaldi.so pykaldi.o -lboost_python -lpython2.7 


# g++ -shared -o hello.so hello.o -lpython2.7 -lboost_python -lboost_system



# g++ pykaldi.cc -I/home/theodore/include -I/usr/include/python2.7 \
#  -I/home/theodore/src/kaldi/src -DHAVE_ATLAS -I/home/theodore/src/kaldi/tools/ATLAS/include \
#  -I/home/theodore/src/kaldi/tools/openfst/include \
#  -Wl,-no-undefined,-rpath,/home/theodore/lib  \
#   -L/usr/lib/python2.7/config -L/home/theodore/lib -lpython2.7 -lboost_python \
#   -L/home/theodore/src/kaldi/src/ -lkaldi -L/usr/lib -llapack -lcblas -latlas -lf77blas \
#  -o pykaldi.so -shared 




# ====== example kaldi exe ===============================================================

# g++ -msse -msse2 -Wall -I/home/bluche/Work/KALDI/kaldi-dev/src \
#   -DKALDI_DOUBLEPRECISION=0 \
#   -DHAVE_POSIX_MEMALIGN -Wno-sign-compare -Winit-self \
#   -DHAVE_EXECINFO_H=1 -rdynamic -DHAVE_CXXABI_H \
#   -g -O0 -DKALDI_PARANOID   \
#   -rdynamic \
#   -lm -lpthread \
# -DHAVE_ATLAS -I/home/bluche/Work/KALDI/kaldi-dev/tools/ATLAS/include \
# -I/home/bluche/Work/KALDI/kaldi-dev/tools/openfst/include \
#   extract-ml2-dataset.cc /home/bluche/Work/KALDI/kaldi-dev/src/lat/kaldi-lat.a \
# /home/bluche/Work/KALDI/kaldi-dev/src/decoder/kaldi-decoder.a /home/bluche/Work/KALDI/kaldi-dev/src/feat/kaldi-feature.a \
# /home/bluche/Work/KALDI/kaldi-dev/src/transform/kaldi-transform.a /home/bluche/Work/KALDI/kaldi-dev/src/gmm/kaldi-gmm.a \
# /home/bluche/Work/KALDI/kaldi-dev/src/hmm/kaldi-hmm.a /home/bluche/Work/KALDI/kaldi-dev/src/tree/kaldi-tree.a \
# /home/bluche/Work/KALDI/kaldi-dev/src/matrix/kaldi-matrix.a /home/bluche/Work/KALDI/kaldi-dev/src/util/kaldi-util.a \
# /home/bluche/Work/KALDI/kaldi-dev/src/base/kaldi-base.a  /home/bluche/Work/KALDI/kaldi-dev/tools/openfst/lib/libfst.a \
# -ldl /home/bluche/src/ATLAS/build/lib//liblapack.a \
# /home/bluche/src/ATLAS/build/lib//libcblas.a \
# /home/bluche/src/ATLAS/build/lib//libatlas.a \
# /home/bluche/src/ATLAS/build/lib//libf77blas.a \
#  -o extract-ml2-dataset



# ====== libkaldi.so =====================================================================

# gcc -shared -o libkaldi.so -Wl,--whole-archive \
#   ../tools/openfst/lib/libfst.a ../tools/openfst/lib/libfstscript.a \
#   base/kaldi-base.a matrix/kaldi-matrix.a util/kaldi-util.a feat/kaldi-feature.a \
#   tree/kaldi-tree.a gmm/kaldi-gmm.a tied/kaldi-tied-gmm.a transform/kaldi-transform.a \
#   sgmm/kaldi-sgmm.a hmm/kaldi-hmm.a  lm/kaldi-lm.a \
#   decoder/kaldi-decoder.a lat/kaldi-lat.a nnet/kaldi-nnet.a \
#   -Wl,--no-whole-archive


# export LD_PRELOAD=/usr/lib/libstdc++.so.6.0.13



# ====== dependencies ====================================================================

# base:
# matrix : base
# util: base matrix
# thread: util
# feat: base matrix util gmm transform
# tree: base util matrix
# optimization: base matrix
# gmm: base util matrix tree
# tied: base util matrix gmm tree transform
# transform: base util matrix gmm tree
# sgmm: base util matrix gmm tree transform thread
# sgmm2: base util matrix gmm tree transform thread
# fstext: base util matrix tree
# hmm: base tree matrix 
# lm: base util
# decoder: base util matrix gmm sgmm hmm tree transform
# lat: base util
# cudamatrix: base util matrix    
# nnet: base util matrix cudamatrix
# nnet-cpu: base util matrix threa




# ====== may be interesting to have a lokk at ... ========================================

# --- Boost.NumPy ---
# https://github.com/ndarray/Boost.NumPy


