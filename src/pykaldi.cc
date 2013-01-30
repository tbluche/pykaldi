//    Copyright 2013 T. Bluche
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <boost/python.hpp>

void PyKaldi_ExportUtils();
void PyKaldi_ExportFeatures();
void PyKaldi_ExportGmm();
void PyKaldi_ExportDecoder();


BOOST_PYTHON_MODULE(pykaldi2)
{
    PyKaldi_ExportUtils();
    PyKaldi_ExportFeatures();
    PyKaldi_ExportGmm();
    PyKaldi_ExportDecoder();
}