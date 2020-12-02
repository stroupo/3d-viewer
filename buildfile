cxx.std = latest
using cxx

hxx{*}: extension = hpp
cxx{*}: extension = cpp

import libs = glbinding%lib{glbinding}
import libs += glm%lib{glm}
import libs += glfw3%lib{glfw3}

exe{viewer}: {hxx cxx}{**} $libs

cxx.poptions =+ "-I$src_base"
cxx.coptions += -O3 -march=native
cxx.loptions =+ "-L/usr/local/lib"