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
OPENRAVE_PYDIR="_"$(<<<${OPENRAVE_DIR} sed -e s/openrave/openravepy/ | tr -- "-." "__") # _openravepy_0_11

g++ -std=gnu++98 -O2 -fPIC -shared -o openrave_rawxml.so \
-I ${PREFIX}/include/${OPENRAVE_DIR} -I ${PREFIX}/include -I /usr/include/python2.7 -I /usr/include/libxml2 \
openrave_rawxml.cpp parsexml.cpp \
${PREFIX}/lib/python2.7/site-packages/openravepy/${OPENRAVE_PYDIR}/openravepy_int.so \
-L ${PREFIX}/lib -lpython2.7 -l$(<<<${OPENRAVE_DIR} tr -d -- -) -lboost_system -lboost_python -llog4cxx -lxml2 \
-Wl,--no-undefined -Wl,-rpath,${PREFIX}/lib/python2.7/site-packages/openravepy/${OPENRAVE_PYDIR} -Wl,-rpath,${PREFIX}/lib
