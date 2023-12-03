^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package plotjuggler
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

3.8.1 (2023-11-23)
------------------
* data_tamer added to rosx_introspection
* Contributors: Davide Faconti

3.8.0 (2023-11-12)
------------------
* data_tamer updated
  This reverts commit 4ba24c591a9a84fbfb6c0329b787d73f98a2b23b.
* CI updated
* qwt updated
* Merge pull request `#869 <https://github.com/facontidavide/PlotJuggler/issues/869>`_ from zdavkeos/step_interpolation
  Add "Steps" when drawing curves
* Merge pull request `#870 <https://github.com/facontidavide/PlotJuggler/issues/870>`_ from MirkoFerrati/patch-3
  Fix missing '22' in the new snap core22 workflow
* Fix missing '22' in the new snap core22 workflow
* Merge pull request `#849 <https://github.com/facontidavide/PlotJuggler/issues/849>`_ from MirkoFerrati/mirko/core22_snap
  update to core22, remove ros1, enable humble instead of foxy
* Add "Steps" when drawing curves
* Remove deprecated messages from foxy
* Skip git security check for git owner inside the snap container
* Adapt to new snapcraft syntax for core22, sort stage-packages
* swap default snap with core22 snap for ros humble
* duplicate the snap github action to publish new humble track
* add snap for core22, remove ros1, enable humble
* Merge pull request `#853 <https://github.com/facontidavide/PlotJuggler/issues/853>`_ from MirkoFerrati/mirko/fix_snap
  remove deprecated msg from snapcraft
* Merge pull request `#846 <https://github.com/facontidavide/PlotJuggler/issues/846>`_ from locusrobotics/fix-catkin-build
  Use a more reliable method to select buildtool
* fix
* MCAP loader is not faster for large files
* fix parsers names
* extend the Toolbox plugin interface
* mcap updated
* remove deprecated msg from snapcraft
* Use a more reliable method to select buildtool
* Merge pull request `#843 <https://github.com/facontidavide/PlotJuggler/issues/843>`_ from faisal-shah/date-time-format-urls
  Add link to QDate format string
* Add 'tab' as a separator in the CSV loader
* Add link to QDate format string
  A link to QTime format string was there, but not QDate
* Merge pull request `#840 <https://github.com/facontidavide/PlotJuggler/issues/840>`_ from jbendes/zcm-improvements
  Zcm improvements
* Moved away from std function for speed
* Fixed loading of selected channels from layout
* Merge pull request `#827 <https://github.com/facontidavide/PlotJuggler/issues/827>`_ from jbendes/zcm
  Added zcm streaming support
* Merge pull request `#834 <https://github.com/facontidavide/PlotJuggler/issues/834>`_ from rinnaz/fix-protobuf-parser-leak
  Fix memory leak in protobuf parser
* Made transport text box wider
* Looking for zcm in alternate directory first
* A bit more stable
* Serializing and deserializing dataloader for zcm in layout
* fix: memory leak in protobuf parser
* Reverted change
* Changed to ZCM_DEFAULT_URL
* ZCM refactored
* ZCM works, with single type file
* Cleaner loading dialogs
* Added progress dialog
* Added channel selection
* Added data loading from files
* A bit of simplification and bug fix
* Added zcm streaming support
* Add missing cstdint include
* Contributors: Davide Faconti, Faisal Shah, Jonathan Bendes, Mirko Ferrati, Paul Bovbel, Rinat Nazarov, Zach Davis, joajfreitas

3.7.0 (2023-05-19)
------------------
* Handle protobuf maps (`#824 <https://github.com/facontidavide/PlotJuggler/issues/824>`_)
  Protobuf maps are just repeated protobuf messages with only 2 fields
  "key" and "value". Extract the map's key and use it in the series name
  and skip adding series for "key" fields to reduce the number of non
  useful series. Additionally don't include "value" in the series name for
  the value of a map.
* add progress dialog to MCAP loading
* new plugin: DataTamer parser
* performance optimization in pushBack
* more information in MCAP error
* optimization in MoveData
* address `#820 <https://github.com/facontidavide/PlotJuggler/issues/820>`_
* Prevent runtime_error exceptions from plugins crashing the main app (`#812 <https://github.com/facontidavide/PlotJuggler/issues/812>`_)
  Catch runtime_error exceptions thrown from the plugins and skip the throwing plugins, so that the main app can continue its normal operation.
* fix(snap): add libzstd for mcap support (`#815 <https://github.com/facontidavide/PlotJuggler/issues/815>`_)
* Update README.md
* Add a "central difference" method of derivative calculation (`#813 <https://github.com/facontidavide/PlotJuggler/issues/813>`_)
* Updating COMPILE dependencies to install (`#810 <https://github.com/facontidavide/PlotJuggler/issues/810>`_)
  Taken from CI: https://github.com/facontidavide/PlotJuggler/blob/main/.github/workflows/ubuntu.yaml#L20-L31
* Fix the bug where the shared library Parquet is not linked (`#807 <https://github.com/facontidavide/PlotJuggler/issues/807>`_)
  The actual path to the shared library is in `${PARQUET_SHARED_LIB}` instead of in
  `${PARQUET_LIBRARIES}`.
* Add CMake into comp vars and minor format improvements (`#804 <https://github.com/facontidavide/PlotJuggler/issues/804>`_)
  Co-authored-by: Erick G. Islas Osuna <eislasosuna@netflix.com>
* Fix for missing preferences (`#795 <https://github.com/facontidavide/PlotJuggler/issues/795>`_)
* fix typos in "tips and tricks" cheatsheet (`#798 <https://github.com/facontidavide/PlotJuggler/issues/798>`_)
  fix a couple of minor typos in dialog text
* Support Proto's That Reference Google/Protobuf (`#793 <https://github.com/facontidavide/PlotJuggler/issues/793>`_)
* Fix for segfault in DataLoadCSV destructor (`#784 <https://github.com/facontidavide/PlotJuggler/issues/784>`_)
  - Change order of deletion for dialogs.
  - First delete child dialog `_dateTime_dialog` then parent
  `_dialog`.
* Add CodeQL workflow (`#765 <https://github.com/facontidavide/PlotJuggler/issues/765>`_)
* [bugfix] String deserialization (`#780 <https://github.com/facontidavide/PlotJuggler/issues/780>`_)
* forgot throw
* fixing nan check (`#777 <https://github.com/facontidavide/PlotJuggler/issues/777>`_)
* Update Reactive Scripts on playback loop (`#771 <https://github.com/facontidavide/PlotJuggler/issues/771>`_)
* fix
* Contributors: Alistair, AndyZe, Bartimaeus-, Connor Anderson, Davide Faconti, Erick G. Islas-Osuna, Guillaume Beuzeboc, Mark Cutler, Michael Orlov, Peter Stöckli, Sam Pfeiffer, Zach Davis, Zheng Qu, augustinmanecy, ozzdemir

3.6.1 (2022-12-18)
------------------
* apply changes suggested in `#767 <https://github.com/facontidavide/PlotJuggler/issues/767>`_
* fix parsing of custom types added multiple times in messages (`#769 <https://github.com/facontidavide/PlotJuggler/issues/769>`_)
* ZMQ: Add topics filtering (`#730 <https://github.com/facontidavide/PlotJuggler/issues/730>`_)
* Add CSV loader date-time format help dialog (`#731 <https://github.com/facontidavide/PlotJuggler/issues/731>`_)
* Set MQTT topics list to be multi-selectable (`#745 <https://github.com/facontidavide/PlotJuggler/issues/745>`_)
* Always use topic names for creating MQTT parsers (`#746 <https://github.com/facontidavide/PlotJuggler/issues/746>`_)
* fix typo (`#770 <https://github.com/facontidavide/PlotJuggler/issues/770>`_)
* Fix/add other dds vendors (`#763 <https://github.com/facontidavide/PlotJuggler/issues/763>`_)
* Add option to build plotjuggler_base to shared library (`#757 <https://github.com/facontidavide/PlotJuggler/issues/757>`_)
* Add a new meme with The Rock (`#751 <https://github.com/facontidavide/PlotJuggler/issues/751>`_)
* Add precision to CSV export to handle geocoordinates (`#753 <https://github.com/facontidavide/PlotJuggler/issues/753>`_)
* compile: add cmake to brew install (`#742 <https://github.com/facontidavide/PlotJuggler/issues/742>`_)
* Add MIT license notice to QCodeEditor dddition (`#733 <https://github.com/facontidavide/PlotJuggler/issues/733>`_)
  Added per https://github.com/facontidavide/PlotJuggler/issues/732
* Fix multi-plugin selection (`#739 <https://github.com/facontidavide/PlotJuggler/issues/739>`_)
  Broken in `#726 <https://github.com/facontidavide/PlotJuggler/issues/726>`_. If all plugins are enabled, then opening a file supported by multiple plugins does not work.
* - Add drag n drop (`#726 <https://github.com/facontidavide/PlotJuggler/issues/726>`_)
  - Ignore VSCode and OS X files
* readme: add details about default snap command (`#727 <https://github.com/facontidavide/PlotJuggler/issues/727>`_)
* Add mac compilation section (`#725 <https://github.com/facontidavide/PlotJuggler/issues/725>`_)
* Update README.md (`#723 <https://github.com/facontidavide/PlotJuggler/issues/723>`_)
  minor typos
* Update README.md
* Update COMPILE.md
* Contributors: Andrew Van Overloop, Bartimaeus-, Bonkura, Davide Faconti, Guillaume Beuzeboc, Jeff Ithier, Jeremie Deray, Mark Cutler, Orhan G. Hafif, Romain Reignier, Zach Davis

3.6.0 (2022-08-13)
------------------
* More memes
* Refactoring of the MessageParser plugins 
* Mcap support (`#722 <https://github.com/facontidavide/PlotJuggler/issues/722>`_)
* Improve CSV loader error handling (`#721 <https://github.com/facontidavide/PlotJuggler/issues/721>`_)
* Fix plotwidget drag and drop bug (Issue `#716 <https://github.com/facontidavide/PlotJuggler/issues/716>`_) (`#717 <https://github.com/facontidavide/PlotJuggler/issues/717>`_)
* fix(snap): remove yaml grade (`#718 <https://github.com/facontidavide/PlotJuggler/issues/718>`_)
  grade is set from the part
  YAML grade has priority over the programmed one so we remove it
* Contributors: Bartimaeus-, Davide Faconti, Guillaume Beuzeboc

3.5.2 (2022-08-05)
------------------
* fix issue `#642 <https://github.com/facontidavide/PlotJuggler/issues/642>`_
* fix FFT toolbox
* Add options for enabling/disabling autozoom in preferences (`#704 <https://github.com/facontidavide/PlotJuggler/issues/704>`_)
* add support for custom window titles (`#715 <https://github.com/facontidavide/PlotJuggler/issues/715>`_)
* Fix/snap rosbag (`#714 <https://github.com/facontidavide/PlotJuggler/issues/714>`_)
* fix mosquitto build in linux
* Better cmake (`#710 <https://github.com/facontidavide/PlotJuggler/issues/710>`_)
* fix `#707 <https://github.com/facontidavide/PlotJuggler/issues/707>`_
* better installation instructions
* fix(snap): reapply changes remove by the merge of main (`#703 <https://github.com/facontidavide/PlotJuggler/issues/703>`_)
* save ColorMaps in layout
* Contributors: Bartimaeus-, Davide Faconti, Guillaume Beuzeboc, grekiki

3.5.1 (2022-07-25)
------------------
* Dev/ros1 ros2 snap (`#698 <https://github.com/facontidavide/PlotJuggler/issues/698>`_)
* update nlohmann json to fix `#640 <https://github.com/facontidavide/PlotJuggler/issues/640>`_
* should prevent error `#696 <https://github.com/facontidavide/PlotJuggler/issues/696>`_
* Merge branch 'improved_zoomout' into main
* cleanup after `#702 <https://github.com/facontidavide/PlotJuggler/issues/702>`_
* Statistics dialog improvements and bug fixes (`#702 <https://github.com/facontidavide/PlotJuggler/issues/702>`_)
* Include std::thread instead of QThread, since it is being utilized in the mqtt plugin instead of QThread. (`#700 <https://github.com/facontidavide/PlotJuggler/issues/700>`_)
* fix zmq compilation
* cherry picking from `#698 <https://github.com/facontidavide/PlotJuggler/issues/698>`_
* increase playback step precision (`#692 <https://github.com/facontidavide/PlotJuggler/issues/692>`_)
* Fix typo in ColorMap warning (`#693 <https://github.com/facontidavide/PlotJuggler/issues/693>`_)
* Set buttonBackground icon in .ui file (`#694 <https://github.com/facontidavide/PlotJuggler/issues/694>`_)
* Update README.md
* Fix `#697 <https://github.com/facontidavide/PlotJuggler/issues/697>`_
* update sol2 and fix `#687 <https://github.com/facontidavide/PlotJuggler/issues/687>`_
* try to improve the linked zoomout
* Contributors: Bartimaeus-, Davide Faconti, Guillaume Beuzeboc, Hugal31, ozzdemir

3.5.0 (2022-07-12)
------------------
* license changed to MPL 2.0
* Macos ci (`#685 <https://github.com/facontidavide/PlotJuggler/issues/685>`_)
* Add CSV table preview and CSV highlighting (`#680 <https://github.com/facontidavide/PlotJuggler/issues/680>`_)
  * Add CSV table preview and CSV highlighting
  * add toggles for enabling CSV table view and syntax highlighting
* Fix start/end time bug in CSV Exporter (`#682 <https://github.com/facontidavide/PlotJuggler/issues/682>`_)
* Add tooltips to CSV publisher buttons (`#683 <https://github.com/facontidavide/PlotJuggler/issues/683>`_)
  -Add tooltips to the buttons that set the start/end time based on vertical time tracker position
  -add missing space in text ("timerange" to "time range")
* Fix `#415 <https://github.com/facontidavide/PlotJuggler/issues/415>`_
* add statistics
* Add background editor
* fix crash in Parquet plugin
* Add line numbers to csv loader (`#679 <https://github.com/facontidavide/PlotJuggler/issues/679>`_)
* Fix type-o in reactive script editor (`#678 <https://github.com/facontidavide/PlotJuggler/issues/678>`_)
  missing "r" in "ScatterXY"
* Contributors: Bartimaeus-, Davide Faconti

3.4.5 (2022-06-29)
------------------
* fix compilation
* add QCodeEditor
* CI: cmake ubuntu/Windows
* Fix CSV generated time axis. (`#666 <https://github.com/facontidavide/PlotJuggler/issues/666>`_)
  Previously the CSV dataload plugin was not saving the correct XML state
  when a generated time axis was used.
* Added support for converted int types (`#673 <https://github.com/facontidavide/PlotJuggler/issues/673>`_)
  * Added support for converted int types
  * Added fallback for int32 and int64
  Co-authored-by: Rano Veder <r.veder@primevision.com>
* Add tooltip to the zoom out button (`#670 <https://github.com/facontidavide/PlotJuggler/issues/670>`_)
* PlotJuggler will generate its own cmake target
* Parquet plugin (`#664 <https://github.com/facontidavide/PlotJuggler/issues/664>`_)
* fix Cancel button in CSV loader (`#659 <https://github.com/facontidavide/PlotJuggler/issues/659>`_)
* Make tutorial link open in browser when clicked (`#660 <https://github.com/facontidavide/PlotJuggler/issues/660>`_)
  Similar to https://github.com/facontidavide/PlotJuggler/pull/658 but applied to the tutorial link in the reactive lua editor
* accept white lines in CSV
* Update README.md (`#661 <https://github.com/facontidavide/PlotJuggler/issues/661>`_)
* Make link open in browser when clicked (`#658 <https://github.com/facontidavide/PlotJuggler/issues/658>`_)
  Set openExternalLinks property of label_4 to true to allow the hyperlink to open in a web browser when clicked
* Fix  `#655 <https://github.com/facontidavide/PlotJuggler/issues/655>`_. Add autoZoom to transform dialog
* Rememvber CSV time column. Cherry picking from `#657 <https://github.com/facontidavide/PlotJuggler/issues/657>`_.
* fix `#650 <https://github.com/facontidavide/PlotJuggler/issues/650>`_
* Contributors: Andrew Goessling, Bartimaeus-, Davide Faconti, Konstantinos Lyrakis, Rano Veder, Zach Davis

3.4.4 (2022-05-15)
------------------
* fix issue `#561 <https://github.com/facontidavide/PlotJuggler/issues/561>`_
* add STATUS to CmakeLists.txt message() to avoid 'message called with incorrect number of arguments' (`#649 <https://github.com/facontidavide/PlotJuggler/issues/649>`_)
  cmake 3.22.1 errors on this
* Passing CI on ROS2 Rolling (`#629 <https://github.com/facontidavide/PlotJuggler/issues/629>`_)
  * fix ament-index-cpp dependency on ubuntu jammy
  * add rolling ci
* Modify install command and make it easier to install (`#620 <https://github.com/facontidavide/PlotJuggler/issues/620>`_)
* Contributors: Davide Faconti, Kenji Brameld, Krishna, Lucas Walter

3.4.3 (2022-03-06)
------------------
* Apply changes to reactive Scripts
* improve reactive Scripts
* clear selections when CustomSeries is created
* save batch function settings
* cleaning up `#601 <https://github.com/facontidavide/PlotJuggler/issues/601>`_
* Timestampfield (`#601 <https://github.com/facontidavide/PlotJuggler/issues/601>`_)
* add new batch editor
* check validity of the Lua function
* consolidate tree view
* add missing files and use CurveTree
* multifile prefix
* ReactiveLuaFunction cleanup
* adding absolute transform
* small UI fix
* Contributors: Davide Faconti, ngpbach

3.4.2 (2022-02-12)
------------------
* delete orhphaned transforms
* bug fix that cause crash
* fix error `#603 <https://github.com/facontidavide/PlotJuggler/issues/603>`_
* Fix `#594 <https://github.com/facontidavide/PlotJuggler/issues/594>`_
* Contributors: Davide Faconti

3.4.1 (2022-02-06)
------------------
* add flip axis
* fix zoom in icon
* Fix typo in toolbox Lua (`#598 <https://github.com/facontidavide/PlotJuggler/issues/598>`_)
* Fix MutableTimeseries shadowed by MutableScatterXY (`#597 <https://github.com/facontidavide/PlotJuggler/issues/597>`_)
  * Fix MutableTimeseries shadowed by MutableScatterXY
  * add math library
  Co-authored-by: Simon CHANU <simon.chanu@cmdl.pro>
* MQTT upgraded
* Update README.md
* Installer and readme updates
* Contributors: Davide Faconti, SebasAlmagro, Simon CHANU

3.4.0 (2022-01-29)
------------------
* fix `#585 <https://github.com/facontidavide/PlotJuggler/issues/585>`_
* fix `#560 <https://github.com/facontidavide/PlotJuggler/issues/560>`_
* fix `#575 <https://github.com/facontidavide/PlotJuggler/issues/575>`_
* Reactive scripts (`#589 <https://github.com/facontidavide/PlotJuggler/issues/589>`_)
* Fix Quaternion toolbox, issue `#587 <https://github.com/facontidavide/PlotJuggler/issues/587>`_
* fix double delete
* fix memory leaks `#582 <https://github.com/facontidavide/PlotJuggler/issues/582>`_
* Contributors: Davide Faconti

3.3.5 (2022-01-04)
------------------
* fix zoom issue when toggling T_offset
* cosmetic changes
* show missing curves in error dialog (`#579 <https://github.com/facontidavide/PlotJuggler/issues/579>`_)
* fix `#550 <https://github.com/facontidavide/PlotJuggler/issues/550>`_
* Contributors: Adeeb Shihadeh, Davide Faconti

3.3.4 (2021-12-28)
------------------
* Video plugin (`#574 <https://github.com/facontidavide/PlotJuggler/issues/574>`_)
* gitignore *.swp files (`#569 <https://github.com/facontidavide/PlotJuggler/issues/569>`_)
* Added libprotoc-dev to the apt install targets (`#573 <https://github.com/facontidavide/PlotJuggler/issues/573>`_)
* turn on Sol3 safety flag
* trying to solve reported issue with Lua
* add fields that were not set in Protobuf
* Protobuf update (`#568 <https://github.com/facontidavide/PlotJuggler/issues/568>`_)
* add zoomOut after loadDataFile
* Protobuf options refactored
* changed the protobuf implementation to deal with dependencies
* Protobuf parser and MQTT plugins
* Merge pull request `#531 <https://github.com/facontidavide/PlotJuggler/issues/531>`_ from erickisos/fix/517
  Homebrew path added into CMakeLists `#517 <https://github.com/facontidavide/PlotJuggler/issues/517>`_
* LUA version updated
* fix dependency between transformed series
* fix issue `#557 <https://github.com/facontidavide/PlotJuggler/issues/557>`_
* Homebrew path added into CMakeLists
* Contributors: Adeeb Shihadeh, Davide Faconti, Erick G. Islas-Osuna, Miklós Márton

3.3.3 (2021-10-30)
------------------
* Fix critical bug when loading a file twice
* change order of removal
* fix crash when one of the source of XY is deleted
* fix issue `#549 <https://github.com/facontidavide/PlotJuggler/issues/549>`_ (comma decima separator)
* Fix issue `#545 <https://github.com/facontidavide/PlotJuggler/issues/545>`_
* Contributors: Davide Faconti

3.3.2 (2021-10-21)
------------------
* don't add the prefix. Checkbox added
* bug fix when accidentally merging datafiles
* clang-format
* Contributors: Davide Faconti

3.3.1 (2021-10-04)
------------------
* fix `#527 <https://github.com/facontidavide/PlotJuggler/issues/527>`_
* avoid shared libraries in libkissFFT
* Fix `#524 <https://github.com/facontidavide/PlotJuggler/issues/524>`_ and `#529 <https://github.com/facontidavide/PlotJuggler/issues/529>`_
* Fix bug with Outlier Removal (`#532 <https://github.com/facontidavide/PlotJuggler/issues/532>`_)
* minor changes
* Implement Moving RMS filter `#510 <https://github.com/facontidavide/PlotJuggler/issues/510>`_
* Fix issue `#516 <https://github.com/facontidavide/PlotJuggler/issues/516>`_
  - Don't show more than once "Do you want to delete old data" when
  loading multiple files.
  - Correctly clean all the data, including _loaded_datafiles
* Update README.md
* Contributors: Davide Faconti

3.3.0 (2021-09-07)
------------------
* add "start_streamer" option
* Support MacOS and Dark Mode
* custom SplitLine function in CSV. Fix `#509 <https://github.com/facontidavide/PlotJuggler/issues/509>`_
* fix issue  `#507 <https://github.com/facontidavide/PlotJuggler/issues/507>`_
* New CSV plugin
* Back to static libraries ( `#507 <https://github.com/facontidavide/PlotJuggler/issues/507>`_)
* Fixed wrong slot name on PreferecesDialog and moved skin-based setting of MainWindowTitle after the setupUi() call
* fix typo in preferences dialog
* add notifications from Streaming plugins (`#489 <https://github.com/facontidavide/PlotJuggler/issues/489>`_)
* cherry picking features from `#489 <https://github.com/facontidavide/PlotJuggler/issues/489>`_
  - new options [enabled_plugins] and [disabled_plugins]
  - new option [skin_path]
* remove potential issue with TransformFunction::reset
* remove .appveyor.yml
* prefer the PlotAttribute enum instead of string
* add changes similar to `#424 <https://github.com/facontidavide/PlotJuggler/issues/424>`_
* Fast Fourier Transform plugin added
* ToolboxQuaternion added
* toolbox plugins introduced
* add latest fmt
* unified TransformFunction
* change name of Transforms plugins
* Contributors: Davide Faconti, GerardoPardo, myd7349

3.2.1 (2021-06-20)
------------------
* adding string reference
* qwt updated and fix for `#463 <https://github.com/facontidavide/PlotJuggler/issues/463>`_
* fix `#461 <https://github.com/facontidavide/PlotJuggler/issues/461>`_
* add quaternion to Euler conversion snippets (`#459 <https://github.com/facontidavide/PlotJuggler/issues/459>`_)
  Add 3 functions to convert a Hamiltonian attitude quaternion to its Euler (Trait-Bryan 321) representation
* fix typo when building without ROS support (`#460 <https://github.com/facontidavide/PlotJuggler/issues/460>`_)
* Update README.md
* Contributors: Davide Faconti, Mathieu Bresciani, Nuno Marques

3.2.0 (2021-06-13)
------------------
* file removed
* fix potential bug in StringSeries
* fix rebase
* apply color and style recursively in a group
* delete button added. CPU optimized
* apply the array visualization in the curvelist_panel itself
* bug fix
* add deleteSerieFromGroup
* Fix "TextColor" in dark mode
* fix PlotGroup and new attributes
* multiple changes
  - remove redundant importPlotDataMapHelper
  - add "text_color" attribute
  - change the way _replot_timer works (one shot triggered by
  DataStreamer::dataReceived() )
* adding PlotGroups and alternative "tree_name"
* bug fix
* fix issue when starting streaming plugins (add placeholders)
* string series seems to work
* WIP
* embracing C++17 and new data strucutre to accomodate more types
* Updated support for windows build + installer (`#396 <https://github.com/facontidavide/PlotJuggler/issues/396>`_)
  Added win32build.bat batch file for easy windows builds (need to update QT path variables inside to correct ones in case it does not work)
* Fix issue `#453 <https://github.com/facontidavide/PlotJuggler/issues/453>`_, `#419 <https://github.com/facontidavide/PlotJuggler/issues/419>`_ and `#405 <https://github.com/facontidavide/PlotJuggler/issues/405>`_ . Ulog path in Windows
* Lag and crash fixed (`#455 <https://github.com/facontidavide/PlotJuggler/issues/455>`_)
  * reduce lag when looking for streams
  * crash fixed when lsl stream start and stop
  * select all button added for LSL plugin
* Update README.md
* Update appimage.md
* Contributors: Celal Savur, Davide Faconti, alkaes

3.1.2 (2021-06-03)
------------------
* add disable_opnegl option in command line
* new API for MessagePublishers
* bug fix that affects statepublishers
  crash when application is closed
* bug fix in Plotwidget transform
* AppImage instructions added
* fix `#445 <https://github.com/facontidavide/PlotJuggler/issues/445>`_
* change to QHostAddress::Any in UDP plugin (issue `#410 <https://github.com/facontidavide/PlotJuggler/issues/410>`_)
* Contributors: Davide Faconti

3.1.1 (2021-05-16)
------------------
* ulog: ignore parameter default message (`#413 <https://github.com/facontidavide/PlotJuggler/issues/413>`_)
* Fix typo in "load transformations" prompt (`#416 <https://github.com/facontidavide/PlotJuggler/issues/416>`_)
* added CSV export plugin
* fix opengl preference
* added options to enable OpenGL and TreeView
* Add libqt5x11extras5-dev into installation guide for fedora/ubuntu users. (`#418 <https://github.com/facontidavide/PlotJuggler/issues/418>`_)
* Fix issue `#405 <https://github.com/facontidavide/PlotJuggler/issues/405>`_ with ULOG in windows
* Use format string when time index is not a number (`#406 <https://github.com/facontidavide/PlotJuggler/issues/406>`_)
* XY curve markers: fixed colors and removed ghosts symbols (`#407 <https://github.com/facontidavide/PlotJuggler/issues/407>`_)
* Updated support for windows build + installer (`#396 <https://github.com/facontidavide/PlotJuggler/issues/396>`_)
* fix warnings and move to C++17
* fix warnings in MSVS
* Contributors: Beat Küng, Davide Faconti, Faisal Shah, Gabriel, Shawn, alessandro, alkaes

3.1.0 (2021-01-31)
------------------
* fix issue `#394 <https://github.com/facontidavide/PlotJuggler/issues/394>`_
* Update udp_server.cpp (`#393 <https://github.com/facontidavide/PlotJuggler/issues/393>`_)
  Fixes random corruptions of UDP Json messages (garbage collector related?)
* Fix style in Windows (`#390 <https://github.com/facontidavide/PlotJuggler/issues/390>`_)
* Fix compilation in C++17
* fix issue `#389 <https://github.com/facontidavide/PlotJuggler/issues/389>`_
* remove qrand
* Add better help dialog to custom functions
* Allow custom function return multiple points (`#386 <https://github.com/facontidavide/PlotJuggler/issues/386>`_)
* Apple Mac M1 build fix. (`#392 <https://github.com/facontidavide/PlotJuggler/issues/392>`_)
  backward-cpp dependency fix for ARM 64 backport, wrong access to PC register.
* fix issue `#384 <https://github.com/facontidavide/PlotJuggler/issues/384>`_
* temporary remove LSL
* Contributors: David CARLIER, Davide Faconti, Hugal31, alkaes

3.0.7 (2021-01-05)
------------------
* Add plugin folders in the preference dialog
* fix issue `#370 <https://github.com/PlotJuggler/PlotJuggler/issues/370>`_: libDataStreamMQTT compilation with Clang
* fix command line options
* change the way ROS path are added t othe list of plugins
* fixing windows builds, for real this time. (`#379 <https://github.com/PlotJuggler/PlotJuggler/issues/379>`_)
* fix bug when datapoints are cleared
* remember the directory in the FunctionEditor
* moved file svg_util
* Add warning when a CSV file is malformed, and suggested in `#378 <https://github.com/PlotJuggler/PlotJuggler/issues/378>`_
* Fixed message_parser plugin loading segfault (`#376 <https://github.com/PlotJuggler/PlotJuggler/issues/376>`_)
* Contributors: Davide Faconti, Jordan McMichael, davide

3.0.6 (2020-12-24)
------------------
* fix issue  `#372 <https://github.com/PlotJuggler/PlotJuggler/issues/372>`_ (install didn't work)
* Update rangeX during streaming
* LabStreamlayer (LSL) plugin is developed. (`#355 <https://github.com/PlotJuggler/PlotJuggler/issues/355>`_)
* Update CMakeLists.txt (`#363 <https://github.com/PlotJuggler/PlotJuggler/issues/363>`_)
* Contributors: Celal Savur, Davide Faconti, Tobias Fischer

3.0.5 (2020-12-10)
------------------
* fix a crash when data is cleared during streaming (LuaCustomFunction)
* should fix issue `#360 <https://github.com/PlotJuggler/PlotJuggler/issues/360>`_ with stylesheet
* fix bug `#359 <https://github.com/PlotJuggler/PlotJuggler/issues/359>`_
* fix compilation error
* Some template types have an enum ItemType. MSVC fails with compilation (`#358 <https://github.com/PlotJuggler/PlotJuggler/issues/358>`_)
  error.
* Add required Qt5::Network for DataStreamUDP (`#356 <https://github.com/PlotJuggler/PlotJuggler/issues/356>`_)
* Contributors: Davide Faconti, Tobias Fischer, gabm

3.0.4 (2020-12-04)
------------------
* Lua ans Sol updated (c++17
* bug fix `#350 <https://github.com/PlotJuggler/PlotJuggler/issues/350>`_ (crash in lua)
* Contributors: Davide Faconti

3.0.2 (2020-11-28)
------------------
* fix icon color in dark mode
* updated to latest Qads
* temporary fix for `#349 <https://github.com/PlotJuggler/PlotJuggler/issues/349>`_
* link updated
* use correct dependency
* fix issue `#348 <https://github.com/PlotJuggler/PlotJuggler/issues/348>`_
* Contributors: Davide Faconti

3.0.0 (2020-11-23)
------------------
* Trying to fix issue `#346 <https://github.com/facontidavide/PlotJuggler/issues/346>`_
* Massive refactoring
* Contributors: Davide Faconti

2.8.4 (2020-08-15)
------------------
* readme updated
* fix issue `#318 <https://github.com/facontidavide/PlotJuggler/issues/318>`_
* fix  `#170 <https://github.com/facontidavide/PlotJuggler/issues/170>`_ : problem with ULOG parser in Windows
* build fixes to work on ROS2 eloquent (`#314 <https://github.com/facontidavide/PlotJuggler/issues/314>`_)
* add qtpainterpath.h (`#313 <https://github.com/facontidavide/PlotJuggler/issues/313>`_)
* Update datastream_sample.cpp
* Update contributors.txt
* Fix another sprintf buffer size warning (`#303 <https://github.com/facontidavide/PlotJuggler/issues/303>`_)
* Contributors: Akash Patel, Davide Faconti, Lucas, Mike Purvis

2.8.3 (2020-07-11)
------------------
* more memes
* "New versione vailable" improved
* fix segmentation fault when tryin reconnect to ROS master
* Contributors: Davide Faconti

2.8.2 (2020-07-07)
------------------
* might fix issue `#301 <https://github.com/facontidavide/PlotJuggler/issues/301>`_
* fix warnings
* fix potential mutex problem related to `#300 <https://github.com/facontidavide/PlotJuggler/issues/300>`_
* bug fix
* Update package.xml
* updated gif
* cherry picking changes from `#290 <https://github.com/facontidavide/PlotJuggler/issues/290>`_
* fix `#296 <https://github.com/facontidavide/PlotJuggler/issues/296>`_
* fix issues on windows Qt 5.15
* fix error
* move StatePublisher to tf2
* revert changes
* fix warnings
* Contributors: Davide Faconti

2.8.1 (2020-05-28)
------------------
* fix critical bug in streaming ROS plugin
* Contributors: Davide Faconti

2.8.0 (2020-05-24)
------------------
* Update CMakeLists.txt
* Added graph context menu description (`#288 <https://github.com/facontidavide/PlotJuggler/issues/288>`_)
* Update FUNDING.yml
* Merge branch 'master' of https://github.com/facontidavide/PlotJuggler
* finished with refactoring
* WIP: re publisher ROS2
* added stuff to dataload_ros2
* Update appimage_howto.md
* fix package name
* embrace pj_msgs (https://github.com/facontidavide/plotjuggler_msgs)
* new clang format and fix in header_stamp usage
* removed marl and rule editing
* more parsers added
* more or less working
* save computation like a champ with plot_data in each parser
* precompute strings only once
* fix compilation on ROS1
* Merge branch 'master' of https://github.com/facontidavide/PlotJuggler
* builtin parsers added
* Githug actions win (`#284 <https://github.com/facontidavide/PlotJuggler/issues/284>`_)
  * try compiling on windows
  * Update windows.yaml
  * multiple workflows
  * Update README.md
  Co-authored-by: daf@blue-ocean-robotics.com <Davide Faconti>
* bug fix
* segfault fixed in TypeHasHeader
* removed rosdep of pj_msgs
* added pj_msgs to ROS2
* fix errors
* heavy refactoring of ROS2 plugins
* critical bug fix in ROS2 parsing
* try to fix problem with StringTreeLeaf::toStr
* reduce a bit allocations overhead
* reduce memory used by the job queue of marl, with periodic flushes
* Contributors: Davide Faconti, Ilya Petrov

2.7.0 (2020-05-03)
------------------
* Merge branch 'ros2' of https://github.com/facontidavide/PlotJuggler into ros2
* added github actions for ros2
* last fixes to DataStreamROS2
* implemented DataLoadRosBag2
* compile with ament/colcon
* Contributors: Davide Faconti

2.6.4 (2020-04-30)
------------------
* Fix the damn icons
* marl updated
* fix issue `#281 <https://github.com/facontidavide/PlotJuggler/issues/281>`_
* catch exception in marl
* fix backward-cpp
* Implement feature `#274 <https://github.com/facontidavide/PlotJuggler/issues/274>`_
* Implement feature `#269 <https://github.com/facontidavide/PlotJuggler/issues/269>`_
* Contributors: Davide Faconti

2.6.3 (2020-04-07)
------------------
* Fix issue `#271 <https://github.com/facontidavide/PlotJuggler/issues/271>`_
* @veimox added
* Bugfix/executable (`#264 <https://github.com/facontidavide/PlotJuggler/issues/264>`_)
  * created launching script , installing and making use of it in the icon
  * ignoring temporary folders when creating binary locally
  * corrected intsallation of script
  * using PROGRAM to install it with executable permissions
  Co-authored-by: Jorge Rodriguez <jr@blue-ocean-robotics.com>
* Feature/scalable icon (`#265 <https://github.com/facontidavide/PlotJuggler/issues/265>`_)
  * installing icons in /usr/share and do it at any build type
  * added scalable icon
  * removed old icon
  Co-authored-by: Jorge Rodriguez <jr@blue-ocean-robotics.com>
* fix default suffix
* Fix bug `#258 <https://github.com/facontidavide/PlotJuggler/issues/258>`_
* Contributors: Davide Faconti, Jorge Rodriguez

2.6.2 (2020-02-25)
------------------
* bug fix in IMU parser
* added step size for the time tracker
* fis issue `#256 <https://github.com/facontidavide/PlotJuggler/issues/256>`_ (new release dialog)
* Update README.md
* Contributors: Davide Faconti

2.6.1 (2020-02-21)
------------------
* fix issue `#253 <https://github.com/facontidavide/PlotJuggler/issues/253>`_ and some cleanup
* fix issue `#254 <https://github.com/facontidavide/PlotJuggler/issues/254>`_
* Fix `#251 <https://github.com/facontidavide/PlotJuggler/issues/251>`_
* Contributors: Davide Faconti

2.6.0 (2020-02-19)
------------------
* bug fix
* fix splashscreen delay
* GUI refinement
* regex filter removed. bug fix in column resize
* new icons in CurveList panel
* add text placeholder
* smaller buttons
* moved buttons to top right corner to gain more space
* changed style (sharper corners)
* bug fix: potential crash trying to save data into rosbag
* more ememes `#248 <https://github.com/facontidavide/PlotJuggler/issues/248>`_
* bug fix in Lua functions
* cleanups
* Merge branch 'lua_scripting'
* Adding custom parser for Imu message (issue `#238 <https://github.com/facontidavide/PlotJuggler/issues/238>`_)
* remember the last value in the function editor
* minor update
* Both javascript and Lua langiages can be selected in preferences
* WIP to support both QML and Lua
* fix menu bar size of PlotJuggler
* scripting moved to Lua
* adding lua stuff to 3rd party libraries
* preliminary change to support `#244 <https://github.com/facontidavide/PlotJuggler/issues/244>`_ (`#247 <https://github.com/facontidavide/PlotJuggler/issues/247>`_)
* preliminary change to support `#244 <https://github.com/facontidavide/PlotJuggler/issues/244>`_
* Update .appveyor.yml
* Update README.md
* Update .appveyor.yml
* Update .appveyor.yml
* further cleanup
* moved files and cleanup
* Contributors: Davide Faconti

2.5.1 (2020-02-07)
------------------
* Fixed slow Menu Bar
* Use ordered map, appendData needs to insert data in order (`#245 <https://github.com/facontidavide/PlotJuggler/issues/245>`_)
  Otherwise the time order may not be respected and the data is loaded
  incorrectly
* prevent call of dropEvent() when not needed
* fix issue `#239 <https://github.com/facontidavide/PlotJuggler/issues/239>`_
* add include array header file to fix build error (`#234 <https://github.com/facontidavide/PlotJuggler/issues/234>`_)
* Contributors: Davide Faconti, Victor Lopez, xiaowei zhao

2.5.0 (2019-12-19)
------------------
* Fix issues `#196 <https://github.com/facontidavide/PlotJuggler/issues/196>`_ and `#236 <https://github.com/facontidavide/PlotJuggler/issues/236>`_: allow user to use deterministic color sequence
* fix the edit button
* fix issue `#235 <https://github.com/facontidavide/PlotJuggler/issues/235>`_
* Update appimage_howto.md
* fix timestamp problem in streaming
* Contributors: Davide Faconti

2.4.3 (2019-11-21)
------------------
* less dark theme
* bug fix
* Contributors: Davide Faconti

2.4.2 (2019-11-18)
------------------
* multithread ROS DataLoader
* directories moved
* manually resizable columns of table view
* Contributors: Davide Faconti

2.4.1 (2019-11-11)
------------------
* considerable speed improvement when MANY timeseries are loaded
* bug fix: slow update of left curve table
* AppImage update
* meme update
* Contributors: Davide Faconti

2.4.0 (2019-11-10)
------------------
* Tree view  (`#226 <https://github.com/facontidavide/PlotJuggler/issues/226>`_)
* fix issue `#225 <https://github.com/facontidavide/PlotJuggler/issues/225>`_
* add version number of the layout syntax
* fix issue `#222 <https://github.com/facontidavide/PlotJuggler/issues/222>`_
* more readable plugin names
* fix issue `#221 <https://github.com/facontidavide/PlotJuggler/issues/221>`_
* Merge branch 'master' of github.com:facontidavide/PlotJuggler
* minor bug fix
* Contributors: Davide Faconti

2.3.7 (2019-10-30)
------------------
* Dont take invisible curve into account for axis limit computation (`#185 <https://github.com/facontidavide/PlotJuggler/issues/185>`_)
* consistent line width
* do not close() a rosbag unless you accepted the dialog
* important bug fix: stop playback when loading new data
* fix bug in TopicPublisher
* do complete reset of globals in custom functions
* apply changes discussed in `#220 <https://github.com/facontidavide/PlotJuggler/issues/220>`_
* Merge branch 'master' of github.com:facontidavide/PlotJuggler
* cherry picking bug fix from `#220 <https://github.com/facontidavide/PlotJuggler/issues/220>`_ : update custom functions
  Thanks @aeudes
* Fix F10 is ambiguous (`#219 <https://github.com/facontidavide/PlotJuggler/issues/219>`_)
* fix compilation and add feature `#218 <https://github.com/facontidavide/PlotJuggler/issues/218>`_
* qwt updated
* appImage instructions updated
* Contributors: Davide Faconti, alexandre eudes

2.3.6 (2019-10-16)
------------------
* fix issue `#215 <https://github.com/facontidavide/PlotJuggler/issues/215>`_
* Contributors: Davide Faconti

2.3.5 (2019-10-11)
------------------
* remember the size of the splitter
* fix inveted XY
* Contributors: Davide Faconti
* remember last splashscreen
* Update README.md
* Update appimage_howto.md
* fix warning
* meme fixed
* Contributors: Davide Faconti

2.3.4 (2019-10-03)
------------------
* prepare "meme edition"
* Merge branch 'master' of https://github.com/facontidavide/PlotJuggler
* RosMsgParsers: add cast to be clang compatible (#208)
* Update README.md
* Update FUNDING.yml
* Correct "Github" to "GitHub" (#206)
* 2.3.3
* fix issue with FMT
* Contributors: Dan Katzuv, Davide Faconti, Timon Engelke

2.3.3 (2019-10-01)
------------------
* removed explicit reference to Span
* remove abseil dependency (to be tested)
* Contributors: Davide Faconti

2.3.2 (2019-09-30)
------------------
* always use random color in addCurveXY
* Fix issue #204
* Fix issue #203
* Add missed absl Span<T> header include
* Add missed abseil_cpp depend
* Contributors: Davide Faconti, Enrique Fernandez

2.3.1 (2019-09-24)
------------------
* Fix `#202 <https://github.com/facontidavide/PlotJuggler/issues/202>`_ use_header_stamp not initialized for built-in types
* Merge pull request `#200 <https://github.com/facontidavide/PlotJuggler/issues/200>`_ from aeudes/multiple_streamer
  data stream topic plugin
* new color palette
* Allow to have working datastreamtopic plugin in more than one plotjuggler
  instance
* adding covariance to Odometry msg again
* fix issue `#187 <https://github.com/facontidavide/PlotJuggler/issues/187>`_
* Fix segfault when swap plotwidget on archlinux (qt5.12.3).
  This bug is introduced in: 7959e54 Spurious DragLeave fixed?
  And produce a segfault(nullptr) in QCursor::shape() call by
  QBasicDrag::updateCursor(Qt::DropAction) [trigger by plotwidget.cpp:1352
  drag->exec();].
  It seems to me that the change of global application cursor on leave event during drag drop
  operation cause the problem [is it the drop widget duty to reset cursor?].
* minor fixes related to dark theme
* Contributors: Alexandre Eudes, Davide Faconti

2.3.0 (2019-07-11)
------------------
* Countless changes and merges of PR.
* Contributors: Alexandre Eudes, Davide Faconti, Juan Francisco Rascón Crespo, alexandre eudes

2.1.10 (2019-03-29)
-------------------
* critical bug fixed in CustomFunctions
* Contributors: Davide Faconti

2.1.9 (2019-03-25)
------------------
* QwtRescaler replaced
* fix issues related to #118 (PlotZoom)
* Contributors: Davide Faconti

2.1.8 (2019-03-24)
------------------
* bug fixes
* xy equal scaling seems to work
* Super fancy Video cheatsheet (#164)
* better date display
* Fix issue #161 and remember last directory used
* mainwindow - use yyyy-MM-dd_HH-mm-ss name when saving a plot as png. This allows to save several times without having to rename the previous image (#162)
* Contributors: Davide Faconti, bresch

2.1.7 (2019-03-20)
------------------
* Date time visualization on X axis
* fix slow PLAY when rendering takes more than 20 msec
* new way to zoom a single axis (issues #153 and #135)
* Inverted mouse wheel zoom #153
* On MacOS there are several mime formats generated in addition to "curveslist", this fix will keep curves array with names collected instead of resetting it for each new mime format. (#159)
* ulog_parser: fixed parsing of array topics (#157)
  Signed-off-by: Roman <bapstroman@gmail.com>
* fis issue  #156 : catch expections
* remember if the state of _action_clearBuffer
* QSettings cleanups
* Contributors: Alexey Zaparovanny, Davide Faconti, Roman Bapst

2.1.6 (2019-03-07)
------------------
* removed obsolate question
* remember RemoveTimeOffset state
* add clear buffer from data stream
* reject non valid data
* fix sorting in ULog messages
* Fix Ulog window
* ulog plugin improved
* Update .appveyor.yml
* yes, I am sure I want to Quit
* simplifications in RosoutPublisher
* better double click behavior in FunctionEditor
* adding Info and parameters
* big refactoring of ulog parser. Fix issue #151
* download links updated
* Contributors: Davide Faconti

2.1.5 (2019-02-25)
------------------
* reintroducing timestamp from header
* added way to create installer
* disable zooming during streaming and reset tracker when new file loaded
* Contributors: Davide Faconti

2.1.4 (2019-02-21)
------------------
* Fix issues #146: ULog and multiple instances of a message
* close issue #138
* remove svg dependency
* Appveyor fixed (#144)
* fancy menubar
* Contributors: Davide Faconti

2.1.3 (2019-02-18)
------------------
* BUG: fixed issue with Customtracker when the plot is zoomed
* new icons
* ULog plugin added
* Allow to build the DataStreamClientSample on Linux (#143)
* Update README.md
* Contributors: Davide Faconti, Romain Reignier

2.1.2 (2019-02-13)
------------------
* legend button now has three states: left/right/hide
* replace tracker text when position is on the right side
* allow again to use the header.stamp
* fix problem with legend visibility
* Save all tab plots as images in a folder. (#137)
* Make default filename for tab image the tab name (#136)
* Update README.md
* adding instructions to build AppImage
* Contributors: Davide Faconti, d-walsh

2.1.1 (2019-02-07)
------------------
* Added filter to function editor
* ask for support
* cleanup
* fix issue with Datetime and cheatsheet dialog
* further stylesheet refinements
* fixing visualization of fucntion editor dialog
* fixing html of cheatsheet
* Contributors: Davide Faconti

2.1.0 (2019-02-07)
------------------
* minor change
* stylesheet fix
* Cheatsheet added
* fixing style
* improved magnifier ( issue #135)
* added zoom max
* Contributors: Davide Facont, Davide Faconti

2.0.7 (2019-02-06)
------------------
* fix for dark layout
* fix issue with edited function transforms
* about dialog updated
* added more key shortcuts
* reverted behaviour of Dialog "delete previous curves"?
* fix glitches related to drag and drop
* update timeSlider more often
* play seems to work properly for both sim_time and rewritten timestamps
* play button added
* clock published
* remove timestamp modifier
* Contributors: Davide Faconti

2.0.5 (2019-02-05)
------------------
* fix problem in build farm
* bug fix plot XY
* Contributors: Davide Faconti

2.0.4 (2019-01-29)
------------------
* add parent to message boxes
* ask confirmation at closeEvent()
* fix problem with selection of second column
* fix issue 132
* simplification
* minor bug fixed in filter of StatePublisher
* Contributors: Davide Facont, Davide Faconti

2.0.3 (2019-01-25)
------------------
* adding descard/clamp policy to large arrays
* fix problem with table view resizing
* make size of fonts modifiable with CTRL + Wheel (issue #106)
* Update .travis.yml
* Contributors: Davide Faconti

2.0.2 (2019-01-23)
------------------
* should solve issue #127 : stop publishers when data reloaded or deleted
* fixing issues whe disabling an already disabled publisher
* solved problem with time slider (issue #125)
* fix issue #126
* StatePublisher improved
* Contributors:  Davide Faconti

2.0.1 (2019-01-21)
------------------
* important bug fix. Removed offset in X axis of PlotXY
* fix minor visualization issue.
* Contributors: Davide Faconti

1.9.0 (2018-11-12)
------------------
* version bump
* Spurious DragLeave fixed? (The worst and most annoying bug of PlotJuggler)
* adjust font size in left panel
* CMAKE_INSTALL_PREFIX flag fix for non-ROS user (#114)
* adding improvements from @aeudes , issue #119
  1) Improved RemoveCurve dialog (colors and immediate replot)
  2) Fixed QMenu actions zoom horizontally and vertically
  3) Fix issue with panner and added Mouse Middle Button
* minor changes
* Merge branch 'master' of https://github.com/facontidavide/PlotJuggler
* speed up loading rosbags (5%-10%)
* custom qFileDialog to save the Layout
* minor changes
* Contributors: Davide Faconti, Mat&I

1.8.4 (2018-09-17)
------------------
* add tooltip
* fix issue #109
* CMakeLists.txt add mac homebrew qt5 install directory (#111)
* Merge pull request #107 from v-lopez/master
* Fix dragging/deletion of hidden items
* Contributors: Andrew Hundt, Davide Faconti, Victor Lopez

1.8.3 (2018-08-24)
------------------
* bug fix (crash when detaching a _point_marker)
* more informative error messages
* cleanups
* more compact view and larger dummyData
* Contributors: Davide Faconti

1.8.2 (2018-08-19)
------------------
* bug fix (crash from zombie PlotMatrix)
* Contributors: Davide Faconti

1.8.1 (2018-08-18)
------------------
* message moved back to the ROS plugin
* More informative dialog (issue #100)
* many improvements related to  FilteredTableListWidget, issue #103
* Contributors: Davide Faconti

1.8.0 (2018-08-17)
------------------
* fixing splash time
* minor update
* fix issue #49
* README and splashscreen updates
* Update ISSUE_TEMPLATE.md
* F10 enhancement
* preparing release 1.8.0
* (speedup) skip _completer->addToCompletionTree altogether unless Prefix mode is active
* avoid data copying when loading a datafile
* fix issue #103
* workaround for issue #100
* trying to fix problem with time offset durinh streaming
* removed enableStreaming from StreamingPlugins
* several useless replot() calls removed
* more conservative implementation of setTimeOffset
* optimization
* reduced a lot the amount of computation related to addCurve()
* bug fix
* Update .appveyor.yml
* bug fix (_main_tabbed_widget is already included in TabbedPlotWidget::instances())
* remove bug (crash at deleteDataOfSingleCurve)
* make PlotData non-copyable
* change in sthe state publisher API
* shared_ptr removed. To be tested
* WIP: changed the way data is shared
* added suggestion from issue #105
* skip empty dataMaps in importPlotDataMap() . Issue #105
* fix issue #102 (grey background)
* Contributors: Davide Faconti

1.7.3 (2018-08-12)
------------------
* enhancement discussed in #104 Can clear buffer while streaming is active
* adding enhancements 4 and 5 from issue #105
* fixed bug reported in  #105
* fix critical error
* fix issue #101
* Contributors: Davide Faconti

1.7.2 (2018-08-10)
------------------
* Update .travis.yml
* fixed potential thread safety problem
* trying to apply changes discussed in issue #96
* add transport hint
* make hyperlinks clickable by allowing to open external links (#95)
* Contributors: Davide Faconti, Romain Reignier

* Update .travis.yml
* fixed potential thread safety problem
* trying to apply changes discussed in issue #96
* add transport hint
* make hyperlinks clickable by allowing to open external links (#95)
* Contributors: Davide Faconti, Romain Reignier

1.7.1 (2018-07-22)
------------------
* catch exceptions
* fix resize of PlotData size. Reported in issue #94
* Contributors: Davide Faconti

1.7.0 (2018-07-19)
------------------
* fixing issue #93 (thread safety in XYPlot and streaming)
* fix issue #92
* bug fix
* Issue #88 (#90)
* Reorder header files to fix conflicts with boost and QT (#86)
* Contributors: Davide Faconti, Enrique Fernández Perdomo

1.6.2 (2018-05-19)
------------------
* fixing issue introduced in bec2c74195d74969f9c017b9b718faf9be6c1687
* Contributors: Davide Faconti

1.6.1 (2018-05-15)
------------------
* allow the buffer size to be edited
* qDebug removed
* fixing right mouse drag&drop
* Contributors: Davide Faconti

1.6.0 (2018-05-01)
------------------
* fixed the most annoying bug ever (erroneus DragLeave). issue #80
* fine tuning the widget spacing
* added feature #83
* fix issue #82
* remove redundant code in CMakeLists.txt
* Qwt updated and background color change during drag&drop
* Contributors: Davide Faconti

1.5.2 (2018-04-24)
------------------
* bug fix #78
* Fix typo (#76)
* Fix QmessageBox
* fixed issue reported in #68
* Contributors: Davide Faconti, Victor Lopez

1.5.1 (2018-02-14)
------------------
* Ignore not initialized timestamps (#75)
* added a warning as suggested in issue #75
* Housekeeping of publishers in StatePublisher
* improved layout and visibility in StatePublisher selector
* Fix issue #73: bad_cast exception
* Update README.md
* added more control over the published topics
* save ALL message instances
* CSV  plugin: accept CSV files with empty cells
* fix issue #72: std::round not supported by older compilers
* add a prefix to the field name if required
* Fix issue #69
* bug fix in onActionSaveLayout + indentation
* A small plugin creating a websocket server (#64)
* bug fixes
* Contributors: Davide Faconti, Philippe Gauthier

1.5.0 (2017-11-28)
------------------
* using AsyncSpinner as it ought to be
* fixing the mutex problem in streaming
* Contributors: Davide Faconti

1.4.2 (2017-11-20)
------------------
* bug fix in getIndexFromX that affected the vertical axis range calculation
* fix issue #61
* Contributors: Davide Faconti

1.4.1 (2017-11-19)
------------------
* fixed some issue with reloading rosbags and addressing issue #54
* adding improvement #55
* Contributors: Davide Faconti

1.4.0 (2017-11-14)
------------------
* added the ability to set max_array_size in the GUI
* Contributors: Davide Faconti

1.3.1 (2017-11-14)
------------------
* warnings added
* License updated
* Fix build failures on Archlinux (#57)
* Update README.md
* Contributors: Davide Faconti, Kartik Mohta

1.3.0 (2017-10-12)
------------------
* added xmlLoadState and xmlSaveState to ALL plugins
* works with newer ros_type_introspection
* speed up
* fix potential confision with #include
* minor fix in timeSlider
* Contributors: Davide Faconti

1.2.1 (2017-08-30)
------------------
* better limits for timeSlider
* fix a potential issue with ranges
* set explicitly the max vector size
* avoid wasting time doing tableWidget->sortByColumn
* bug fix
* prevent a nasty error during construction
* Update README.md
* added ros_type_introspection to travis
* Contributors: Davide Faconti

1.2.0 (2017-08-29)
------------------
* Ros introspection updated (`#52 <https://github.com/facontidavide/PlotJuggler/issues/52>`_)
* Potential fix for precision issue when adding time_offset
* Update snap/snapcraft.yaml
* Contributors: Davide Faconti, Kartik Mohta

1.1.3 (2017-07-11)
------------------
* fixed few issues with DataStreamROS
* Update README.md
* improvement `#43 <https://github.com/facontidavide/PlotJuggler/issues/43>`_. Use F10 to hide/show controls
* Contributors: Davide Faconti

1.1.2 (2017-06-28)
------------------
* bug-fix in DataLoadROS (multi-selection from layout)
* Merge branch 'master' of github.com:facontidavide/PlotJuggler
* minor change
* Update README.md
* Contributors: Davide Faconti

1.1.1 (2017-06-26)
------------------
* store rosbag::MessageInstance to replay data with the publisher
* avoid allocation
* minor optimizations
* bug fix: checkbox to use renaming rules was not detected correctly
* fix for very large rosbags
* Contributors: Davide Faconti

1.1.0 (2017-06-20)
------------------
* fixing bug `#47 <https://github.com/facontidavide/PlotJuggler/issues/47>`_
* Contributors: Davide Faconti

1.0.8 (2017-06-20)
------------------
* update to be compatible with ros_type_introspection 0.6
* setting uninitialized variable (thanks valgrind)
* improvement `#48 <https://github.com/facontidavide/PlotJuggler/issues/48>`_
* fix for issue `#46 <https://github.com/facontidavide/PlotJuggler/issues/46>`_ (load csv files)
* more intuitive ordering of strings. Based on PR `#45 <https://github.com/facontidavide/PlotJuggler/issues/45>`_. Fixes `#27 <https://github.com/facontidavide/PlotJuggler/issues/27>`_
* Correct the string being searched for to find the header stamp field (`#44 <https://github.com/facontidavide/PlotJuggler/issues/44>`_)
* Contributors: Davide Faconti, Kartik Mohta

1.0.7 (2017-05-12)
------------------
* the list of topics in the Dialog will be automatically updated
* bug fix
* fixed some issues with the installation
* Contributors: Davide Faconti

1.0.5 (2017-05-07)
------------------
* fixed an issue with ROS during destruction
* allow timestamp injection
* Create ISSUE_TEMPLATE.md
* Contributors: Davide Faconti

1.0.4 (2017-04-30)
------------------
* save/restore the selected topics in the layout file
* Contributors: Davide Faconti

1.0.3 (2017-04-28)
------------------
* fixed window management
* Contributors: Davide Faconti

1.0.2 (2017-04-26)
------------------
* set axis Y limit is undoable now
* added the command line option "buffer_size"
* filter xml extension for save layout
* added axis limits (Y)
* Contributors: Davide Faconti

1.0.1 (2017-04-24)
------------------
* documentation fix
* color widget simplified
* Update README.md
* default extension fixed in layout.xml
* Contributors: Davide Faconti, Eduardo Caceres

1.0.0 (2017-4-22)
-----------------
* Total awesomeness

0.18.0 (2017-04-21)
-------------------
* added visualization policy to the TimeTracker
* bug fix in RosoutPublisher
* added try-catch guard to third party plugins method invokation
* improving documentation
* multiple fixes
* shall periodically update the list of curves from the streamer
* make the API of plugins more consistent and future proof
* removed double replot during streaming (and framerate limited to 25)
* Contributors: Davide Faconti

0.17.0 (2017-04-02)
-------------------
* more renaming rules and samples
* feature request #31
* fix QFileDialog (save)
* fixing a nasty bug in save plot to file
* Add dummy returns to function that required it (#36)
* trying to fix some issues with the streamer time offset
* fixing a crash in the plugin
* saving more application settings with QSettings
* cleanups
* new plugin: rosout
* several bugs fixed
* removed unused plugin
* Update README.md
* cleanups
* added data samples
* move wais to filter the listWidget
* visualization improvements
* Contributors: Davide Faconti, v-lopez

0.16.0 (2017-03-22)
-------------------
* removed the normalization of time in ROS plugins
* relative time seems to work properly
* Contributors: Davide Faconti

0.15.3 (2017-03-22)
-------------------
* multiple fixes
* update related to backtrace
* backward-cpp added
* show coordinates when the left mouse is clicked (but not moved)
* Contributors: Davide Faconti

0.15.1 (2017-03-20)
-------------------
* adding some deadband to the zoomer
* fixed a bug related to tabs and new windows
* Contributors: Davide Faconti

0.15.0 (2017-03-17)
-------------------
* Multiple problems fixed with streaming interface nd XY plots
* Contributors: Davide Faconti

0.14.2 (2017-03-16)
-------------------
* improve CurveColorPick
* bugs fixed
* crash fixed
* Prevent compiler warning if compiling under ROS (#29)
* Contributors: Davide Faconti, Tim Clephas

0.14.1 (2017-03-15)
-------------------
* improved the time slider
* bug fixes
* Contributors: Davide Faconti

0.14.0 (2017-03-15)
-------------------
* improved usability
* adding XY plots (#26)
* improving plot magnifier
* changed key combination
* file extension of saved images fixed
* bug fixes
* adding the ability to delete curves
* Contributors: Davide Faconti

0.13.1 (2017-03-14)
-------------------
* bug fix
* Contributors: Davide Faconti

0.13.0 (2017-03-12)
-------------------
* default range X for empty plots
* better formatting
* improving 2nd column visualization
* Contributors: Davide Faconti

0.12.2 (2017-03-10)
-------------------
* Left curve list will display current value from vertical tracker
* new splashscreen phrases
* Temporarily disabling Qt5Svg
* Contributors: Davide Faconti


0.12.0 (2017-03-06)
-------------------
* Create .appveyor.yml
* added the ability to save rosbags from streaming
* bug fixes
* might fix compilation problem in recent cmake (3.x)
* improvement of the horizontal slider
* save plots to file
* qwt updated to trunk
* catch the rosbag exception
* Contributors: Davide Faconti

0.11.0 (2017-02-23)
-------------------
* should fix the reloading issue
* Update README.md
* minor fixes of the help_dialog layout
* Contributors: Davide Faconti, MarcelSoler

0.10.3 (2017-02-21)
-------------------
* adding help dialog
* minor bug fix
* Contributors: Davide Faconti

0.10.2 (2017-02-14)
-------------------
* critical bug fixed in ROS streaming
* Contributors: Davide Faconti

0.10.1 (2017-02-14)
-------------------
* adding more command line functionality
* BUG-FIX: bad resizing when a matrix row or column is deleted
* simplifying how random colors are managed
* more streaming buffer
* remember selected topics
* improvements and bug fixes
* Contributors: Davide Faconti

0.10.0 (2017-02-12)
-------------------
* auto loading of streamer based on saved layout
* refactoring of the ROS plugins 
* REFACTORING to allow future improvements of drag&drop
* trying to fix a compilation problem
* Update README.md
* FIX: menu bar will stay where it is supposed to.
* Contributors: Davide Faconti

0.9.1 (2017-02-09)
------------------
* FIX: avoid the use of catkin when using plain cmake
* IMPROVEMENT: exit option in the file menu
* IMPROVEMENT: reduce the number of steps to launch a streamer
* SPEEDUP: use a cache to avoid repeated creation of std::string
* better way to stop streaming and reload the plugins
* fixed a compilation problem on windows
* fixed a problem with resizing
* help menu with About added
* qDebug commented
* default to RelWithDebInfo
* Contributors: Davide Faconti

0.9.0 (2017-02-07)
------------------
* bug fixes
* QWT submodule removed
* removed boost dependency
* Contributors: Davide Faconti

* remove submodule
* Contributors: Davide Faconti

0.8.1 (2017-01-24)
------------------
* removing the old name "SuperPlotter"
* bug fix that affected data streaming
* this explicit dependency might be needed by bloom

0.8.0 (2017-01-23)
------------------
* First official beta of PJ
* Contributors: Arturo Martin-de-Nicolas, Davide Faconti, Kartik Mohta, Mikael Arguedas
