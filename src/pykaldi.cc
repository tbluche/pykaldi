#include <boost/python.hpp>

void PyKaldi_ExportUtils();
void PyKaldi_ExportFeatures();
void PyKaldi_ExportGmm();
void PyKaldi_ExportDecoder();


BOOST_PYTHON_MODULE(pykaldi)
{
    PyKaldi_ExportUtils();
    PyKaldi_ExportFeatures();
    PyKaldi_ExportGmm();
    PyKaldi_ExportDecoder();
}