from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class MyLibConan(ConanFile):
	name = "libmcstatus"
	version = "0.0.0"
	license = "MIT"
	url = "https://github.com/BrainStone/libmcstatus"
	description = "C++ implementation of https://github.com/py-mine/mcstatus. Not necessarily a perfect 1:1 port but with roughly the same functionality"
	settings = "os", "compiler", "build_type", "arch", "compiler.cppstd"
	generators = "CMakeDeps", "CMakeToolchain"
	exports_sources = "src/*", "include/*", "CMakeLists.txt"
	options = {"shared": [True, False], "fPIC": [True, False]}
	default_options = {"shared": False, "fPIC": True}

	def requirements(self):
		self.requires("boost/[>=1.70.0]")
		self.test_requires("gtest/[>=1.10.0]")

	def config_options(self):
		if self.settings.os == "Windows":
			del self.options.fPIC

	def configure(self):
		if self.options.shared:
			self.options.rm_safe("fPIC")

	def layout(self):
		cmake_layout(self)

	def build(self):
		cmake = CMake(self)
		cmake.configure()
		cmake.build()

	def package(self):
		cmake = CMake(self)
		cmake.install()

	def package_info(self):
		self.cpp_info.libs = ["mcstatus"]
