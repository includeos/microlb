from conans import ConanFile, python_requires, CMake

conan_tools = python_requires("conan-tools/[>=1.0.0]@includeos/stable")

class MicroLBConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "microlb"
    version = conan_tools.git_get_semver()
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake', 'virtualenv'
    url = "http://www.includeos.org/"
    no_copy_source=True
    scm = {
        "type" : "git",
        "url" : "auto",
        "subfolder": ".",
        "revision" : "auto"
    }

    options={
        "liveupdate":[True,False],
        "tls": [True,False]
    }
    default_options={
        "liveupdate":True,
        "tls":True
    }

    def package_id(self):
        self.info.requires.major_mode()

    def requirements(self):
        self.requires("includeos/[>=0.14.0,include_prerelease=True]@includeos/latest")
        if (self.options.liveupdate):
            self.requires("liveupdate/[>=0.14.0,include_prerelease=True]@includeos/latest")

    def _arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))

    def _cmake_configure(self):
        cmake = CMake(self)
        cmake.definitions['ARCH']=self._arch()
        cmake.definitions['LIVEUPDATE']=self.options.liveupdate
        cmake.definitions['TLS']=self.options.tls
        cmake.configure(source_folder=self.source_folder)
        return cmake

    def build(self):
        cmake = self._cmake_configure()
        cmake.build()

    def package(self):
        cmake = self._cmake_configure()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs=['microlb']

    def deploy(self):
        self.copy("*",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")
