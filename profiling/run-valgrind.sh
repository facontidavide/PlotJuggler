#!/bin/bash -e

# Add this flag to cmake in build.sh if you cannot get debug symbols: -DCMAKE_BUILD_TYPE=RelWithDebInfo

export LD_PROFILE_OUTPUT=$(pwd)
export LD_PROFILE=libDataLoadRlog.so
rm -f $LD_PROFILE.profile

# Run build.sh on your own if ../build exists but files have been updated
[[ -d ../build ]] || (cd ../; sh build.sh)

callgrind_out="callgrind.out.pj.dynamic"

valgrind --tool=callgrind --dump-instr=yes --simulate-cache=yes --collect-jumps=yes \
  --callgrind-out-file=$callgrind_out `pwd`/../build/bin/plotjuggler -d `pwd`/tmpaq3p1j9w.rlog

# Close plotjuggler window manually after it loads log
kcachegrind callgrind.out.pj
