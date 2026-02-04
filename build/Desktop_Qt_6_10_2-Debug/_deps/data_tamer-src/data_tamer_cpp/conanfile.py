# -*- coding: utf-8 -*-
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class DataTamerConan(ConanFile):
    name = "data_tamer"
    version = "0.9.4"
    package_type = "library"
    url = "https://github.com/facontidavide/data_tamer"
    license = "MIT"
    description = "library for data profiling"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "tests": [True, False],
        "examples": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "tests": True,
        "examples": True
    }
    exports_sources = (
        "3rdparty/*",
        "include/*",
        "src/*",
        "examples/*",
        "tests/*",
        "CMakeLists.txt",
        "data_tamerConfig.cmake.in"
    )

    def requirements(self):

        self.requires("mcap/1.3.0")
        if self.options.tests:
            self.requires("gtest/1.14.0")

    def build_requirements(self):
        self.tool_requires("cmake/3.26.4")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(
            {
                "DATA_TAMER_BUILD_TESTS": self.options.tests,
                "DATA_TAMER_BUILD_EXAMPLES": self.options.examples
            }
        )
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["data_tamer"]
