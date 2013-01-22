pykaldi
=======

Wrapping KALDI <http://sourceforge.net/projects/kaldi/> into python functions.

WHY
---

* Most of the code I have is written in Python, and I have many scripts which
  alternate between executing Kaldi commands and parsing the generated files.

* Many toolkits are written in Python, and I would like to make then interact
  with Kaldi directly. 

* I also wanted to learn C++ and Boost Python, and since I use Kaldi and I know
  Python, it may be an useful thing to do... (so do not expect high quality C++
  code for now...)



WARNING
-------

* To many aspects, it does not follow the philosophy of the Kaldi project.

* This code merely attempts to use some of the Kaldi functionalities, so
  you loose many cool aspects of Kaldi executable files, such as the possibility
  to pipe Kaldi command, amongst other things...

* This is not an attempt to export the whole Kaldi in Python, but only (some)
  of the executables : most of the code is a copy-paste of the code in ``*bin/``
  folders of Kaldi sources, with a few adaptations.

* The proposed solutions are probably a lot slower, memory-consuming, etc. than
  Kaldi is.

* So far, I did not perform any test beyond very basic ones...

* I will develop features as I need them, and since I do not work on speech 
  recognition, I will probably skip a lot of things

* So far, I only developed a Python version of the ``decode-faster-mapped``
  program from Kaldi


TODO
----

* Proper makefile
* Tests
* Clean code - and add licence in files
* Add functionalities
* Add python code examples
