#!/bin/bash
if [ "$PREFIX" == "" ]; then
    echo 'Give me PREFIX variable (usually /usr)'
    exit 1
fi

Entries=$(perl -M5.010 -e 'opendir(my $dh,$ENV{"PREFIX"}."/include");for $name(grep {/^openrave/} readdir($dh)){say $name};')
if [ "$Entries" == "" -o $(<<<$Entries wc -l) -ge 2 ]; then
    echo 'Multiple (or No) openrave installations found'
    cat <<<$Entries
    exit 1
fi

OPENRAVE_DIR=$(<<<${Entries} tr -d -- "\n") # openrave-0.11
#OPENRAVE_PYDIR="_"$(<<<${OPENRAVE_DIR} sed -e s/openrave/openravepy/ | tr -- "-." "__") # _openravepy_0_11
#OPENRAVEPY_INT=${PREFIX}/lib/python2.7/site-packages/openravepy/${OPENRAVE_PYDIR}/openravepy_int.so

PYTHON=${PYTHON:-python}
OPENRAVEPY_INT="$(${PYTHON} -c 'import openravepy;print(openravepy.openravepy_int.__file__)')"
SOURCE_DIR=$(dirname "$0")

PYTHON_LIBS=$($PYTHON-config --libs --embed 2>/dev/null)
if [ $? -ne 0 ]; then
    PYTHON_LIBS=$($PYTHON-config --libs)
fi

if ! grep -F '#define USE_PYBIND11_PYTHON_BINDINGS' ${PREFIX}/include/${OPENRAVE_DIR}/openravepy/openravepy_config.h >/dev/null; then
   PYTHON_BINDING_LIBS="-lboost_python"
fi

g++ -std=gnu++11 -O2 -fPIC -shared -o openrave_rawxml.so \
-I ${PREFIX}/include/${OPENRAVE_DIR} -I ${PREFIX}/include $(${PYTHON}-config --includes) -I "$(${PYTHON} -c 'import numpy;print(numpy.get_include())')" -I /usr/include/libxml2 \
"${SOURCE_DIR}/openrave_rawxml.cpp" "${SOURCE_DIR}/parsexml.cpp" \
"${OPENRAVEPY_INT}" \
-Wl,--as-needed \
-L ${PREFIX}/lib ${PYTHON_LIBS} -l$(<<<${OPENRAVE_DIR} tr -d -- -) ${PYTHON_BINDING_LIBS} -llog4cxx -lxml2 \
-Wl,--no-undefined "-Wl,-rpath,$(dirname ${OPENRAVEPY_INT})" -Wl,-rpath,${PREFIX}/lib
