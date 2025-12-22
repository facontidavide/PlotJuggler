# Date

[![Build Status](https://travis-ci.org/HowardHinnant/date.svg?branch=master)](https://travis-ci.org/HowardHinnant/date)
[![Join the chat at https://gitter.im/HowardHinnant/date](https://badges.gitter.im/HowardHinnant/date.svg)](https://gitter.im/HowardHinnant/date?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

---

**[Try it out on wandbox!](https://wandbox.org/permlink/oyXjibyF680HHoyS)**

## Summary

This is actually several separate C++11/C++14/C++17 libraries:

1. `"date.h"` is a header-only library which builds upon `<chrono>`.  It adds some new `duration` types, and new `time_point` types.  It also adds "field" types such as `year_month_day` which is a struct `{year, month, day}`.  And it provides convenient means to convert between the "field" types and the `time_point` types.

    * Documentation: http://howardhinnant.github.io/date/date.html
    * Video: https://www.youtube.com/watch?v=tzyGjOm8AKo
    * Slides: http://schd.ws/hosted_files/cppcon2015/43/hinnant_dates.pdf

1. `"tz.h"` / `"tz.cpp"`  are a timezone library built on top of the `"date.h"` library.  This timezone library is a complete parser of the IANA timezone database.  It provides for an easy way to access all of the data in this database, using the types from `"date.h"` and `<chrono>`.  The IANA database also includes data on leap seconds, and this library provides utilities to compute with that information as well.

    * Documentation: http://howardhinnant.github.io/date/tz.html
    * Video: https://www.youtube.com/watch?v=Vwd3pduVGKY
    * Slides: http://schd.ws/hosted_files/cppcon2016/0f/Welcome%20To%20The%20Time%20Zone%20-%20Howard%20Hinnant%20-%20CppCon%202016.pdf

1. `"iso_week.h"` is a header-only library built on top of the `"date.h"` library which implements the ISO week date calendar.

    * Documentation: http://howardhinnant.github.io/date/iso_week.html

1. `"julian.h"` is a header-only library built on top of the `"date.h"` library which implements a proleptic Julian calendar which is fully interoperable with everything above.

    * Documentation: http://howardhinnant.github.io/date/julian.html

1. `"islamic.h"` is a header-only library built on top of the `"date.h"` library which implements a proleptic Islamic calendar which is fully interoperable with everything above.

    * Documentation: http://howardhinnant.github.io/date/islamic.html

## Standardization

Slightly modified versions of `"date.h"` and `"tz.h"` were voted into the C++20 working draft at the Jacksonville FL meeting on 2018-03-17:

* http://howardhinnant.github.io/date/d0355r7.html

## Build & Test

The recommended way to use any of these libraries besides `"tz.h"` is to just include it.  These are header-only libraries (except `"tz.h"`).

To use `"tz.h"`, there is a single source file (`src/tz.cpp`) that needs to be compiled. Here are the recommended directions:  https://howardhinnant.github.io/date/tz.html#Installation.

One can run tests by cd'ing into the `test` subdirectory and running `testit`.  There are known failures on all platforms except for macOS.  And even on macOS if C++11 is used.  If any of these failures present problems for you, there exist workarounds.

Additionally there is _unsupported_ support for [vcpkg](https://github.com/Microsoft/vcpkg) and [CMake](https://cmake.org/).  I don't personally use or maintain these systems as for me they cause more problems than they solve (for this small project).  If you would like to contribute to these build systems please feel free to file a PR.

You can download and install Date using the [vcpkg](https://github.com/Microsoft/vcpkg) dependency manager:

    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg integrate install
    vcpkg install date

The Date port in vcpkg is updated by Microsoft team members and community contributors. If the version falls behind, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.

You can optionally build using [CMake](https://cmake.org/). Here is a guide of how to build and test using the CMake Makefile generator.

```bash
mkdir build
cd build
cmake -DENABLE_DATE_TESTING=ON -DBUILD_TZ_LIB=ON ../
cmake --build . --target testit # Consider '-- -j4' for multithreading
```
## Projects using this library

* www.safe.com
* www.webtoolkit.eu/wt
* https://github.com/ViewTouch/viewtouch
* https://routinghub.com
* https://github.com/valhalla
* https://github.com/siodb/siodb
* https://github.com/KomodoPlatform/atomicDEX-Pro
* https://github.com/Kotlin/kotlinx-datetime
* https://github.com/royalbee/jewish_date
* https://github.com/apache/arrow/
* https://lottopark.com

If you would like your project (or product) on this list, just let me know.
