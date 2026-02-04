^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package data_tamer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.0.3 (2025-05-23)
-----------
* Remove ament_target_dependencies usage

1.0.2 (2025-05-12)
-----------
* Merge pull request `#52 <https://github.com/PickNikRobotics/data_tamer/issues/52>`_ from PickNikRobotics/fix_build_farm_3
  Try installing test deps no matter what
* try installing test deps no matter what
* Merge pull request `#51 <https://github.com/PickNikRobotics/data_tamer/issues/51>`_ from PickNikRobotics/fix_build_farm_2
  ignore build ros argument in test CMake
* ignore build ros argument in test
* Merge pull request `#50 <https://github.com/PickNikRobotics/data_tamer/issues/50>`_ from PickNikRobotics/fix_build_farm
  Fix ament GTest usage
* use ament add gtest
* Merge pull request `#49 <https://github.com/PickNikRobotics/data_tamer/issues/49>`_ from PickNikRobotics/fix_gtest_link
  Get gtest from vendor on ROS
* add myself to maintainer list, add package xml scheme
* get gtest from vendor on ROS
* Contributors: Henry Moore

1.0.1 (2025-03-03)
------------------
* force the size of BasicType ot be 1 byte (`#47 <https://github.com/PickNikRobotics/data_tamer/issues/47>`_)
  * force the size of BasicType ot be 1 byte
  This fix the issue when the log is generated on two different computers
  using different compilers (example Linux / QNX)
  * fix
* add file reset capabilities (`#37 <https://github.com/PickNikRobotics/data_tamer/issues/37>`_)
* Enable users to build without ROS (`#36 <https://github.com/PickNikRobotics/data_tamer/issues/36>`_)
  * make building with ROS an option
  * clarify warnings, allow for default-building without ROS
  * switch name of ros build flag
  * add build ros argument for examples
  * fix incorrect message
* Merge pull request `#32 <https://github.com/PickNikRobotics/data_tamer/issues/32>`_ from PickNikRobotics/default_increment_filename
  Allow endless recording
* remove unused variable
* allow for unlimited recording
* use a new mutex wrapper API (`#27 <https://github.com/PickNikRobotics/data_tamer/issues/27>`_)
  * use a new mutx wrapper API
  * fix compilation
  * change docstrings to reference non-deprecated function
  * add now-missing includes
  ---------
  Co-authored-by: Henry Moore <henry.moore@picknik.ai>
* Locked ptr test, documentation, and example (`#24 <https://github.com/PickNikRobotics/data_tamer/issues/24>`_)
  * add locked ptr usage to example
  * update comment relating to locked ptr
  * remove unused headers
  * add test for locked ptr and non-blocking method
  * remove unsafe lockedPtr get function
* Merge pull request `#23 <https://github.com/PickNikRobotics/data_tamer/issues/23>`_ from torsoelectronics/main
  Fix compile error
* Fix build error
* Contributors: Daniel Mouritzen, Davide Faconti, Henry Moore

1.0.0 (2024-04-30)
------------------
* Support lifecycle node for ros2 publisher sink (`#17 <https://github.com/PickNikRobotics/data_tamer/issues/17>`_)
  * Support lifecycle node for ros2 publisher sink
  * Remove unused member variable node\_
  * Add template for both constructors
* more efficient locking of LoggedValue<T> and new clang format
* refactoring custom types
* fix compilation with and without conan
* new clang format
* add mcap to 3rdparty
* Contributors: Davide Faconti, Victor Massagué Respall

0.9.4 (2024-02-02)
------------------
* changed the way registerValue throws if you try registering the same address again
* add unit tests to verify that vectors with changing size are OK
* Contributors: Davide Faconti

0.9.3 (2024-02-01)
------------------
* add std::hash<DataTamer::RegistrationID>
* fix dead-lock
* Contributors: Davide Faconti

0.9.2 (2024-01-30)
------------------
* fix compilation in ament
* Update CMakeLists.txt. Fix `#11 <https://github.com/facontidavide/data_tamer/issues/11>`_
* Contributors: Davide Faconti

0.9.1 (2024-01-12)
------------------
* add support for enums
* renamed folder to data_tamer_cpp
* Contributors: Davide Faconti

0.8.0 (2023-11-30)
------------------
* API change related to CustomSerializers
* Contributors: Davide Faconti

0.7.0 (2023-11-28)
------------------
* recursive_mutex and call it a day
* add MCAP option
* add MCAPSink::stopRecording
* add more types to mcap example
* add ChannelsRegistry::clear()
* extended tests
* bug fixes and more tests
* fix warning
* compute fixed size at compilation time
* new wrappying of TypeDefinition
* refactoring type registry
* major refactoring of custom types
* Contributors: Davide Faconti

0.6.0 (2023-11-23)
------------------
@ add back compatibility to data_tamer_parser
* works correctly with plotjuggler
* fix ROS2 compilation
* Contributors: Davide Faconti

0.5.0 (2023-11-22)
------------------
* preliminary custom type support
* Contributors: Davide Faconti

0.4.1 (2023-11-21)
------------------

0.4.0 (2023-11-21)
------------------
* add again channel name to hash
* bug fixes in schema hash and parsing
* add benchmark
* readme update
* added data_tamer_parser with some samples and testing
* add locked reference
* bug fixes and tests
* refactored API to support containers
* Contributors: Davide Faconti

0.3.0 (2023-11-14)
------------------
* add coverage
* fix bug
* add CI
* unit test added
* allow registering again with new pointer
* add docs
* use custom mutex on linux
* adding ros2 example
* ros2 publisher sink
* Contributors: Davide Faconti

0.2.1 (2023-11-13)
------------------
* fix conan
* fix conan
* Contributors: Davide Faconti

0.2.0 (2023-11-13)
------------------
* First release: supports MCAP sink only
* Contributors: Davide Faconti, Henry Moore
