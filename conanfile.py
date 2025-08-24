from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.scm import Git
from conan.tools.files import update_conandata, chdir
import shutil, os, glob

class ReviewApp(ConanFile):
    name = "review_app"
    version = "0.1.2"
    package_type = "application"
    
    settings = "os", "compiler", "build_type", "arch"
    
    options = {
        "with_tests": [True, False],
        "use_system_qt": [True, False],
        "use_system_boost": [True, False],
        "use_system_jsoncpp": [True, False],
        "use_system_openssl": [True, False],

        # 将构建开关定义为 options(代替cmake的操作)
        "with_postgresql": [True, False],
        "with_mysql": [True, False],
        "with_ctl": [True, False],
        "with_orm": [True, False],
        "with_yaml_config": [True, False],
        "with_brotli": [True, False],
    }
    
    default_options = {
        "with_tests": True,
        "use_system_qt": False,
        "use_system_boost": False,
        "use_system_jsoncpp": False,
        "use_system_openssl": False,
        "qt/*:qtsvg": True,
        "qt/*:shared": True,
        "qt/*:qtquickcontrols": True,
        "qt/*:openssl": False,
        "libcurl/*:shared": True,
        "zlib/*:shared": False,
        "trantor/*:openssl": False,

        # 将构建开关定义为 options(代替cmake的操作)
        "with_postgresql": False,
        "with_mysql": False,
        "with_ctl": False,
        "with_orm": False,
        "with_yaml_config": False,
        "with_brotli": False,
    }

    def configure(self):
        self.output.warning(f"=====================def configure(self):")
        self.settings.compiler.cppstd = "14"
        self.options["crashpad"].with_handler = True

        if not self.options.use_system_qt:
            self.options["qt"].shared = True 
        if not self.options.use_system_boost:    
            self.options["boost"].shared = True
        
        # Ubuntu18.04和22.04的api移除问题
        if self.settings.os == "Linux":
            self.options["openssl"].shared = True

    def generate(self):
        self.output.warning(f"=====================def generate(self):")
        """Generate build configuration"""
        tc = CMakeToolchain(self)
        self._configure_system_paths(tc)
        if self.settings.os == "Linux":
            tc.variables["CMAKE_PREFIX_PATH"] = "/home/aoi/Qt5.15.2/Qt/5.15.2/gcc_64"
            tc.variables["CMAKE_CXX_FLAGS"] = "-Wno-error=deprecated-declarations"

        tc.generate()
        CMakeDeps(self).generate()

    def build(self):
        self.output.warning(f"=====================def build(self):")
        """实现cmake的构建和编译逻辑"""
        cmake = CMake(self)
        # configure 现在不需要传递变量了，因为它们已经在 toolchain 中定义好了
        cmake.configure()
        cmake.build()

    def package(self):
        self.output.warning(f"=====================def package(self):")

        """实现cmake --install逻辑"""
        cmake = CMake(self)
        # 设置下载路径(reject!!!)
        # self.package_folder="./.build/install"
        build_dir = os.path.abspath(".")
        
        # install路径的设置取决于layout的定义
        if self.settings.os == "Windows":
            install_dir = os.path.join(self.build_folder, "..", "install")
        else:
            install_dir = os.path.join(self.build_folder, "..", "..", "install")

        self.run(f'cmake --install "{self.build_folder}" --prefix "{install_dir}" --component bin')

        """实现打包逻辑"""
        if not self.options.use_system_qt:
            if self.settings.os == "Windows":
                windeployqt = os.path.join(self.dependencies["qt"].package_folder,
                                        "bin", "windeployqt.exe")
                exe_path = os.path.join(install_dir, "bin", "app.exe")
            elif self.settings.os == "Linux":
                linuxdeployqt = os.path.join(self.dependencies["qt"].package_folder,
                                        "bin", "linuxdeployqt")
                exe_path = os.path.join(install_dir, "bin", "app")
        else:
            if self.settings.os == "Windows":
                windeployqt = self.conf.get("user.build:system_qt_prefix") + "/lib/windeployqt"
                exe_path = os.path.join(install_dir, "bin", "app.exe")
            elif self.settings.os == "Linux":
                linuxdeployqt = self.conf.get("user.build:system_qt_prefix") + "/lib/linuxdeployqt"
                exe_path = os.path.join(install_dir, "bin", "app")

        current_dir = os.path.dirname(os.path.abspath(__file__))
        self.output.warning("current_dir: %s" % current_dir)
        res_qml = os.path.abspath(os.path.join(current_dir, "res", "qml"))

        # self.output.warning("windeployqt dir: %s" % windeployqt)
        # self.output.warning("linuxdeployqt dir: %s" % linuxdeployqt)
        self.output.warning("exe_path: %s" % exe_path)

        if self.settings.os == "Windows":
            if os.path.exists(windeployqt) and os.path.exists(exe_path):
                with chdir(self, os.path.dirname(exe_path)):
                    command = [
                        windeployqt,
                        "--qmldir",
                        res_qml,
                        "--release",
                        exe_path
                    ]
                self.run(" ".join(command))
            else:
                error_str = f"windeployqt ({windeployqt}) 或可执行文件 ({exe_path}) 未找到"
                self.output.error(error_str)
                raise Exception(error_str)
        else:
            if os.path.exists(linuxdeployqt) and os.path.exists(exe_path):
                with chdir(self, os.path.dirname(exe_path)):
                    command = [
                        linuxdeployqt,
                        exe_path,
                        "-qmldir=" + res_qml
                    ]
                self.run(" ".join(command))
            else:
                error_str = f"linuxdeployqt ({linuxdeployqt}) 或可执行文件 ({exe_path}) 未找到"
                self.output.error(error_str)
                raise Exception(error_str)


        # =====================copy===============================
        # Windows下拷贝到一起 Linux下分目录拷贝(取决于不同系统下deployqt的执行特点)
        des_bin_path =  os.path.join(install_dir, "bin")
        des_lib_path =  os.path.join(install_dir, "lib")
        
        # For Linux, also define executable directory for direct library copying (like WCDB)
        if self.settings.os == "Linux":
            des_exe_path = des_bin_path  # Same as bin path for Linux

        os.makedirs(des_bin_path, exist_ok=True)
        os.makedirs(des_lib_path, exist_ok=True)

        # copy zlib or libcurl to install/bin
        # 判断zlib是否为shared
        if self.dependencies["zlib"].options.shared:
            if self.settings.os == "Windows":
                zlib_bin = os.path.join(self.dependencies["zlib"].package_folder, "bin", "zlib1.dll")
                shutil.copy2(zlib_bin, des_bin_path)
            elif self.settings.os == "Linux":
                zlib_bin = os.path.join(self.dependencies["zlib"].package_folder, "lib", "libz.so")
                shutil.copy2(zlib_bin, des_lib_path)
            self.output.warning("zlib_bin dir: %s" % zlib_bin)
        else:
            self.output.info("zlib is not built as shared library, skip copying zlib shared library.")

        # 判断libcurl是否为shared
        if self.dependencies["libcurl"].options.shared:
            if self.settings.os == "Windows":
                libcurl_bin = os.path.join(self.dependencies["libcurl"].package_folder, "bin", "libcurl.dll")
                shutil.copy2(libcurl_bin, des_bin_path)
            elif self.settings.os == "Linux":
                libcurl_bin = os.path.join(self.dependencies["libcurl"].package_folder, "lib", "libcurl.so")
                shutil.copy2(libcurl_bin, des_lib_path)
            self.output.warning("libcurl_bin dir: %s" % libcurl_bin)
        else:
            self.output.info("libcurl is not built as shared library, skip copying libcurl shared library.")


        # copy Qt.labs.platform to install
        if self.settings.os == "Windows":
            qt_labs_dir = self.find_labs_folder(self.dependencies["qt"].package_folder)
        elif self.settings.os == "Linux":
            qt_labs_dir = self.find_labs_folder(self.conf.get("user.build:system_qt_prefix"))
            
        self.output.warning("qt_labs_dir: %s" % qt_labs_dir)
        des_qtlab_path = os.path.join(des_bin_path, "Qt", "labs", "platform")
        if os.path.exists(qt_labs_dir):
            shutil.copytree(qt_labs_dir, des_qtlab_path, dirs_exist_ok=True)
            self.output.warning("Copied Qt/labs from {} to {}".format(qt_labs_dir, des_qtlab_path))
        else:
            self.output.warn("Qt/labs folder not found in Qt package!")

        # copy boost_json, boost_filesystem
        if self.options.use_system_boost:
            boost_pkg = self.conf.get("user.build:system_boost_prefix")
            lib_dir = os.path.join(boost_pkg, "lib")
        else:
            boost_pkg = self.dependencies["boost"].package_folder
            if self.settings.os == "Windows":
                lib_dir = os.path.join(boost_pkg, "bin")
            else:
                lib_dir = os.path.join(boost_pkg, "lib")

        if self.settings.os == "Windows":
            dll_files = ["boost_json.dll", "boost_filesystem.dll"]
            if os.path.exists(lib_dir):
                for dll in dll_files:
                    src = os.path.join(lib_dir, dll)
                    if os.path.exists(src):
                        shutil.copy2(src, des_bin_path)
                        self.output.warning(f"Copied {src} to {des_bin_path}")
                    else:
                        self.output.warning(f"{src} does not exist in Boost bin path!")
            else:
                self.output.warning(f"Boost bin dir {lib_dir} does not exist!")
        else:
            so_files = ["libboost_json.so", "libboost_filesystem.so"]
            for fname in os.listdir(lib_dir):
                if fname.startswith("libboost_json.so") or fname.startswith("libboost_filesystem.so"):
                    src = os.path.join(lib_dir, fname)
                    shutil.copy2(src, des_lib_path)
                    self.output.warning(f"Copied {src} to {des_lib_path}")

        
        # copy libssl-1_1-x64, libcrypto-1_1-x64(只有Linux系统下需要设置成shared模式)
        if not self.options.use_system_openssl and self.settings.os == "Linux":
            # 使用 conan openssl && 动态依赖(需要拷贝)
            openssl_pkg = self.dependencies["openssl"].package_folder

            if self.settings.os == "Windows":
                bin_dir = os.path.join(openssl_pkg, "bin")
                dll_files = ["libssl-1_1-x64.dll", "libcrypto-1_1-x64.dll"]
                self.output.warning(f"Copying openssl dlls from {bin_dir} to {des_bin_path}")
                if os.path.exists(bin_dir):
                    for dll in dll_files:
                        src = os.path.join(bin_dir, dll)
                        if os.path.exists(src):
                            shutil.copy2(src, des_bin_path)
                            self.output.warning(f"Copied {src} to {des_bin_path}")
                        else:
                            self.output.warning(f"{src} does not exist (conan openssl)!")
                else:
                    self.output.warning(f"Conan openssl bin dir {bin_dir} does not exist!")
            else:
                lib_dir = os.path.join(openssl_pkg, "lib")
                so_files = ["libssl.so.1.1", "libcrypto.so.1.1"]
                self.output.warning(f"Copying openssl so from {lib_dir} to {des_lib_path}")
                if os.path.exists(lib_dir):
                    os.makedirs(des_lib_path, exist_ok=True)
                    # 支持实际名称和软连接
                    for fname in os.listdir(lib_dir):
                        for so in so_files:
                            if fname.startswith(so):
                                src = os.path.join(lib_dir, fname)
                                shutil.copy2(src, des_lib_path)
                                self.output.warning(f"Copied {src} to {des_lib_path}")
                else:
                    self.output.warning(f"Conan openssl lib dir {lib_dir} does not exist!")

        # copy crashpad_handler.exe
        crashpad_pkg = self.dependencies["crashpad"].package_folder
        
        # 判断平台和文件名
        if self.settings.os == "Windows":
            fname = "crashpad_handler.exe"
        else:
            fname = "crashpad_handler"
        # 文件名所在路径可能不确定
        potential_paths = [
            os.path.join(crashpad_pkg, fname),
            os.path.join(crashpad_pkg, "bin", fname),
            os.path.join(crashpad_pkg, "libexec", fname),
            os.path.join(crashpad_pkg, "bin", "Release", fname),
            os.path.join(crashpad_pkg, "bin", "Debug", fname),
        ]
        src_file = None
        for path in potential_paths:
            if os.path.isfile(path):
                src_file = path
                break
        if not src_file:
            raise FileNotFoundError(f"{fname} not found in {crashpad_pkg}")
        shutil.copy2(src_file, des_bin_path)
        self.output.info(f"Copied {src_file} -> {des_bin_path}")

    def find_labs_folder(self, root_dir):
        for dirpath, dirnames, filenames in os.walk(root_dir):
            if "platform" in dirnames:
                return os.path.join(dirpath, "platform")
        return None

    def layout(self):
        self.output.warning(f"=====================def layout(self):")
        cmake_layout(self)

        multi_config_generators = ["Visual Studio", "Xcode", "Ninja Multi-Config"]
        generator = str(self.conf.get("tools.cmake.cmaketoolchain:generator", ""))
        is_multi_config = any(gen in generator for gen in multi_config_generators)

        if self.settings.os == "Windows" or is_multi_config:
            self.folders.build = "build"
            self.folders.generators = "generators"
        else:
            self.folders.build = os.path.join("build", str(self.settings.build_type))

    def requirements(self):
        self.output.warning(f"=====================def requirements(self):")
        
        self._add_ui_dependencies()
        self._add_core_dependencies()
        self._add_database_dependencies()
        self._add_network_dependencies()
        self._add_utility_dependencies()
        
        if self.options.with_tests:
            self.test_requires("gtest/1.14.0")

    def _add_ui_dependencies(self):
        """UI related dependencies"""
        qt_prefix = self.conf.get("user.build:system_qt_prefix")
        if not self.options.use_system_qt or not qt_prefix:
            self.requires("qt/5.15.11", options={"with_sqlite3": False})

    def _add_core_dependencies(self):
        """Core dependencies"""
        self.requires("crashpad/cci.20220219")
        if not self.options.use_system_boost:
            self.requires("boost/1.83.0")
            
        jsoncpp_prefix = self.conf.get("user.build:system_jsoncpp_prefix")
        if not self.options.use_system_jsoncpp or not jsoncpp_prefix:
            self.requires("jsoncpp/1.9.5")

    def _add_network_dependencies(self):
        """Network related dependencies"""
        if not self.options.use_system_openssl:
            self.requires("openssl/1.1.1t")
            
        self.requires("libcurl/8.12.1")
        self.requires("c-ares/1.25.0")

    def _add_database_dependencies(self):
        """Database related dependencies"""
        self.requires("sqlite3/3.45.0", options={"build_executable": False})
        self.requires("mariadb-connector-c/3.4.3", transitive_headers=True, transitive_libs=True)
        self.requires("sqlpp11/0.64")

    def _add_utility_dependencies(self):
        """Utility library dependencies"""
        self.requires("spdlog/1.14.1")
        self.requires("zlib/1.2.13")
        self.requires("pugixml/1.15")
        self.requires("nlohmann_json/3.11.2")


    def _configure_system_paths(self, tc):
        """Configure system dependency paths"""
        prefix_paths = []
        
        if self.options.use_system_qt:
            qt_prefix = self.conf.get("user.build:system_qt_prefix")
            if qt_prefix:
                prefix_paths.append(qt_prefix)

        if self.options.use_system_boost:
            boost_prefix = self.conf.get("user.build:system_boost_prefix")
            if boost_prefix:
                prefix_paths.append(boost_prefix)

        if self.options.use_system_jsoncpp:
            jsoncpp_prefix = self.conf.get("user.build:system_jsoncpp_prefix")
            if jsoncpp_prefix:
                prefix_paths.append(jsoncpp_prefix)

        if self.options.use_system_openssl:
            openssl_prefix = self.conf.get("user.build:system_openssl_prefix")
            if openssl_prefix:
                prefix_paths.append(openssl_prefix)

        if prefix_paths:
            tc.cache_variables["CMAKE_PREFIX_PATH"] = ";".join(prefix_paths)

    def export(self):
        self.output.warning(f"=====================def export(self):")
        """Export Git information"""
        try:
            git = Git(self, self.recipe_folder)
            scm_url = git.get_remote_url()
            scm_commit = git.get_commit()
            update_conandata(self, {"sources": {"commit": scm_commit, "url": scm_url}})
        except Exception:
            pass

    def source(self):
        self.output.warning(f"=====================def source(self):")
        """Get source code"""
        if "sources" not in self.conan_data:
            return
            
        try:
            git = Git(self)
            sources = self.conan_data["sources"]
            git.clone(url=sources["url"], target=".")
            git.checkout(commit=sources["commit"])
            git.run("submodule update --init --recursive")
        except Exception as e:
            self.output.error(f"Source retrieval failed: {e}")
            raise

    def package_info(self):
        # Application—no public headers or libs
        self.cpp_info.libdirs = []
        self.cpp_info.bindirs = ["bin"]