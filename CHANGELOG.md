# Changelog

## [0.8.15](https://github.com/pythonscad/pythonscad/compare/v0.8.14...v0.8.15) (2026-01-10)


### Bug Fixes

* **ci:** use version-agnostic Python detection for Windows builds ([#309](https://github.com/pythonscad/pythonscad/issues/309)) ([50febc2](https://github.com/pythonscad/pythonscad/commit/50febc2e37e7e3091e56eeb110497cbd5460ad18))

## [0.8.14](https://github.com/pythonscad/pythonscad/compare/v0.8.13...v0.8.14) (2026-01-10)


### Bug Fixes

* **scripts:** allow bash variable expansion in update-apt-repo.sh ([#304](https://github.com/pythonscad/pythonscad/issues/304)) ([3440af7](https://github.com/pythonscad/pythonscad/commit/3440af714a6909cb8871af36e88ef740ae2d56f0))

## [0.8.13](https://github.com/pythonscad/pythonscad/compare/v0.8.12...v0.8.13) (2026-01-10)


### Bug Fixes

* **actions:** correct strategy structure; matrix include output ([#301](https://github.com/pythonscad/pythonscad/issues/301)) ([d5105a0](https://github.com/pythonscad/pythonscad/commit/d5105a0c94dfc368a21440a2be80b1ee80e7d143))

## [0.8.12](https://github.com/pythonscad/pythonscad/compare/v0.8.11...v0.8.12) (2026-01-10)


### Bug Fixes

* **ci:** write matrix output to GITHUB_OUTPUT file ([#299](https://github.com/pythonscad/pythonscad/issues/299)) ([b4be64f](https://github.com/pythonscad/pythonscad/commit/b4be64fa8e8ddc3876219eac3550a23608a41b7e))

## [0.8.11](https://github.com/pythonscad/pythonscad/compare/v0.8.10...v0.8.11) (2026-01-10)


### Features

* **ci:** support multiple Debian/Ubuntu distributions in APT repository ([#297](https://github.com/pythonscad/pythonscad/issues/297)) ([9bb88fc](https://github.com/pythonscad/pythonscad/commit/9bb88fc146ffc26542ad1ae5b41553a4b59e2ed3))

## [0.8.10](https://github.com/pythonscad/pythonscad/compare/v0.8.9...v0.8.10) (2026-01-09)


### Bug Fixes

* **ci:** add GPG passphrase support for repository signing ([#295](https://github.com/pythonscad/pythonscad/issues/295)) ([2239a97](https://github.com/pythonscad/pythonscad/commit/2239a977b98e8ebd2db7ba8e5fb4d721fa535f18))

## [0.8.9](https://github.com/pythonscad/pythonscad/compare/v0.8.8...v0.8.9) (2026-01-09)


### Bug Fixes

* correct path for installing generated openscad.1 man page ([#293](https://github.com/pythonscad/pythonscad/issues/293)) ([8aa58bc](https://github.com/pythonscad/pythonscad/commit/8aa58bc05ed3109bf467f1983fd5564609878501))
* evalauting fn for path_extrude again ([#287](https://github.com/pythonscad/pythonscad/issues/287)) ([61a1f7a](https://github.com/pythonscad/pythonscad/commit/61a1f7a713ad538cb4147df33d9dae28a17cfced))

## [0.8.8](https://github.com/pythonscad/pythonscad/compare/v0.8.7...v0.8.8) (2026-01-09)


### Features

* **python:** make add_parameter return value and add feature toggle ([#290](https://github.com/pythonscad/pythonscad/issues/290)) ([97c4c11](https://github.com/pythonscad/pythonscad/commit/97c4c115a4b4f92a83014618cf4ef8f5c56239d0))


### Documentation

* add CLAUDE.md for project guidance and development workf ([#288](https://github.com/pythonscad/pythonscad/issues/288)) ([67f075e](https://github.com/pythonscad/pythonscad/commit/67f075e4a1f57fce6c5dfa0405b6bafe07120231))

## [0.8.7](https://github.com/pythonscad/pythonscad/compare/v0.8.6...v0.8.7) (2026-01-06)


### Bug Fixes

* add GPG pinentry-mode loopback for non-interactive signing ([#284](https://github.com/pythonscad/pythonscad/issues/284)) ([05f075c](https://github.com/pythonscad/pythonscad/commit/05f075c7be5c28e2c1cbc02d0b5a16c1f941f245))
* disable qt5 builds in Windows workflows ([#286](https://github.com/pythonscad/pythonscad/issues/286)) ([5f99971](https://github.com/pythonscad/pythonscad/commit/5f9997158d0694ec047607d3b6f3b4edea52507c))

## [0.8.6](https://github.com/pythonscad/pythonscad/compare/v0.8.5...v0.8.6) (2026-01-06)


### Bug Fixes

* improve YUM repository version handling and fix first-run issues ([#282](https://github.com/pythonscad/pythonscad/issues/282)) ([d7a1555](https://github.com/pythonscad/pythonscad/commit/d7a1555cd964d1b64d1b4fc034c8fce241865c92))

## [0.8.5](https://github.com/pythonscad/pythonscad/compare/v0.8.4...v0.8.5) (2026-01-06)


### Bug Fixes

* resolve race condition in APT and YUM repository workflows ([#280](https://github.com/pythonscad/pythonscad/issues/280)) ([9690849](https://github.com/pythonscad/pythonscad/commit/969084929a32b87025452949fe7a66697d779911))

## [0.8.4](https://github.com/pythonscad/pythonscad/compare/v0.8.3...v0.8.4) (2026-01-05)


### Bug Fixes

* resolve RPM and Debian packaging workflow failures ([#278](https://github.com/pythonscad/pythonscad/issues/278)) ([bf4620a](https://github.com/pythonscad/pythonscad/commit/bf4620a6d4320bb710471a3a6a37ee2d56c61c00))

## [0.8.3](https://github.com/pythonscad/pythonscad/compare/v0.8.2...v0.8.3) (2026-01-05)


### Features

* add RPM package build system ([#274](https://github.com/pythonscad/pythonscad/issues/274)) ([dd59cab](https://github.com/pythonscad/pythonscad/commit/dd59cab59dcfc3e3e231da7d086adc1b05ccaab1))


### Bug Fixes

* macOS app bundle not launching due to missing permissions and code signing ([#271](https://github.com/pythonscad/pythonscad/issues/271)) ([b0e16c1](https://github.com/pythonscad/pythonscad/commit/b0e16c1fa5b3f305a12a47197d5a453f35e1cba5))

## [0.8.2](https://github.com/pythonscad/pythonscad/compare/v0.8.1...v0.8.2) (2026-01-02)


### Features

* add Debian package build system ([#268](https://github.com/pythonscad/pythonscad/issues/268)) ([13d7e93](https://github.com/pythonscad/pythonscad/commit/13d7e9373f988bab86f0e9ff822e0d8d4a7f0598))

## [0.8.1](https://github.com/pythonscad/pythonscad/compare/v0.8.0...v0.8.1) (2025-12-31)


### Bug Fixes

* add include-component-in-tag setting to maintain tag compatibility ([#266](https://github.com/pythonscad/pythonscad/issues/266)) ([9f92d99](https://github.com/pythonscad/pythonscad/commit/9f92d998f1109381707e40513fc3c7a3a883b4e1))
* change release trigger from published to created ([#256](https://github.com/pythonscad/pythonscad/issues/256)) ([ea76cb4](https://github.com/pythonscad/pythonscad/commit/ea76cb454cdad1e1c725234ba9aa7b972c281e7d))
* correct VERSION.txt which previously wasn't updated by Release Please ([#262](https://github.com/pythonscad/pythonscad/issues/262)) ([c75f75a](https://github.com/pythonscad/pythonscad/commit/c75f75a03c4a9ee3885d8e4a03b6098b6a9cceaf))
* prevent macOS release builds from triggering on pull requests ([#258](https://github.com/pythonscad/pythonscad/issues/258)) ([a45e9f4](https://github.com/pythonscad/pythonscad/commit/a45e9f49be39b38a24aa760485881907d417d62b))
* use config files for release-please to enable VERSION.txt updates ([#260](https://github.com/pythonscad/pythonscad/issues/260)) ([1916712](https://github.com/pythonscad/pythonscad/commit/1916712f2651edce4dc12b4014e9e08362cd0440))

## [0.8.0](https://github.com/pythonscad/pythonscad/compare/v0.7.2...v0.8.0) (2025-12-30)


### Features

* add macOS build and upload workflow ([#252](https://github.com/pythonscad/pythonscad/issues/252)) ([ad6f085](https://github.com/pythonscad/pythonscad/commit/ad6f085c9cbc27a9f2ef0f32a3313350636140a3))

## [0.7.2](https://github.com/pythonscad/pythonscad/compare/v0.7.1...v0.7.2) (2025-12-28)


### Bug Fixes

* configure release-please to update VERSION.txt and improve version detection ([#250](https://github.com/pythonscad/pythonscad/issues/250)) ([0771c10](https://github.com/pythonscad/pythonscad/commit/0771c108419051a49304463a303043597ef990ef))

## [0.7.1](https://github.com/pythonscad/pythonscad/compare/v0.7.0...v0.7.1) (2025-12-28)


### Bug Fixes

* version detection, release artifact detection ([#248](https://github.com/pythonscad/pythonscad/issues/248)) ([abf087c](https://github.com/pythonscad/pythonscad/commit/abf087cad9bb0218a7dcdf837db538105e4b7c36))

## [0.7.0](https://github.com/pythonscad/pythonscad/compare/v0.6.0...v0.7.0) (2025-12-28)


### Features

* add Windows binary distribution packaging ([#245](https://github.com/pythonscad/pythonscad/issues/245)) ([fedc2ef](https://github.com/pythonscad/pythonscad/commit/fedc2ef811524c9e6d3c06e57644459852ed844c))

## 0.6.0 (2025-12-23)


### Features

* add AUTO option for Qt5Gamepad with Qt6 compatibility check ([51ee33a](https://github.com/pythonscad/pythonscad/commit/51ee33a57b2c0ae1286ebc25eb6532d17d0d6efa))
* add icons to language selection menu in status bar ([cb69bc5](https://github.com/pythonscad/pythonscad/commit/cb69bc5d370b40994caf4ae86dbe8727cb032b0a))
* add language selector to status bar with manual override support ([3658cc6](https://github.com/pythonscad/pythonscad/commit/3658cc6c5ebd459ac3b0e23a1cb8cd1b91b6062e))
* add language-specific icons to editor tabs ([6fe2904](https://github.com/pythonscad/pythonscad/commit/6fe29041d6f3b67a94f1a2375660cbb0bfaeaa88))
* add position property to Python object interface ([be103e9](https://github.com/pythonscad/pythonscad/commit/be103e9e24c9dfa8394c2a1113709fef9551f67c))
* add Python wrapper module with operator overloads and install support ([b9898c4](https://github.com/pythonscad/pythonscad/commit/b9898c42cd4f9bd8e421fa33709f326e0cfbcbcf))
* add PythonSCAD application icons for stable and nightly builds ([cb2dde3](https://github.com/pythonscad/pythonscad/commit/cb2dde3c121e4008dec893d3e8694dd639622ea8))
* enable calling OpenSCAD functions from PythonSCAD via osuse() ([#233](https://github.com/pythonscad/pythonscad/issues/233)) ([be5d2f4](https://github.com/pythonscad/pythonscad/commit/be5d2f473b6082521f941087c5bde071b468f5fc))
* implement three-state CMake options for dependency handling ([2d99b55](https://github.com/pythonscad/pythonscad/commit/2d99b55304672291bb3d771f07bc3c31c02675e9))
* **macos:** bundle Python framework for portable distribution ([8a4cdf8](https://github.com/pythonscad/pythonscad/commit/8a4cdf89cfcad8382678785e2120f27bde9fcbb3))
* modernize AppImage build script and add automated workflow ([#239](https://github.com/pythonscad/pythonscad/issues/239)) ([b763d01](https://github.com/pythonscad/pythonscad/commit/b763d01275c125db2789fed4f569bec983b26d0b))


### Bug Fixes

* 68 and [#69](https://github.com/pythonscad/pythonscad/issues/69) ([cb3e05b](https://github.com/pythonscad/pythonscad/commit/cb3e05b917631a5202bd0cf5b1128693b11d2c6b))
* Adapt GitHub issue template for PythonSCAD ([a522546](https://github.com/pythonscad/pythonscad/commit/a52254683bfeefa76821dc53a6ff0c86d3af26af))
* add conditional install for lib3mf-dev and libqt5gamepad5-dev ([055ec4d](https://github.com/pythonscad/pythonscad/commit/055ec4da6a7ae6785c18f3403115c88fa666f079))
* Add ellipsis to python venv menu entries  as user action is required after clicking them ([142166b](https://github.com/pythonscad/pythonscad/commit/142166ba5a38ebb113351d0ff579d31ce8f64a78))
* add libfuse2 dependency and explicitly specify desktop file ([#243](https://github.com/pythonscad/pythonscad/issues/243)) ([eb7a7f8](https://github.com/pythonscad/pythonscad/commit/eb7a7f890bfd3aba250d46fe988fb74fc2654986))
* add missing functions to Python stub ([#238](https://github.com/pythonscad/pythonscad/issues/238)) ([7d02813](https://github.com/pythonscad/pythonscad/commit/7d028134e3cae8cf45b88a3090597a98ea448ad3))
* automatically download linuxdeploy in case it is not found ([#241](https://github.com/pythonscad/pythonscad/issues/241)) ([b15fd4d](https://github.com/pythonscad/pythonscad/commit/b15fd4d4715bbb1ebb0789f9568a8958e205246a))
* cmake: Correctly handle REQUIRED in FindLib3MF.cmake ([#6288](https://github.com/pythonscad/pythonscad/issues/6288)) ([dac4638](https://github.com/pythonscad/pythonscad/commit/dac4638afdcfb6d3f8555211a647defd9b116442))
* **cmake:** define EXECUTABLE_NAME before winconsole subdirectory ([e75de6f](https://github.com/pythonscad/pythonscad/commit/e75de6f8a207527d85a95085cd489ad9054bf6b0))
* Corrected gvim startup problems and setting to use gvim in preferences ([7b43b79](https://github.com/pythonscad/pythonscad/commit/7b43b790d429e0999d2bc1b0d0fdb9d01df73dcf))
* Disable manifold tests when ENABLE_MANIFOLD is OFF ([#6237](https://github.com/pythonscad/pythonscad/issues/6237)) ([60fa74a](https://github.com/pythonscad/pythonscad/commit/60fa74addddd49822bae67c48e8c477ca97d0e68))
* do not run tests requiring lib3mf if OpenSCAD was compiled without lib3mf ([14b69d9](https://github.com/pythonscad/pythonscad/commit/14b69d9dc4df7ec3b688ae44a35c7a0da6349e25))
* Do not run tests requiring libfive if it is not enabled ([6a43d46](https://github.com/pythonscad/pythonscad/commit/6a43d46e57d2310a87ae7b3ed56d82247cd42eae))
* Enable experimental features per default. ([7925af2](https://github.com/pythonscad/pythonscad/commit/7925af2335c7acb2b544250e85b88d68ccd97263))
* Exclude .git files from macOS app bundle ([#6180](https://github.com/pythonscad/pythonscad/issues/6180)) ([4c27c74](https://github.com/pythonscad/pythonscad/commit/4c27c746dba0027bdceb43cad02aa2fa2c8f43cf)), closes [#6179](https://github.com/pythonscad/pythonscad/issues/6179)
* guard manifold-specific code with ENABLE_MANIFOLD preprocessor directives ([10cd9c2](https://github.com/pythonscad/pythonscad/commit/10cd9c297b096495acd4436131a17842df9feb95))
* improve CMake test configuration logic and conditional compilation ([#6221](https://github.com/pythonscad/pythonscad/issues/6221)) ([5042c64](https://github.com/pythonscad/pythonscad/commit/5042c64dc9aa113e3e96b1d61d0dd595007b4ca7))
* improve size calculation by directly computing geometry bounds ([4febd3a](https://github.com/pythonscad/pythonscad/commit/4febd3a4c28acaf6cf5958096b065b00ccf176d9))
* invalid show datatype caused crash ([#226](https://github.com/pythonscad/pythonscad/issues/226)) ([9b11745](https://github.com/pythonscad/pythonscad/commit/9b117450b94e2d1f7bbf2d4617a4a4b4a7b3ba07))
* **macos:** add EIGEN_DONT_ALIGN to prevent memory corruption ([0ed2f6c](https://github.com/pythonscad/pythonscad/commit/0ed2f6cedce348c89d548d267882da94dea644cf))
* **macos:** create pythonscad-python symlink inside app bundle ([7e5a471](https://github.com/pythonscad/pythonscad/commit/7e5a4718d4ec4ec844eb64eb0141d0bf3af126cc))
* **macos:** use correct Python3 version variables in install_name_tool ([ff10f1f](https://github.com/pythonscad/pythonscad/commit/ff10f1fdee5271c0443d3404015cf271e89dbe69))
* make uni-get-dependencies.sh compatible with sh again. ([e948b2b](https://github.com/pythonscad/pythonscad/commit/e948b2be3221100aa928df8047909a2d3da80116))
* Name changes from OpenSCAD to PythonSCAD in user visible stringsvin CMakeLists.txt ([99e140f](https://github.com/pythonscad/pythonscad/commit/99e140f8cd81e6098d08128555fa98a7f011b990))
* Prevent auto-preview on startup when auto-reload is disabled ([#6137](https://github.com/pythonscad/pythonscad/issues/6137)) ([3a36cb1](https://github.com/pythonscad/pythonscad/commit/3a36cb171692212b7b4037bd6c588127d42bab2b))
* python module works again ([#229](https://github.com/pythonscad/pythonscad/issues/229)) ([4ed26e2](https://github.com/pythonscad/pythonscad/commit/4ed26e2d70a3111380dcc8476c88df47434cf11c))
* Re-add libcurl to uni-get-dependencies.sh ([84fa9b7](https://github.com/pythonscad/pythonscad/commit/84fa9b7d5d1894508be32348343f8fd90a7af429))
* refactor: use 'auto' for num_rings calculation in SphereNode::createGeometry ([d66917d](https://github.com/pythonscad/pythonscad/commit/d66917d45a92afa31579ee60978845544427359a))
* Remove generated files from git ([16b2fb4](https://github.com/pythonscad/pythonscad/commit/16b2fb46bf753a355142e4ae84d1609db338cb02))
* Remove generated files from git ([916a7b0](https://github.com/pythonscad/pythonscad/commit/916a7b02bc5a739db98789023d33d55375e6eb23))
* remove orphaned PROPERTIES DISABLED TRUE from CMakeLists.txt ([9f2abc5](https://github.com/pythonscad/pythonscad/commit/9f2abc5601ca111895b10939695c9077b7dc8d70))
* replace assert by LOG, no crash anymore ([a63801a](https://github.com/pythonscad/pythonscad/commit/a63801aa477cbd348d6d958ae7f8125d17576ede))
* replace assert by LOG, no crash anymore ([94e5d66](https://github.com/pythonscad/pythonscad/commit/94e5d66dc6265aaf2ee6ed545acde9a85b856918))
* Temporarily disable Linux build tests without marix as they need more extensive fixing first ([26a86fd](https://github.com/pythonscad/pythonscad/commit/26a86fd1bf8e813700cc9e141993cc2a85b2b0b3))
* trust unsaved Python files to prevent segfault after canceled Save-As ([4924770](https://github.com/pythonscad/pythonscad/commit/49247706efabda2d4c3284868881bb7e1fc57c6e))
* unwrap OpenSCADWrapper objects in function arguments ([56f15c2](https://github.com/pythonscad/pythonscad/commit/56f15c215a4b70abf9509b99b8886aece8af15a8))
* use downloaded linuxdeploy when system version not available ([#242](https://github.com/pythonscad/pythonscad/issues/242)) ([14cecda](https://github.com/pythonscad/pythonscad/commit/14cecda172ec2c2ad73adcb4596c6be67ed0a7a1))
* use Python3 FindPackage module instead of Python for consistency ([f5a0590](https://github.com/pythonscad/pythonscad/commit/f5a0590d99711fe38f3d573a267098aec9647c82))
* **windows:** add bsdiff dependency for libpython patch script ([f5ed3af](https://github.com/pythonscad/pythonscad/commit/f5ed3af78e987c184b715a8d9bacaf1b6719a040))
* **windows:** add Clipper2 include directory for OpenSCADPy on Windows ([731ab23](https://github.com/pythonscad/pythonscad/commit/731ab23709245606f1a02741a9f5dfec0ad5919e))
* **windows:** avoid 'Argument list too long' error in ar command ([ea709b6](https://github.com/pythonscad/pythonscad/commit/ea709b68c7a714a07f95b37179602239ef632bf0))
* **windows:** change winconsole to launch pythonscad.exe instead of openscad.exe ([ffdd506](https://github.com/pythonscad/pythonscad/commit/ffdd5062f16ede38d7a92f7ff1e13b3d6a191e1d))
* **windows:** copy pythonscad-python.exe instead of creating symlink ([5d7d898](https://github.com/pythonscad/pythonscad/commit/5d7d898e453cc0e1a7a8869142c09ca8c4e5c334))
* **windows:** install bsdiff using pacman instead of pacboy ([a9ca6f5](https://github.com/pythonscad/pythonscad/commit/a9ca6f5fa0305167b8de49cd2fad4afa01bc52a7))
* **windows:** remove redundant tar extraction step ([b49ee3a](https://github.com/pythonscad/pythonscad/commit/b49ee3a36a166f22f916ea3ccad86a060f28feed))
* **windows:** replace unzip/unzstd with bsdtar for MSYS2 compatibility ([9a7a255](https://github.com/pythonscad/pythonscad/commit/9a7a255d0238fda805f6109223165eeccd933f2b))
* **windows:** use bsdiff4 from pip instead of system bsdiff ([d8b581f](https://github.com/pythonscad/pythonscad/commit/d8b581ffb86c2f3c49ab08cde0ee4dc39eaf6ed9))
* **windows:** use CURL_LIBRARIES from find_package on Windows ([872f933](https://github.com/pythonscad/pythonscad/commit/872f933fe883adf9c81722fbf62659cad1999343))
* **windows:** wrap custom commands in bash for MSYS2 compatibility ([b7d50ec](https://github.com/pythonscad/pythonscad/commit/b7d50ecf3ee1e59dea4aeaa57f767e9fa56b5e2d))


### Miscellaneous Chores

* set version to 0.6.0 ([#236](https://github.com/pythonscad/pythonscad/issues/236)) ([7410b97](https://github.com/pythonscad/pythonscad/commit/7410b97ce8f718ea0460a2ae1eee844626e190ba))
