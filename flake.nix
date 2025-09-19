{
  description = "A flake for building and running PlotJuggler";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
          config.qt5.enable = true;
        };

        data-tamer-src = pkgs.fetchzip {
          url = "https://github.com/PickNikRobotics/data_tamer/archive/refs/tags/1.0.3.zip";
          sha256 = "sha256-hGfoU6oK7vh39TRCBTYnlqEsvGLWCsLVRBXh3RDrmnY=";
        };

        plotjuggler-pkg = pkgs.qt5.mkDerivation {
          pname = "plotjuggler";
          version = "3.10.11";

          src = ./.;
          patches = [ ./nix/arrow.patch ];

          postPatch = ''
            substituteInPlace cmake/find_or_download_data_tamer.cmake \
              --replace "URL" "SOURCE_DIR" \
              --replace "https://github.com/PickNikRobotics/data_tamer/archive/refs/tags/1.0.3.zip" "${data-tamer-src}"

            rm cmake/find_or_download_fmt.cmake
            rm cmake/find_or_download_fastcdr.cmake
            rm cmake/find_or_download_zstd.cmake

            substituteInPlace CMakeLists.txt \
              --replace "include(cmake/find_or_download_fmt.cmake)" "find_package(fmt REQUIRED)" \
              --replace "find_or_download_fmt()" ""

            substituteInPlace CMakeLists.txt \
              --replace "include(cmake/find_or_download_fastcdr.cmake)" "find_package(fastcdr REQUIRED)" \
              --replace "find_or_download_fastcdr()" ""
            find . -name "CMakeLists.txt" -exec sed -i 's/fastcdr::fastcdr/fastcdr/g' {} +

            cat > plotjuggler_plugins/DataLoadMCAP/CMakeLists.txt << 'EOF'
            cmake_minimum_required(VERSION 3.5)

            if(mcap_vendor_FOUND)
              set(CMAKE_AUTOUIC ON)
              set(CMAKE_AUTORCC ON)
              set(CMAKE_AUTOMOC ON)

              project(DataLoadMCAP)

              add_library(mcap INTERFACE)
              find_package(zstd REQUIRED)
              find_package(lz4 REQUIRED)

              add_library(dataload_mcap MODULE dataload_mcap.cpp)

            target_link_libraries(
              dataload_mcap PUBLIC Qt5::Widgets Qt5::Xml Qt5::Concurrent plotjuggler_base mcap
                                  zstd lz4)

            if(WIN32 AND MSVC)
              target_link_options(dataload_mcap PRIVATE /ignore:4217)
            endif()

            install(TARGETS dataload_mcap DESTINATION ''${PJ_PLUGIN_INSTALL_DIRECTORY})
            endif()
            EOF
          '';

          cmakeFlags = [
            "-DPLJ_USE_SYSTEM_LUA=ON"
            "-DPLJ_USE_SYSTEM_NLOHMANN_JSON=ON"
          ];


          nativeBuildInputs = [ pkgs.cmake pkgs.qt5.wrapQtAppsHook ];

          buildInputs = [
            pkgs.qt5.full
            pkgs.qt5.qtsvg
            pkgs.qt5.qtimageformats
            pkgs.qt5.qtdeclarative
            pkgs.zeromq
            pkgs.sqlite
            pkgs.lua
            pkgs.nlohmann_json
            pkgs.fmt
            pkgs.fastcdr
            pkgs.lz4
            pkgs.zstd
            pkgs.mosquitto
            pkgs.protobuf
            pkgs.xorg.libX11
            pkgs.xorg.libxcb
            pkgs.xorg.xcbutil
            pkgs.xorg.xcbutilkeysyms
            pkgs.arrow-cpp
          ];
          dontWrapQtApps = true;

          meta = with pkgs.lib; {
            description = "A tool to plot streaming data, fast and easy";
            homepage = "https://github.com/facontidavide/PlotJuggler";
            license = licenses.mpl20;
            platforms = platforms.linux ++ platforms.darwin;
          };
        };

      in
      {
        packages.default = plotjuggler-pkg;
        packages.plotjuggler = plotjuggler-pkg;

        apps.default = {
          type = "app";
          program = "${plotjuggler-pkg}/bin/plotjuggler";
        };
        apps.plotjuggler = self.apps.${system}.default;

        devShells.default = pkgs.mkShell {
          packages = [
            pkgs.cmake
            pkgs.qt5.full
            pkgs.qt5.qtsvg
            pkgs.qt5.qtimageformats
            pkgs.qt5.qtdeclarative
            pkgs.arrow-cpp
            pkgs.zeromq
            pkgs.sqlite
            pkgs.lua
            pkgs.nlohmann_json
            pkgs.fmt
            pkgs.fastcdr
            pkgs.lz4
            pkgs.zstd
            pkgs.mosquitto
            pkgs.protobuf
            pkgs.codespell
            pkgs.xorg.libX11
            pkgs.xorg.libxcb
            pkgs.xorg.xcbutil
            pkgs.xorg.xcbutilkeysyms
          ];
        };
      }
    );
}
