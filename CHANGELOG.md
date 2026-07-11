# Changelog

## [1.1.0](https://github.com/pythonscad/pythonscad/compare/v1.0.0...v1.1.0) (2026-07-11)


### Features

* **appdata:** replace inherited OpenSCAD releases with PythonSCAD history ([#796](https://github.com/pythonscad/pythonscad/issues/796)) ([ca27d4e](https://github.com/pythonscad/pythonscad/commit/ca27d4ef452b277872c2dcfb3680ec3b661da554))
* **gui:** add form-based bug report template and Help menu action ([#751](https://github.com/pythonscad/pythonscad/issues/751)) ([2dd38b8](https://github.com/pythonscad/pythonscad/commit/2dd38b873d98e40e3103bbcaa7d6b964eadc0db0))
* **gui:** AIDock preferences ([#6836](https://github.com/pythonscad/pythonscad/issues/6836)) ([9aed34f](https://github.com/pythonscad/pythonscad/commit/9aed34f1cbc49585d006c25be04e7ad07162792b))
* **mesh:** add ability for mesh to output color ([01b2d6b](https://github.com/pythonscad/pythonscad/commit/01b2d6b51f0c86ccd9fc0b8b8307cf7b07205809))
* **packaging:** bundle IPython 9.x with MSYS2 psutil vendoring ([#746](https://github.com/pythonscad/pythonscad/issues/746)) ([8c10144](https://github.com/pythonscad/pythonscad/commit/8c1014491069184283c33c48078dad4b9df4e239))
* **pip:** add binary wheels for PyPI distribution ([#744](https://github.com/pythonscad/pythonscad/issues/744)) ([a0b9b5f](https://github.com/pythonscad/pythonscad/commit/a0b9b5f2335c7a9e61ea7dbc8aa5bad41b67120b))
* **python:** add rounded_cube helper with r or d rounding ([#738](https://github.com/pythonscad/pythonscad/issues/738)) ([1d38779](https://github.com/pythonscad/pythonscad/commit/1d387794f559f73f3c8ef57c58e4e3d40b9922a5))
* **python:** support single-file multitool 3MF export ([#831](https://github.com/pythonscad/pythonscad/issues/831)) ([141d8b6](https://github.com/pythonscad/pythonscad/commit/141d8b64496d5faf3dc84485a1b9673a6fb72d84))
* **release:** add automated release smoke tooling ([#870](https://github.com/pythonscad/pythonscad/issues/870)) ([040f4c2](https://github.com/pythonscad/pythonscad/commit/040f4c20089877aef47fe5fe721dd6199df863c5))
* **release:** add release-announcement for v0.16.0 ([2e92fe4](https://github.com/pythonscad/pythonscad/commit/2e92fe4e207866d3dc9856aca36de3e19f693d7b))
* **release:** add release-announcement for v0.17.0 ([a2fcab4](https://github.com/pythonscad/pythonscad/commit/a2fcab48365da7f35ad3493d8e90f9854d281e21))
* **release:** add release-announcement for v0.18.0 ([dbc7c96](https://github.com/pythonscad/pythonscad/commit/dbc7c969037ca02a1d6d751c1545a3de452f4f30))
* **release:** add release-announcement for v0.20.0 ([809e21c](https://github.com/pythonscad/pythonscad/commit/809e21c9e33b7dcf97bf398a5c64bbf848682049))
* **release:** add release-announcement for v1.1.0 ([b6e645c](https://github.com/pythonscad/pythonscad/commit/b6e645ca4b6a4b213852b6ca0352f8dd75ec4094))
* **release:** rename release announcement for v1.0. to have proper "v" prefix ([5c6dd68](https://github.com/pythonscad/pythonscad/commit/5c6dd68f2a756efa721614b70db5a2159eec5abe))
* **wasm:** add CPython 3.14 WebAssembly browser support ([#697](https://github.com/pythonscad/pythonscad/issues/697)) ([6920f8f](https://github.com/pythonscad/pythonscad/commit/6920f8fcde8500497fc05dfd81dfa5441ee32d68))
* **wasm:** in-browser playground with notebook UI and website integration ([#800](https://github.com/pythonscad/pythonscad/issues/800)) ([46658ad](https://github.com/pythonscad/pythonscad/commit/46658ad022588084925fe8e4f4fc0e659062fc78))


### Bug Fixes

* **appimage:** bundle GMP runtime library ([#858](https://github.com/pythonscad/pythonscad/issues/858)) ([4e5df24](https://github.com/pythonscad/pythonscad/commit/4e5df240a2fb6533831c9f1f8a6770bbec285ac3))
* **appimage:** disable GTK platform theme for Qt6 ([#857](https://github.com/pythonscad/pythonscad/issues/857)) ([6444058](https://github.com/pythonscad/pythonscad/commit/644405891090a2c6e01de114c1a6e3ca4485d402))
* **appimage:** isolate GIO modules from host runtime ([#856](https://github.com/pythonscad/pythonscad/issues/856)) ([29fc4f3](https://github.com/pythonscad/pythonscad/commit/29fc4f3af3c79f3a0975909231650fd72f5bac54))
* **assert:** issue warning instead of hard assert ([#810](https://github.com/pythonscad/pythonscad/issues/810)) ([f056785](https://github.com/pythonscad/pythonscad/commit/f0567859f4d6ea48274435ee7beb67395693c363))
* **build:** add OpenSSL package build dependencies ([#846](https://github.com/pythonscad/pythonscad/issues/846)) ([ed77c6f](https://github.com/pythonscad/pythonscad/commit/ed77c6fb3316aa8b0d1b8a6d30e4a99bc9f8d351)), closes [#845](https://github.com/pythonscad/pythonscad/issues/845)
* **building:** enable USE_QT6 option in building documentation ([#833](https://github.com/pythonscad/pythonscad/issues/833)) ([409a2bd](https://github.com/pythonscad/pythonscad/commit/409a2bd4ac99a6931897e80e642e0dfad0f54b72))
* **build:** restore configure_file writability check with clearer diagnostics ([#756](https://github.com/pythonscad/pythonscad/issues/756)) ([7d2cbf7](https://github.com/pythonscad/pythonscad/commit/7d2cbf732ca197d71c97b285ee5dd626e43fc6cb))
* **chore:** add missed FrepNode ([#865](https://github.com/pythonscad/pythonscad/issues/865)) ([f3e0c0d](https://github.com/pythonscad/pythonscad/commit/f3e0c0d991418d066d345b1350180ec05b7ea09e))
* **chore:** compiles again ([f39bd7e](https://github.com/pythonscad/pythonscad/commit/f39bd7e87db77d75a4c9d3b1a58dc637dcf8421d))
* **chore:** customizer delevers recent values ([#720](https://github.com/pythonscad/pythonscad/issues/720)) ([8d8e09f](https://github.com/pythonscad/pythonscad/commit/8d8e09f8295849b23f416b7a93ed5d05d72693af))
* **chore:** fixed meaning of stroke parameter ([980f545](https://github.com/pythonscad/pythonscad/commit/980f545e68f1917739e4990f0d58cd2684aebd70))
* **chore:** get negation correct ([ed8b7a2](https://github.com/pythonscad/pythonscad/commit/ed8b7a2bfc044b40311f593737d4b336f3d97d66))
* **chore:** merge ([a35eea8](https://github.com/pythonscad/pythonscad/commit/a35eea847b9a088dadd3c3b43bf57387746fd67d))
* **chore:** merge it ([035ee5d](https://github.com/pythonscad/pythonscad/commit/035ee5d2001a20339892b3146e9620c991f98ebf))
* **chore:** merging ([adeef5c](https://github.com/pythonscad/pythonscad/commit/adeef5c55125070895592b848d57e431a7a6f499))
* **chore:** removed comment ([321c518](https://github.com/pythonscad/pythonscad/commit/321c51835a1d56a35b8cbccd4e58b8ddede06bd7))
* **ci:** exempt Dependabot commits from commitlint body rule ([#762](https://github.com/pythonscad/pythonscad/issues/762)) ([5184596](https://github.com/pythonscad/pythonscad/commit/5184596208e0400773a2154859cdfe291ed19e58)), closes [#757](https://github.com/pythonscad/pythonscad/issues/757) [#758](https://github.com/pythonscad/pythonscad/issues/758)
* **ci:** guard release-please PR output ([#838](https://github.com/pythonscad/pythonscad/issues/838)) ([d237eee](https://github.com/pythonscad/pythonscad/commit/d237eee0744fb14c7cf780df9eb1ebeddb6be36c))
* **ci:** install OpenSSL dev package for Debian builds ([#848](https://github.com/pythonscad/pythonscad/issues/848)) ([a89d0fb](https://github.com/pythonscad/pythonscad/commit/a89d0fb1443904f0dd05cd5ffc6a1ec4a554259f)), closes [#847](https://github.com/pythonscad/pythonscad/issues/847)
* **ci:** install pip-licenses before bundle step on Windows and macOS ([#792](https://github.com/pythonscad/pythonscad/issues/792)) ([ea74ade](https://github.com/pythonscad/pythonscad/commit/ea74aded9575828d7f6347f208de66e212af1d9e))
* **ci:** resolve zizmor 1.26 high-severity workflow findings ([#787](https://github.com/pythonscad/pythonscad/issues/787)) ([43ed78c](https://github.com/pythonscad/pythonscad/commit/43ed78cadcd395fd720c43df65e4f1a7fa8ff152))
* **ci:** restore clang-tidy cache speed after cancel-in-progress rollout ([#786](https://github.com/pythonscad/pythonscad/issues/786)) ([dabd672](https://github.com/pythonscad/pythonscad/commit/dabd672f4c1df477808e3adc68d203cc9e9978ca))
* **ci:** sign bundled Python native extensions for macOS notarization ([#775](https://github.com/pythonscad/pythonscad/issues/775)) ([c963d95](https://github.com/pythonscad/pythonscad/commit/c963d95616f26af6362aee3f78d87feb404d05e3))
* **ci:** trust openscad Homebrew tap before installing dependencies ([#732](https://github.com/pythonscad/pythonscad/issues/732)) ([40345f3](https://github.com/pythonscad/pythonscad/commit/40345f34b35209ec170cc7906850a07df02a92b1))
* clean implementation with correct formatting ([7173204](https://github.com/pythonscad/pythonscad/commit/71732048141374775fbb794d078ac6142f9b5914))
* clean up compiler warnings in openscad_gui and Tree.h ([#765](https://github.com/pythonscad/pythonscad/issues/765)) ([85573f2](https://github.com/pythonscad/pythonscad/commit/85573f2c4657671786cd558257f15aa2180bc1c3))
* **customizer:** defer text field preview until commit ([#830](https://github.com/pythonscad/pythonscad/issues/830)) ([e4e91d2](https://github.com/pythonscad/pythonscad/commit/e4e91d205db4d576a6f41fb72d13281b2afab280))
* **deps:** update dependency three to v0.185.1 ([#826](https://github.com/pythonscad/pythonscad/issues/826)) ([1d827bd](https://github.com/pythonscad/pythonscad/commit/1d827bd30dd463c9a38642874254ba5357082ce7))
* **gui:** align desktop file ID with installed .desktop entry ([#739](https://github.com/pythonscad/pythonscad/issues/739)) ([a82c63b](https://github.com/pythonscad/pythonscad/commit/a82c63b0cdb2bacaab1ffef19737f616a280ed90))
* **gui:** launch gvim reliably as external editor ([#806](https://github.com/pythonscad/pythonscad/issues/806)) ([2caf6ff](https://github.com/pythonscad/pythonscad/commit/2caf6ffb779cc7738d8010eb21a34a91e9c30e94))
* **locale:** regenerate POT/PO files for v1.1.0 ([#876](https://github.com/pythonscad/pythonscad/issues/876)) ([661475d](https://github.com/pythonscad/pythonscad/commit/661475d6e5a5501430de856cd66c004e657e4709))
* **python:** accept 1-3 element vectors in resize() ([#749](https://github.com/pythonscad/pythonscad/issues/749)) ([026fe94](https://github.com/pythonscad/pythonscad/commit/026fe949a601de00b9e378298bd1e3da2e1c58cb))
* **python:** detect pythonscad-python argv[0] to enter Python mode ([#794](https://github.com/pythonscad/pythonscad/issues/794)) ([84c54d7](https://github.com/pythonscad/pythonscad/commit/84c54d71593a6bae754ce9971d95ffff8ab07bfc))
* **python:** make nimport() work on Windows and report errors ([#729](https://github.com/pythonscad/pythonscad/issues/729)) ([8aab5d1](https://github.com/pythonscad/pythonscad/commit/8aab5d1bc83918dbfce6143ae5d17736ed5953c2))
* **python:** preserve venv base executable ([#873](https://github.com/pythonscad/pythonscad/issues/873)) ([889f5d2](https://github.com/pythonscad/pythonscad/commit/889f5d26436cbaba3e326cd67596d5a51f7013b7))
* **python:** prevent segfault when cloning nodes for children() ([#785](https://github.com/pythonscad/pythonscad/issues/785)) ([a7bf067](https://github.com/pythonscad/pythonscad/commit/a7bf067dd1dca678192063a8e3d2ebd2f3b3286a))
* **python:** refresh script import path on reinit ([#843](https://github.com/pythonscad/pythonscad/issues/843)) ([eda3d76](https://github.com/pythonscad/pythonscad/commit/eda3d768d122425925addb95ac8dfc9d101e2177))
* **python:** use semantic version helpers ([#835](https://github.com/pythonscad/pythonscad/issues/835)) ([2d7f9e9](https://github.com/pythonscad/pythonscad/commit/2d7f9e945ac12f66b9df0671a6f35204678974b9))
* **release:** keep Linux package smoke artifacts bind-mountable ([#875](https://github.com/pythonscad/pythonscad/issues/875)) ([522df82](https://github.com/pythonscad/pythonscad/commit/522df827bdd3fbd5134cedf24a9885b91144bca6))
* **release:** smoke test local Windows artifacts ([#872](https://github.com/pythonscad/pythonscad/issues/872)) ([63039e0](https://github.com/pythonscad/pythonscad/commit/63039e09645e3e8474627882affdb5780781ce04))
* report unsafe fillet topology ([#809](https://github.com/pythonscad/pythonscad/issues/809)) ([4e79fe4](https://github.com/pythonscad/pythonscad/commit/4e79fe419b369c89f0a70044ed093a133ff17220))
* use Cmd instead of Ctrl in hyperlink tooltip on macOS ([4936618](https://github.com/pythonscad/pythonscad/commit/49366181fb428d8b7085738702542b293c5bd812))
* use Cmd instead of Ctrl in tooltip on macOS ([5ab9d3b](https://github.com/pythonscad/pythonscad/commit/5ab9d3bec239c46f298008217a208dd156426e6d)), closes [#4725](https://github.com/pythonscad/pythonscad/issues/4725)
* use Cmd instead of Ctrl in tooltip on macOS ([a474df6](https://github.com/pythonscad/pythonscad/commit/a474df698a00af87c28dc53015dacc1a52c80fec)), closes [#4725](https://github.com/pythonscad/pythonscad/issues/4725)
* use existing labels in issue templates ([#808](https://github.com/pythonscad/pythonscad/issues/808)) ([b2ca812](https://github.com/pythonscad/pythonscad/commit/b2ca812ff71b5b03da0bd62089437062942467c5))
* **wasm:** compile svg target with -fPIC for web build ([#798](https://github.com/pythonscad/pythonscad/issues/798)) ([7c0aaec](https://github.com/pythonscad/pythonscad/commit/7c0aaecbd61d7c18070340608559522474881fb3))
* **wasm:** Emscripten 6.0 sysroot and Python 3.14 browser builds ([#777](https://github.com/pythonscad/pythonscad/issues/777)) ([17a0403](https://github.com/pythonscad/pythonscad/commit/17a0403f3e00800e1ac63c85661d00c89e380774))
* **windows:** add packaged Python runtime search paths ([#864](https://github.com/pythonscad/pythonscad/issues/864)) ([2ba937d](https://github.com/pythonscad/pythonscad/commit/2ba937dbdebc243741423be49f6e95ab2f450e6d))
* **windows:** initialize embedded Python sysconfig state ([#866](https://github.com/pythonscad/pythonscad/issues/866)) ([bfde23d](https://github.com/pythonscad/pythonscad/commit/bfde23d03b960104580bc29283613b7c72a74c92))
* **windows:** patch libpython import lib on the fly and silence CMP0207 ([#764](https://github.com/pythonscad/pythonscad/issues/764)) ([0c9e2ca](https://github.com/pythonscad/pythonscad/commit/0c9e2cab0a1c89e8a6dc24e592ded6144a969645))
* **windows:** resolve Python stdlib path and executable for --repl ([#851](https://github.com/pythonscad/pythonscad/issues/851)) ([56dd11e](https://github.com/pythonscad/pythonscad/commit/56dd11eb67ef1fccd45f1aa6bd5c28291d8755f1))
* **windows:** suppress false-positive "Uninstall failed" on upgrade ([#860](https://github.com/pythonscad/pythonscad/issues/860)) ([19853e4](https://github.com/pythonscad/pythonscad/commit/19853e4c4de679402604fd28083434436b483aa4))


### Documentation

* **python:** document single-file multitool 3MF export ([#832](https://github.com/pythonscad/pythonscad/issues/832)) ([7c3ec8d](https://github.com/pythonscad/pythonscad/commit/7c3ec8da795423b9c4ab78b9a3490614a15e8216))
* **release:** address review feedback ([ad1623c](https://github.com/pythonscad/pythonscad/commit/ad1623c931613a0e0faa584d4a7c3be4956dfc19))
* revamp homepage and download UX (closes [#741](https://github.com/pythonscad/pythonscad/issues/741)) ([#752](https://github.com/pythonscad/pythonscad/issues/752)) ([f8cc3c0](https://github.com/pythonscad/pythonscad/commit/f8cc3c03df0dbe4bac6a431e668f32bf83af3f9c))

## [1.0.0](https://github.com/pythonscad/pythonscad/compare/v0.20.0...v1.0.0) (2026-06-02)


### ⚠ BREAKING CHANGES

* **python:** "from openscad import *" now resolves to the pure-Python overlay added in the next commit, not to this C extension directly. Python code that imported the C extension by its old name must switch to "import _openscad".

### Features

* **chore:** add pythonscad logo source ([#710](https://github.com/pythonscad/pythonscad/issues/710)) ([bd017d2](https://github.com/pythonscad/pythonscad/commit/bd017d25e3d965856592f042c969c3df6a966ec3))
* **ci:** add MSIX packaging as reusable composite action ([#638](https://github.com/pythonscad/pythonscad/issues/638)) ([422b28a](https://github.com/pythonscad/pythonscad/commit/422b28a15f9070e547df1696a3b434584f096521))
* **distributions:** add support for Fedora 44 and Ubuntu 26.04 LTS ([#624](https://github.com/pythonscad/pythonscad/issues/624)) ([5a78c77](https://github.com/pythonscad/pythonscad/commit/5a78c77f73d0f662884c312ddd41f2d670b23a4c))
* **gui:** display absolute filepath in the title bar ([#576](https://github.com/pythonscad/pythonscad/issues/576)) ([41c6552](https://github.com/pythonscad/pythonscad/commit/41c6552d2a658a7be824864623a7f4e3a23d1a6a))
* **gui:** replace Python trust dialog with inline trust bar ([#667](https://github.com/pythonscad/pythonscad/issues/667)) ([1382dd7](https://github.com/pythonscad/pythonscad/commit/1382dd7515c91cc18593e0b5f1ca5281a90ba1e0))
* **install:** per-user Windows installer with on-demand UAC elevation ([#696](https://github.com/pythonscad/pythonscad/issues/696)) ([20a4f18](https://github.com/pythonscad/pythonscad/commit/20a4f186df15c53e939385522c05bb2faf453291))
* native vector operations for addition, subtraction, scaling, dot and cross product ([#588](https://github.com/pythonscad/pythonscad/issues/588)) ([267c753](https://github.com/pythonscad/pythonscad/commit/267c7535f6daccbf76a3499c8442738983d7ae71))
* **polyline:** polylines work with difference and intersection now ([#572](https://github.com/pythonscad/pythonscad/issues/572)) ([6f0ee76](https://github.com/pythonscad/pythonscad/commit/6f0ee76f29ca90151cfecea36b1dcd9306bbc51a))
* **python:** add MultiToolExporter helper to pythonscad module ([#585](https://github.com/pythonscad/pythonscad/issues/585)) ([e15e0cf](https://github.com/pythonscad/pythonscad/commit/e15e0cfd5542572c9fef52720e5b04d602d718db))
* **python:** expose root ColorNode RGBA as solid property `c` ([#562](https://github.com/pythonscad/pythonscad/issues/562)) ([0da296a](https://github.com/pythonscad/pythonscad/commit/0da296abcae4d4762226de22c95e66d7e15ee2c8))
* **python:** introduce _openscad / openscad / pythonscad three-module layout ([#579](https://github.com/pythonscad/pythonscad/issues/579)) ([542c89e](https://github.com/pythonscad/pythonscad/commit/542c89eaac1e9bdcac3115e3a9f1370076be1234))
* **python:** launch real IPython via --ipython, add --repl for legacy prompt ([#600](https://github.com/pythonscad/pythonscad/issues/600)) ([96d8fa5](https://github.com/pythonscad/pythonscad/commit/96d8fa5b1736be5411b16a74bffaa3251ef014ad))
* **release:** Windows code signing via Certum SimplySign (local script) ([#703](https://github.com/pythonscad/pythonscad/issues/703)) ([838a916](https://github.com/pythonscad/pythonscad/commit/838a9165658bec62bb059de4f2bb0a4e2f65908a))


### Bug Fixes

* **3mf:** scope per-mesh material IDs to append_polyset (lib3mf v1) ([#592](https://github.com/pythonscad/pythonscad/issues/592)) ([823a572](https://github.com/pythonscad/pythonscad/commit/823a572abc12e29e5f8bf72d1e487e32f30f9067)), closes [#591](https://github.com/pythonscad/pythonscad/issues/591)
* **appstream:** align &lt;id&gt; and desktop filename to reverse-DNS convention ([#684](https://github.com/pythonscad/pythonscad/issues/684)) ([f9b3cf1](https://github.com/pythonscad/pythonscad/commit/f9b3cf1e0f0050588f3226f0e903cc859fd43375))
* **build:** add hidapi and libspnav dependencies across all platforms ([#700](https://github.com/pythonscad/pythonscad/issues/700)) ([6a202a9](https://github.com/pythonscad/pythonscad/commit/6a202a98644e6cd0bdb2749f800b23455e374a1e))
* **build:** guard sha256_digest call for nettle 4.0 API change ([#630](https://github.com/pythonscad/pythonscad/issues/630)) ([6695570](https://github.com/pythonscad/pythonscad/commit/66955702f8bbc13967acb8b8bb1a8a180004a6f4)), closes [#629](https://github.com/pythonscad/pythonscad/issues/629)
* **chore:** accept defines on command line also during animation ([#699](https://github.com/pythonscad/pythonscad/issues/699)) ([78bb534](https://github.com/pythonscad/pythonscad/commit/78bb534abf2982a114e3db5e963f79fe08bc11c2))
* **chore:** improved GUI performance ([be2c045](https://github.com/pythonscad/pythonscad/commit/be2c045b3e63f46b323f640fdfbd17bcd0a96ba5))
* **chore:** make it compilable ([#605](https://github.com/pythonscad/pythonscad/issues/605)) ([0534499](https://github.com/pythonscad/pythonscad/commit/0534499aa0de6d0e4774da2a564cdb1b1d9571fa))
* **chore:** rebrand remaining OpenSCAD references to PythonSCAD ([#676](https://github.com/pythonscad/pythonscad/issues/676)) ([1514bcf](https://github.com/pythonscad/pythonscad/commit/1514bcf167699e6bdeedc08ed2683248ff872eff))
* **chore:** replace more openscad links ([#672](https://github.com/pythonscad/pythonscad/issues/672)) ([c276831](https://github.com/pythonscad/pythonscad/commit/c2768315e29b1316527b37e24189e3fb5289ad33))
* **chore:** update Homepage URL ([#669](https://github.com/pythonscad/pythonscad/issues/669)) ([63288f2](https://github.com/pythonscad/pythonscad/commit/63288f2044a98a63759199e5fc81d36bc637a849))
* **ci:** bind-mount output dir in arm64 Docker builds for deb and rpm ([#639](https://github.com/pythonscad/pythonscad/issues/639)) ([86eafc4](https://github.com/pythonscad/pythonscad/commit/86eafc483640968628fcce6054f18125ef6822cf)), closes [#615](https://github.com/pythonscad/pythonscad/issues/615)
* **ci:** digest-pin all container images in supported-distributions.json ([#663](https://github.com/pythonscad/pythonscad/issues/663)) ([#664](https://github.com/pythonscad/pythonscad/issues/664)) ([8722c7b](https://github.com/pythonscad/pythonscad/commit/8722c7b226bc6b3894d7910dc3a9f75070bda30e))
* **ci:** harden workflows — permissions, template-injection, shellcheck, hash-pin ([#613](https://github.com/pythonscad/pythonscad/issues/613)) ([#662](https://github.com/pythonscad/pythonscad/issues/662)) ([3021bdc](https://github.com/pythonscad/pythonscad/commit/3021bdc6d809ec601b369b0ba007683b48e5990a))
* **ci:** use CERTUM_CERTIFICATE_CN secret for MSIX publisher DN ([#704](https://github.com/pythonscad/pythonscad/issues/704)) ([f4c3d15](https://github.com/pythonscad/pythonscad/commit/f4c3d15ac96c323dca56ce93790300582f36ad75))
* **ci:** use PAT when promoting release so PyPI workflow gets triggered ([#582](https://github.com/pythonscad/pythonscad/issues/582)) ([05c1508](https://github.com/pythonscad/pythonscad/commit/05c1508a67bd29d4746a57a0a6e44e5d838bf291))
* **csg:** handle missing CONCAT case in CSGTreeEvaluator switch ([#653](https://github.com/pythonscad/pythonscad/issues/653)) ([2814b29](https://github.com/pythonscad/pythonscad/commit/2814b2901527081947b698a909c1a25545de3ba9))
* **debug:** updated pythonscad logo on mac ([#655](https://github.com/pythonscad/pythonscad/issues/655)) ([4954ff0](https://github.com/pythonscad/pythonscad/commit/4954ff0b554932fd1c0857410a6d805f46be4660))
* **docs:** proof of project ownership for certum.pl to get proper Windows SmartScreen certificates ([#692](https://github.com/pythonscad/pythonscad/issues/692)) ([e9142c0](https://github.com/pythonscad/pythonscad/commit/e9142c0817ec4816628884eaf2f0fb1f90c00a8d))
* **gui:** defer editorContentChanged signal until activeEditor is set ([#694](https://github.com/pythonscad/pythonscad/issues/694)) ([0331363](https://github.com/pythonscad/pythonscad/commit/03313630b0bf8bcea942b800298fd16e35169373)), closes [#690](https://github.com/pythonscad/pythonscad/issues/690)
* **gui:** don't mark windows as quitting on session-manager checkpoint ([#581](https://github.com/pythonscad/pythonscad/issues/581)) ([7da0c96](https://github.com/pythonscad/pythonscad/commit/7da0c967265a2433ed3f014dc9329ad8970b8668)), closes [#580](https://github.com/pythonscad/pythonscad/issues/580)
* **gui:** fix startup crash and double file dialog from welcome screen ([#695](https://github.com/pythonscad/pythonscad/issues/695)) ([46619c3](https://github.com/pythonscad/pythonscad/commit/46619c32ff27c028f3bf3feef56f339eb049fb6f))
* **gui:** improve color contrast for Python branding text in dark mode ([#641](https://github.com/pythonscad/pythonscad/issues/641)) ([d42ec18](https://github.com/pythonscad/pythonscad/commit/d42ec18a29a8d32ea88fde3e7b1dbe22aa73176d))
* **gui:** offer exit or discard when session file is too new ([#708](https://github.com/pythonscad/pythonscad/issues/708)) ([285e3ab](https://github.com/pythonscad/pythonscad/commit/285e3ab3461793ea856abbad3c8efb64324ddf76))
* **gui:** persist per-window geometry in session restore ([#627](https://github.com/pythonscad/pythonscad/issues/627)) ([7de0ca9](https://github.com/pythonscad/pythonscad/commit/7de0ca9f69039ffdd8b7985166b6087a40704b96))
* **gui:** remove automatic parameter refresh timer ([#654](https://github.com/pythonscad/pythonscad/issues/654)) ([d78ff0f](https://github.com/pythonscad/pythonscad/commit/d78ff0f8880dc45ae55f98f87904b029ba564940))
* **gui:** restore New button in welcome dialog when Python is disabled ([#637](https://github.com/pythonscad/pythonscad/issues/637)) ([62b3e61](https://github.com/pythonscad/pythonscad/commit/62b3e613550de3907b0cf53200fd8bbf900da692)), closes [#616](https://github.com/pythonscad/pythonscad/issues/616)
* **gui:** route all compile/parse output to console widget ([#643](https://github.com/pythonscad/pythonscad/issues/643)) ([00664ee](https://github.com/pythonscad/pythonscad/commit/00664eeaf438fbf7fe7814e5fb6cce7e255c0142))
* **gui:** shorten IPC socket name to fit macOS sun_path limit ([#640](https://github.com/pythonscad/pythonscad/issues/640)) ([f48b4f1](https://github.com/pythonscad/pythonscad/commit/f48b4f1a359b04a203f780b1ef8fbe5ad59556df)), closes [#623](https://github.com/pythonscad/pythonscad/issues/623)
* **html:** update AboutDialog text for PythonSCAD ([#661](https://github.com/pythonscad/pythonscad/issues/661)) ([35490b0](https://github.com/pythonscad/pythonscad/commit/35490b04c8d27a56bba0ea3e36498509fff33616))
* improve error handling and string comparisons (closes [#648](https://github.com/pythonscad/pythonscad/issues/648)) ([#652](https://github.com/pythonscad/pythonscad/issues/652)) ([10c0ae6](https://github.com/pythonscad/pythonscad/commit/10c0ae633db17820fdf7bc8b1948e62b058fa3bc))
* **install:** align AppStream metainfo filename with &lt;id&gt; suffix ([#693](https://github.com/pythonscad/pythonscad/issues/693)) ([91bde99](https://github.com/pythonscad/pythonscad/commit/91bde999e28f070de9bebfad45abf23094c19812)), closes [#685](https://github.com/pythonscad/pythonscad/issues/685)
* **io,python:** fix import/export deprecation message and raise file errors ([#656](https://github.com/pythonscad/pythonscad/issues/656)) ([555ff92](https://github.com/pythonscad/pythonscad/commit/555ff92054e7d57f7eccb61b610617b3a41a5c50))
* **laser:** improved link function ([ecaa0dc](https://github.com/pythonscad/pythonscad/commit/ecaa0dcb636c73c5cce353a41705da7376529056))
* **locale:** regenerate POT/PO files to fix stale OpenSCAD branding msgid ([#686](https://github.com/pythonscad/pythonscad/issues/686)) ([27a1816](https://github.com/pythonscad/pythonscad/commit/27a1816671266d86d6910596f5d4a9dc1757d35f))
* **macos:** correct dock icon and fix theme mismatch in preferences ([#670](https://github.com/pythonscad/pythonscad/issues/670), [#671](https://github.com/pythonscad/pythonscad/issues/671)) ([#675](https://github.com/pythonscad/pythonscad/issues/675)) ([b89ce51](https://github.com/pythonscad/pythonscad/commit/b89ce51639b40f47a67488a9f7fc50400ac08928))
* **macos:** replace deprecated kUTTypePNG with CFSTR("public.png") (closes [#647](https://github.com/pythonscad/pythonscad/issues/647)) ([#651](https://github.com/pythonscad/pythonscad/issues/651)) ([84d7f83](https://github.com/pythonscad/pythonscad/commit/84d7f839e172bd70317370304b66facc1a51092a))
* **nsis:** split ARP keys by install scope and refine elevation UX ([#702](https://github.com/pythonscad/pythonscad/issues/702)) ([ef8d6f0](https://github.com/pythonscad/pythonscad/commit/ef8d6f0f8395438761a53043053d85865d020254))
* **pip:** ship all lib3mf source variants in PyPI sdist ([#709](https://github.com/pythonscad/pythonscad/issues/709)) ([9afc7ff](https://github.com/pythonscad/pythonscad/commit/9afc7ffc18cfcd0fbe03131c6ad38761d6f09b2c))
* **pre-commit:** exclude changelog from markdownlint validation ([#666](https://github.com/pythonscad/pythonscad/issues/666)) ([eb54833](https://github.com/pythonscad/pythonscad/commit/eb5483312d9c732b31f022d4f852312d1e74e522))
* **python:** correct osimport() defaults for stroke and dpi ([#707](https://github.com/pythonscad/pythonscad/issues/707)) ([7bab843](https://github.com/pythonscad/pythonscad/commit/7bab8436889b17d3602ca853539b976a8b54f4b6))
* **python:** finish python__setitem__ migration to python_pyobject_to_utf8 ([#611](https://github.com/pythonscad/pythonscad/issues/611)) ([4a16fa0](https://github.com/pythonscad/pythonscad/commit/4a16fa0a8584a06dde5e926de8de838ad137ac47))
* **python:** guard FrepNode include behind ENABLE_LIBFIVE ([#628](https://github.com/pythonscad/pythonscad/issues/628)) ([4494631](https://github.com/pythonscad/pythonscad/commit/44946314fbd1520d9acb662318ffa603367a545b))
* **python:** guard PyDict_SetDefaultRef polyfill behind PY_VERSION_HEX ([#602](https://github.com/pythonscad/pythonscad/issues/602)) ([5e21b27](https://github.com/pythonscad/pythonscad/commit/5e21b279aa1967049234c5dbfd702da183244600)), closes [#601](https://github.com/pythonscad/pythonscad/issues/601)
* **python:** handle multiline add_parameter in customizer prescan ([#567](https://github.com/pythonscad/pythonscad/issues/567)) ([af73762](https://github.com/pythonscad/pythonscad/commit/af73762868bc493212276504a21cb4bb1c67a79f)), closes [#566](https://github.com/pythonscad/pythonscad/issues/566)
* **python:** make python_numberval() reject non-parseable str inputs ([#610](https://github.com/pythonscad/pythonscad/issues/610)) ([d54ccfd](https://github.com/pythonscad/pythonscad/commit/d54ccfd0a0956422ea1f8b9737bcf585978fef5b))
* **python:** make solid + solid a union; add minkowski test via % ([#658](https://github.com/pythonscad/pythonscad/issues/658)) ([2c80577](https://github.com/pythonscad/pythonscad/commit/2c80577ecac9c128a268223e9a698510b429c1ed)), closes [#657](https://github.com/pythonscad/pythonscad/issues/657)
* **python:** plug PyOpenSCADObjectToNodeMulti dict leak ([#596](https://github.com/pythonscad/pythonscad/issues/596)) ([#603](https://github.com/pythonscad/pythonscad/issues/603)) ([ad76ba9](https://github.com/pythonscad/pythonscad/commit/ad76ba9ef5bd668aa6a46c714370dd1a1e9ceb20))
* **python:** polyfill PyModule_AddObjectRef for Python 3.9 builds ([#619](https://github.com/pythonscad/pythonscad/issues/619)) ([508555a](https://github.com/pythonscad/pythonscad/commit/508555aaf5868842a00815329b529480016585e4)), closes [#617](https://github.com/pythonscad/pythonscad/issues/617)
* **python:** raise TypeError on non-str dict keys in export() and similar APIs ([#595](https://github.com/pythonscad/pythonscad/issues/595)) ([cbbac27](https://github.com/pythonscad/pythonscad/commit/cbbac27397045af8fd37a777e34c022bf50bfd42))
* **python:** sweep PyBytes_AS_STRING(NULL) UB out of pyopenscad/pyconversion ([#608](https://github.com/pythonscad/pythonscad/issues/608)) ([2b46704](https://github.com/pythonscad/pythonscad/commit/2b46704751a74123aea558c2c541008973a44946))
* **resize:** resize resulted in inconsistent behavior ([22017dd](https://github.com/pythonscad/pythonscad/commit/22017dd241f0d603c4ff5f28af11a81919b210df))
* **resize:** resize resulted in inconsistent behavior ([a222605](https://github.com/pythonscad/pythonscad/commit/a2226050612f1da77b8fc107e0c105689111526d))
* resolve dangling pointer and free-nonheap-object bugs (closes [#644](https://github.com/pythonscad/pythonscad/issues/644)) ([#649](https://github.com/pythonscad/pythonscad/issues/649)) ([de7a4d5](https://github.com/pythonscad/pythonscad/commit/de7a4d58351494f254ce48203e1bd89287f95cba))
* **svg:** fix function of osimport:stroke parameter ([#577](https://github.com/pythonscad/pythonscad/issues/577)) ([9c95792](https://github.com/pythonscad/pythonscad/commit/9c9579216bfebcf29f634bd3c3e6caab097bbbaf))
* **svg:** improved svg output ([ead58fc](https://github.com/pythonscad/pythonscad/commit/ead58fce63658da992f83b94b18781b9fc5385c4))
* **text:** work around fontconfig 2.18.0 app-font scoring regression ([#691](https://github.com/pythonscad/pythonscad/issues/691)) ([4d06ac2](https://github.com/pythonscad/pythonscad/commit/4d06ac23807f379e3ba3cb243a5d383ecb21a63f))


### Documentation

* add release announcement for v1.0.0 ([#714](https://github.com/pythonscad/pythonscad/issues/714)) ([2444c84](https://github.com/pythonscad/pythonscad/commit/2444c84b4c6c224153ae42beb30c59482933131b))
* clarify osuse/osinclude usage and deprecation ([#589](https://github.com/pythonscad/pythonscad/issues/589)) ([4d65aa6](https://github.com/pythonscad/pythonscad/commit/4d65aa6619e9229aa45a5643bced911a85e98d7a))
* use slices instead of fn for linear_extrude with python function ([#593](https://github.com/pythonscad/pythonscad/issues/593)) ([2bdb5c2](https://github.com/pythonscad/pythonscad/commit/2bdb5c2d44528e50eea9de12b026d7c3451f57bd)), closes [#578](https://github.com/pythonscad/pythonscad/issues/578)

## [0.20.0](https://github.com/pythonscad/pythonscad/compare/v0.19.1...v0.20.0) (2026-04-13)


### Features

* **gui:** enable session management by default ([#553](https://github.com/pythonscad/pythonscad/issues/553)) ([097f26c](https://github.com/pythonscad/pythonscad/commit/097f26cb52d3304bad44534dcf449952216c33f4))
* **gui:** log terminal notice on single-instance IPC handoff ([#552](https://github.com/pythonscad/pythonscad/issues/552)) ([b27aca0](https://github.com/pythonscad/pythonscad/commit/b27aca05e0611a8242ba0201392c5fc2b984fd7f))
* **gui:** session restore, customizer params without F5, dry-run safety ([#415](https://github.com/pythonscad/pythonscad/issues/415)) ([0078525](https://github.com/pythonscad/pythonscad/commit/0078525cc47653afb0f5f220ade97c4f3107a3b0))
* **oversample:** add dynamic method ([61a25c7](https://github.com/pythonscad/pythonscad/commit/61a25c71feb0f2f08b01c2ac1b70ec5e3d73d7aa))
* **oversample:** more work ([681be5a](https://github.com/pythonscad/pythonscad/commit/681be5a48f060573ecc8ee0ed191d1af8bb8266b))


### Bug Fixes

* **chore:** undo wrong files ([48f8cc7](https://github.com/pythonscad/pythonscad/commit/48f8cc728a435247116bdca7525bb8d458158ee2))
* **crash:** right Click works better ([7130b80](https://github.com/pythonscad/pythonscad/commit/7130b803bc0d5288145b23ade34d55fdf839409e))
* **crash:** right Click works better ([e89716b](https://github.com/pythonscad/pythonscad/commit/e89716b78185fac6fcc137dc2d7d24988b1e177b))
* fix typo / terminology in code comments ([74b0cf2](https://github.com/pythonscad/pythonscad/commit/74b0cf2317be5e0fe0582e03f392f59de126e66a))
* **gui:** align save-as path, filters, and untitled names with editor language ([#541](https://github.com/pythonscad/pythonscad/issues/541)) ([cd2ba3e](https://github.com/pythonscad/pythonscad/commit/cd2ba3e8511de7e74c64b087a9f0d11d179610c9))
* **gui:** session restore and CLI prompt for missing design files ([#548](https://github.com/pythonscad/pythonscad/issues/548)) ([8477ee3](https://github.com/pythonscad/pythonscad/commit/8477ee3d8825888a8d894d2d96a1046b278e436e))
* **gui:** skip stale-path session dialog for default Untitled files ([#546](https://github.com/pythonscad/pythonscad/issues/546)) ([b3c27f3](https://github.com/pythonscad/pythonscad/commit/b3c27f3af64cdd6d0783bb5a711b46e31118afc6))
* **gui:** use QStandardPaths for session/lock file directory on all platforms ([#540](https://github.com/pythonscad/pythonscad/issues/540)) ([c54c9f6](https://github.com/pythonscad/pythonscad/commit/c54c9f6a430d76f4f2fdae56bb0ee42178100f29))
* **measure:** show correct vertex indexes again ([a8c3e64](https://github.com/pythonscad/pythonscad/commit/a8c3e6483c9aa18c96b1525c3fea89f135efbe73))
* remove duplicate definition of `GeometryEvaluator::applyHull3D(const Geometry::Geometries&)` from src/geometry/GeometryEvaluator.cc ([7e012d4](https://github.com/pythonscad/pythonscad/commit/7e012d4157f92a026c70aad77ec059b868c955af))
* **sync:** undo wrong color ([fb2eee9](https://github.com/pythonscad/pythonscad/commit/fb2eee9f647f956107b8edc32e639311f34c9126))
* update application icon in Windows resource file ([#543](https://github.com/pythonscad/pythonscad/issues/543)) ([d9e3c0c](https://github.com/pythonscad/pythonscad/commit/d9e3c0cae943e4ead5f2ae7528f1a38aef9e761b))
* update nightly icon references in Windows resource file ([#544](https://github.com/pythonscad/pythonscad/issues/544)) ([5f6ab29](https://github.com/pythonscad/pythonscad/commit/5f6ab296aea05ab7dc2add69657a3c224b75e6e6))


### Documentation

* complete Python API cheatsheet and reference documentation ([#531](https://github.com/pythonscad/pythonscad/issues/531)) ([c137d89](https://github.com/pythonscad/pythonscad/commit/c137d89b2227789956884b7b21ecf4d9fecd662d))
* overhaul the upstream-sync documentation ([#538](https://github.com/pythonscad/pythonscad/issues/538)) ([4ba1f8b](https://github.com/pythonscad/pythonscad/commit/4ba1f8b1e86ece9b6a2f0b73f98c5e9814a86bc5))

## [0.19.1](https://github.com/pythonscad/pythonscad/compare/v0.19.0...v0.19.1) (2026-03-31)


### Bug Fixes

* **cdr:** fix compilation on all platforms ([#529](https://github.com/pythonscad/pythonscad/issues/529)) ([6d41a47](https://github.com/pythonscad/pythonscad/commit/6d41a4701602d3719e043b49135af0f91f491a0a))

## [0.19.0](https://github.com/pythonscad/pythonscad/compare/v0.18.0...v0.19.0) (2026-03-31)


### Features

* add polyline primitive ([c87a2a6](https://github.com/pythonscad/pythonscad/commit/c87a2a65ac3a5b7566c14059d305d8bcbe4c2cb8))
* add polyline primitive ([9eae810](https://github.com/pythonscad/pythonscad/commit/9eae81080e86ec05d741c7563f4806e904b5a084))
* add support for importing corel-draw images ([#503](https://github.com/pythonscad/pythonscad/issues/503)) ([ff73f82](https://github.com/pythonscad/pythonscad/commit/ff73f8217f21a4568a8b937e561803a7f14b27eb))
* **add:** update icons-svg-default.scad source for gc, ps, stp icons (previous effort was manually hacked using Inkscape) ([2a81464](https://github.com/pythonscad/pythonscad/commit/2a814647d8e79232a5ee79f25c2eaa60b5ff98fe))
* **gcode:** optimize object order ([#526](https://github.com/pythonscad/pythonscad/issues/526)) ([02ef8ba](https://github.com/pythonscad/pythonscad/commit/02ef8bafbf98c3b489e0ebbfd7328b789b5446c5))
* **gui:** add icons for GC, PS, STP ([#501](https://github.com/pythonscad/pythonscad/issues/501)) ([2a81464](https://github.com/pythonscad/pythonscad/commit/2a814647d8e79232a5ee79f25c2eaa60b5ff98fe))
* **gui:** add icons for GC, PS, STP (fix typo, edit filename, not extension) ([2a81464](https://github.com/pythonscad/pythonscad/commit/2a814647d8e79232a5ee79f25c2eaa60b5ff98fe))
* **gui:** add icons for GC, PS, STP updating icons-chokusen and icons-chokusen-dark .qrc files ([2a81464](https://github.com/pythonscad/pythonscad/commit/2a814647d8e79232a5ee79f25c2eaa60b5ff98fe))
* **gui:** update MainWindow.ui for GC, PS, and STP icons ([2a81464](https://github.com/pythonscad/pythonscad/commit/2a814647d8e79232a5ee79f25c2eaa60b5ff98fe))
* **polyline:** add 3D support ([#528](https://github.com/pythonscad/pythonscad/issues/528)) ([d40f991](https://github.com/pythonscad/pythonscad/commit/d40f991802563ec6b7ad6401cf9ef7d798622fd2))
* **svg,gcode:** SVG class and &lt;style&gt; rules; G-code outlines sorted by area ([#525](https://github.com/pythonscad/pythonscad/issues/525)) ([ec48f89](https://github.com/pythonscad/pythonscad/commit/ec48f89912a4f4ce92ee8eca487517040ba1603e))


### Bug Fixes

* **2d:** improve cleanUnion function ([#519](https://github.com/pythonscad/pythonscad/issues/519)) ([e7c89e2](https://github.com/pythonscad/pythonscad/commit/e7c89e2b7cadebeee43dcc3307b705c04d5024a3))
* add feedAdd property ([cc8c617](https://github.com/pythonscad/pythonscad/commit/cc8c61757362e91e75c9a54306763aede5b86e13))
* add feedAdd property ([a25338d](https://github.com/pythonscad/pythonscad/commit/a25338ddfe5af85606e3b2a89a187db1e3ab7051))
* crash and various and issues/bugfixes with polyline/gcode ([#513](https://github.com/pythonscad/pythonscad/issues/513)) ([863d2ed](https://github.com/pythonscad/pythonscad/commit/863d2ed0e2a4e7a86258db5cc791cd8f856bf72d))
* **gui:** guard against null modinst and path in right-click context menu ([#6693](https://github.com/pythonscad/pythonscad/issues/6693)) ([f20a42b](https://github.com/pythonscad/pythonscad/commit/f20a42b458dba0698a540d31a06cf96106e4503a))
* lazy collider is not used anymore ([2a9ac2b](https://github.com/pythonscad/pythonscad/commit/2a9ac2bcea98050fbc590a61874380f281d81596))
* **polygon:** add discretizer as cache key ([#518](https://github.com/pythonscad/pythonscad/issues/518)) ([ff8f3ad](https://github.com/pythonscad/pythonscad/commit/ff8f3ad603df16e9db87107642224561215a567c))
* prevent font family prefix-matching bug in QFontComboBox ([#6708](https://github.com/pythonscad/pythonscad/issues/6708)) ([afaac53](https://github.com/pythonscad/pythonscad/commit/afaac53349187dcbbef162080f2d67aedce350f6))
* **python:** parse Python from editor buffer, not lastCompiledDoc ([#522](https://github.com/pythonscad/pythonscad/issues/522)) ([0352e1b](https://github.com/pythonscad/pythonscad/commit/0352e1b384bab2a58ead54cdbb5fbfaae1080afb))
* **python:** trust dialog once; reopen and menu to trust ([#523](https://github.com/pythonscad/pythonscad/issues/523)) ([1bed53b](https://github.com/pythonscad/pythonscad/commit/1bed53b88545f76a3ffb5f9145a568af212f681a))
* rectified transform ([757511a](https://github.com/pythonscad/pythonscad/commit/757511a3c4bd99c05402f6f7cb2f98ecf2410b08))
* rectified transform ([c0d5a0b](https://github.com/pythonscad/pythonscad/commit/c0d5a0b02edf016f7fd1ef4eb3717f30cb25fa8c))
* removed debug stuff ([7c1408a](https://github.com/pythonscad/pythonscad/commit/7c1408a47b8c357df116125da3871d5735248f18))
* small bug in pylaser which resulted in short plugs ([64b13eb](https://github.com/pythonscad/pythonscad/commit/64b13eb315f157a6e2a5ca5b8b94e49990107399))

## [0.18.0](https://github.com/pythonscad/pythonscad/compare/v0.17.0...v0.18.0) (2026-03-09)


### Features

* prepare project for PyPI publishing ([#493](https://github.com/pythonscad/pythonscad/issues/493)) ([3cb54e0](https://github.com/pythonscad/pythonscad/commit/3cb54e06f2508cab7969107ae6fb08c21acac92e))


### Bug Fixes

* **build:** add PORTABLE_BINARY=ON to Windows and macOS release builds ([#500](https://github.com/pythonscad/pythonscad/issues/500)) ([a4a2acc](https://github.com/pythonscad/pythonscad/commit/a4a2acc6f9d16bacf65a16ecd3ed4606bfa738fc)), closes [#497](https://github.com/pythonscad/pythonscad/issues/497)
* **gui:** prevent dialog flickering on Qt5 caused by resize oscillation ([#499](https://github.com/pythonscad/pythonscad/issues/499)) ([5186eaf](https://github.com/pythonscad/pythonscad/commit/5186eaf8b03a1e0af892a18172783f035fca2af5))
* resolve Qt6 deprecation warnings in GUI code ([#479](https://github.com/pythonscad/pythonscad/issues/479)) ([a651b48](https://github.com/pythonscad/pythonscad/commit/a651b488b4a4656862ff87af2c08f3ad799a2bad))
* use Fusion style with explicit palettes for reliable cross-platform theming ([#490](https://github.com/pythonscad/pythonscad/issues/490)) ([5e6858b](https://github.com/pythonscad/pythonscad/commit/5e6858ba7828fafcbf9876a0e198bd595da63d1f))


### Documentation

* add rendervars() documentation ([#496](https://github.com/pythonscad/pythonscad/issues/496)) ([f26a430](https://github.com/pythonscad/pythonscad/commit/f26a430a2430b815d08ffe4271e29bd3d9b7742d))

## [0.17.0](https://github.com/pythonscad/pythonscad/compare/v0.16.0...v0.17.0) (2026-03-06)


### Features

* **setrender:** allow python code to set vieweing perspective ([#488](https://github.com/pythonscad/pythonscad/issues/488)) ([de1bde4](https://github.com/pythonscad/pythonscad/commit/de1bde4a0b6219e36703f8263c841dffad0a995d))


### Bug Fixes

* open New Window with Untitled.py tab instead of Untitled.scad ([#489](https://github.com/pythonscad/pythonscad/issues/489)) ([de36a27](https://github.com/pythonscad/pythonscad/commit/de36a27bfe09d2a1a5a4f341f1a96b32456a4090))
* resolve cppcheck findings in src/python/ ([#483](https://github.com/pythonscad/pythonscad/issues/483)) ([abc74b1](https://github.com/pythonscad/pythonscad/commit/abc74b12b30a1d69201cbc853e8316c0a28acead))

## [0.16.0](https://github.com/pythonscad/pythonscad/compare/v0.15.1...v0.16.0) (2026-02-28)


### Features

* **python:** add .size, .position, and .bbox properties for 2D objects ([#474](https://github.com/pythonscad/pythonscad/issues/474)) ([68ecbde](https://github.com/pythonscad/pythonscad/commit/68ecbdefae61abd113ca1be48567d811131bf7f7))

## [0.15.1](https://github.com/pythonscad/pythonscad/compare/v0.15.0...v0.15.1) (2026-02-25)


### Bug Fixes

* auto parameter was not evaluated in resize ([#466](https://github.com/pythonscad/pythonscad/issues/466)) ([aa1f48a](https://github.com/pythonscad/pythonscad/commit/aa1f48a6cad6551039fc95d9c331d760a73bcf17))
* laser tests ([59b0fc7](https://github.com/pythonscad/pythonscad/commit/59b0fc75cd99022279133883466e0ee24251a002))
* prevent segfault when returning Py_None, Py_True and Py_False ([0518066](https://github.com/pythonscad/pythonscad/commit/0518066434f99f75becbcdeed9b9dc5cc916b1a0))
* **windows:** pass release version to CMake to avoid -dirty in package filenames ([#455](https://github.com/pythonscad/pythonscad/issues/455)) ([27d4e67](https://github.com/pythonscad/pythonscad/commit/27d4e6762c8051fa8ba7ea47ed3298fb50970fe6))

## [0.15.0](https://github.com/pythonscad/pythonscad/compare/v0.14.2...v0.15.0) (2026-02-16)


### Features

* **release:** AppImage signing, release checksums, downloads page improvements ([#454](https://github.com/pythonscad/pythonscad/issues/454)) ([8bea546](https://github.com/pythonscad/pythonscad/commit/8bea546696a9f3e03a6941b474913a086a5f3b05))

## [0.14.2](https://github.com/pythonscad/pythonscad/compare/v0.14.1...v0.14.2) (2026-02-15)


### Bug Fixes

* add latest pylaser.py file ([01af6cd](https://github.com/pythonscad/pythonscad/commit/01af6cd7ca04dc1e0802f55f0bbb7cdb9da1c942))
* **macos:** resolve spctl framework validation on macOS 15 and add validation checks ([#451](https://github.com/pythonscad/pythonscad/issues/451)) ([daae2c6](https://github.com/pythonscad/pythonscad/commit/daae2c6ec2d82445ef8a6282c385442e000eaa57))

## [0.14.1](https://github.com/pythonscad/pythonscad/compare/v0.14.0...v0.14.1) (2026-02-13)


### Bug Fixes

* **ci:** validate SFTP secrets and quote host in ssh-keyscan ([#449](https://github.com/pythonscad/pythonscad/issues/449)) ([b17d375](https://github.com/pythonscad/pythonscad/commit/b17d37507b865b3c1c164488c9f4f70419742351))

## [0.14.0](https://github.com/pythonscad/pythonscad/compare/v0.13.0...v0.14.0) (2026-02-13)


### Features

* **macos:** add code signing and notarization for release builds ([#447](https://github.com/pythonscad/pythonscad/issues/447)) ([07172e3](https://github.com/pythonscad/pythonscad/commit/07172e3bd463292244b66ed91bd31eb672652f18))


### Bug Fixes

* added missing include ([30d828b](https://github.com/pythonscad/pythonscad/commit/30d828b747536487cfdbad9ce95808f03aec0049))
* additional sync to upstream/master ([ef871b6](https://github.com/pythonscad/pythonscad/commit/ef871b68a3c40df76d50ab8e8e164df05b7f1d01))
* compiles ([dfe60b4](https://github.com/pythonscad/pythonscad/commit/dfe60b4eded34243091ad092da02eb9f64f2029b))
* **pip:** add minkowski.cpp to manifold sources for pip build ([f05d185](https://github.com/pythonscad/pythonscad/commit/f05d18571bd2a8aeeb07484a835489e0fb70db8b))
* sync all in src/core ([060942a](https://github.com/pythonscad/pythonscad/commit/060942a930ae736416fa2b7e36d4ba853f2beb25))
* sync in src/geometry ([72e9ad8](https://github.com/pythonscad/pythonscad/commit/72e9ad81f8695c565be7f492040c06aec91ed288))

## [0.13.0](https://github.com/pythonscad/pythonscad/compare/v0.12.3...v0.13.0) (2026-02-10)


### Features

* add hasattr, getattr, setattr ([1bf0ddf](https://github.com/pythonscad/pythonscad/commit/1bf0ddfbc0a7928118cd49b072936bb128929341))
* add hasattr, getattr, setattr ([b70016c](https://github.com/pythonscad/pythonscad/commit/b70016ca788e24fe2972a7f03cb59aedd76f5ea7))
* added gcode init and exit code ([da7013a](https://github.com/pythonscad/pythonscad/commit/da7013a7aa4d1d669acc89f73fe336daaea36551))
* added gcode init and exit code ([e52d766](https://github.com/pythonscad/pythonscad/commit/e52d766af6eec5ed50e3c32829ad299600af2690))
* enable libfive across all platforms and fix Python module path ([#427](https://github.com/pythonscad/pythonscad/issues/427)) ([c15e8aa](https://github.com/pythonscad/pythonscad/commit/c15e8aae30be34bd4ee39b0be958b6ef96f1e0a2))
* solids are iteratble ([765a289](https://github.com/pythonscad/pythonscad/commit/765a2897095ce4711e6384d88ad3547c3079be1e))


### Bug Fixes

* cleaned 7 tests ([1bfdb1d](https://github.com/pythonscad/pythonscad/commit/1bfdb1d38f90f1fb00c3c1ad597733759d41fdf2))
* filter git describe to match only PythonSCAD version tags ([de9223d](https://github.com/pythonscad/pythonscad/commit/de9223d4097f85144749d385301ffd67a2fa92f8))
* filter git describe to match only PythonSCAD version tags ([a9d5262](https://github.com/pythonscad/pythonscad/commit/a9d526283caf482cc0fe09f8b56fb21bac0092bb))
* fixed plan script ([3ade205](https://github.com/pythonscad/pythonscad/commit/3ade2059687fa4c7826fbc49e00c7560c56f6b45))
* Merge commit '030f55de832cd024c64c489a440180d7c3725303' into sync/openscad-2026-02-03 ([fe3d4a2](https://github.com/pythonscad/pythonscad/commit/fe3d4a2f2bf5a45ea14e31d78de2f86bb0395cef))
* minor sync ([9fa18b6](https://github.com/pythonscad/pythonscad/commit/9fa18b625a0480e69a8143b1ca3e20d041733aa4))
* switch clang-tidy to install qt6 dependencies as that's the new default ([857c2f2](https://github.com/pythonscad/pythonscad/commit/857c2f290aa5e612a3720341f903f71c11029e16))

## [0.12.3](https://github.com/pythonscad/pythonscad/compare/v0.12.2...v0.12.3) (2026-02-02)


### Bug Fixes

* **build:** enable PORTABLE_BINARY for Debian/APT and RPM package builds ([#418](https://github.com/pythonscad/pythonscad/issues/418)) ([582f989](https://github.com/pythonscad/pythonscad/commit/582f98927abf191894f11c027097a79ebd97b0a5))

## [0.12.2](https://github.com/pythonscad/pythonscad/compare/v0.12.1...v0.12.2) (2026-01-30)


### Documentation

* **web:** upstream sync ([#412](https://github.com/pythonscad/pythonscad/issues/412)) ([46e17ab](https://github.com/pythonscad/pythonscad/commit/46e17ab6c61f713ee10e62a842bdfb83c90cce22))

## [0.12.1](https://github.com/pythonscad/pythonscad/compare/v0.12.0...v0.12.1) (2026-01-29)


### Bug Fixes

* **python:** fix segfault and reference counting bugs in CSG operators ([#407](https://github.com/pythonscad/pythonscad/issues/407)) ([a016057](https://github.com/pythonscad/pythonscad/commit/a016057dcb01c60e677182832580881e62169839))
* **windows:** remove '-dirty' suffix from build version in package filenames ([#409](https://github.com/pythonscad/pythonscad/issues/409)) ([7963144](https://github.com/pythonscad/pythonscad/commit/7963144a3e3ef306422c2d5ab4e9056ec63ecbfc))

## [0.12.0](https://github.com/pythonscad/pythonscad/compare/v0.11.0...v0.12.0) (2026-01-28)


### Features

* add GUI theme preference (light/dark/auto) with native look for light mode ([#405](https://github.com/pythonscad/pythonscad/issues/405)) ([3a1fe90](https://github.com/pythonscad/pythonscad/commit/3a1fe900f2a49e464966f56c23450ae3a5f12c1f))

## [0.11.0](https://github.com/pythonscad/pythonscad/compare/v0.10.4...v0.11.0) (2026-01-27)


### Features

* **macos:** improve DMG install experience and fix release build ([#401](https://github.com/pythonscad/pythonscad/issues/401)) ([f9cccb3](https://github.com/pythonscad/pythonscad/commit/f9cccb3a1c5c085f56a64396131559af50a9af76))

## [0.10.4](https://github.com/pythonscad/pythonscad/compare/v0.10.3...v0.10.4) (2026-01-26)


### Bug Fixes

* include filename in warnings for files included via osuse() ([#387](https://github.com/pythonscad/pythonscad/issues/387)) ([#397](https://github.com/pythonscad/pythonscad/issues/397)) ([103fa36](https://github.com/pythonscad/pythonscad/commit/103fa36272e9e76ac9d628e82ef276a6c8b0bff5))
* negative offset3d creates correct number of roundings ([#392](https://github.com/pythonscad/pythonscad/issues/392)) ([d62134e](https://github.com/pythonscad/pythonscad/commit/d62134ed78fa4579a72bc3d6b85711c3d4cb6809))
* **python:** handle $-variables correctly when calling functions via osuse() ([#386](https://github.com/pythonscad/pythonscad/issues/386)) ([#398](https://github.com/pythonscad/pythonscad/issues/398)) ([900afa4](https://github.com/pythonscad/pythonscad/commit/900afa47ab56be7639218ad28e70d6907c5a038c))

## [0.10.3](https://github.com/pythonscad/pythonscad/compare/v0.10.2...v0.10.3) (2026-01-23)


### Bug Fixes

* **ci:** mark published release as latest in GitHub ([#394](https://github.com/pythonscad/pythonscad/issues/394)) ([30f22bf](https://github.com/pythonscad/pythonscad/commit/30f22bf676fdb92ad94dbe5dabeb2e5d1241f704))
* disable -march=native for portable AppImage builds ([#395](https://github.com/pythonscad/pythonscad/issues/395)) ([390cf27](https://github.com/pythonscad/pythonscad/commit/390cf27773baaa47fc3ccd0ef370bd96f2d00f3f))

## [0.10.2](https://github.com/pythonscad/pythonscad/compare/v0.10.1...v0.10.2) (2026-01-22)


### Bug Fixes

* **spec:** correct man page reference from openscad to pythonscad ([#385](https://github.com/pythonscad/pythonscad/issues/385)) ([b9fb68c](https://github.com/pythonscad/pythonscad/commit/b9fb68c1d869a95b7895904509dafb95e987c66c))

## [0.10.1](https://github.com/pythonscad/pythonscad/compare/v0.10.0...v0.10.1) (2026-01-21)


### Documentation

* migrate manpage from openscad to pythonscad with new options ([#383](https://github.com/pythonscad/pythonscad/issues/383)) ([62a23be](https://github.com/pythonscad/pythonscad/commit/62a23be9edb06b722162026b2be9d47c2ff74e0e))

## [0.10.0](https://github.com/pythonscad/pythonscad/compare/v0.9.1...v0.10.0) (2026-01-21)


### Features

* better lasercutter support ([#374](https://github.com/pythonscad/pythonscad/issues/374)) ([521e500](https://github.com/pythonscad/pythonscad/commit/521e5009503443ccb297dab6166c1409be98a7a0))


### Bug Fixes

* use PythonSCAD icon in application overview ([b8292a4](https://github.com/pythonscad/pythonscad/commit/b8292a498330c53266de07fc3afb3b107bc27f91))
* use PythonSCAD icon in application overview ([cefb7d5](https://github.com/pythonscad/pythonscad/commit/cefb7d5694b85d22689d8b69d4a949b96d0044b8)), closes [#376](https://github.com/pythonscad/pythonscad/issues/376)

## [0.9.1](https://github.com/pythonscad/pythonscad/compare/v0.9.0...v0.9.1) (2026-01-21)


### Documentation

* **web:** various improvements to the website ([#377](https://github.com/pythonscad/pythonscad/issues/377)) ([56ddc34](https://github.com/pythonscad/pythonscad/commit/56ddc34f979734102a5aac6421990fbbbe708a79))

## [0.9.0](https://github.com/pythonscad/pythonscad/compare/v0.8.31...v0.9.0) (2026-01-20)


### Features

* **ci:** add native Windows packaging workflow ([#371](https://github.com/pythonscad/pythonscad/issues/371)) ([88e06fa](https://github.com/pythonscad/pythonscad/commit/88e06fad708b4cdcc381ac2d19aa581383875038))
* **python:** extend add_parameter() with customizer options ([#368](https://github.com/pythonscad/pythonscad/issues/368)) ([2781253](https://github.com/pythonscad/pythonscad/commit/27812538fceca260f5352ed04521a8f6c5c4993c))


### Bug Fixes

* updated tests ([0efb4a3](https://github.com/pythonscad/pythonscad/commit/0efb4a35102d0cee37a37b6b94e656a710bde685))

## [0.8.31](https://github.com/pythonscad/pythonscad/compare/v0.8.30...v0.8.31) (2026-01-16)


### Bug Fixes

* **ci:** fix keygrip extraction for RPM 6.x signing ([#366](https://github.com/pythonscad/pythonscad/issues/366)) ([5d3b069](https://github.com/pythonscad/pythonscad/commit/5d3b0697c82ad3de3b12aa7f989c637fff9ac7d0))

## [0.8.30](https://github.com/pythonscad/pythonscad/compare/v0.8.29...v0.8.30) (2026-01-16)


### Bug Fixes

* **ci:** fix RPM signing for Fedora 43 (RPM 6.x) ([#363](https://github.com/pythonscad/pythonscad/issues/363)) ([c3d6eb2](https://github.com/pythonscad/pythonscad/commit/c3d6eb291d67eea43db466dc24ad542b4ffbca2d))

## [0.8.29](https://github.com/pythonscad/pythonscad/compare/v0.8.28...v0.8.29) (2026-01-16)


### Bug Fixes

* **ci:** import GPG public key to RPM keyring for signature verification ([#361](https://github.com/pythonscad/pythonscad/issues/361)) ([5c990d1](https://github.com/pythonscad/pythonscad/commit/5c990d105ce1c2dee262bae9be2700b712350dfe))

## [0.8.28](https://github.com/pythonscad/pythonscad/compare/v0.8.27...v0.8.28) (2026-01-16)


### Bug Fixes

* **ci:** fix GPG signing and unify build matrices ([#357](https://github.com/pythonscad/pythonscad/issues/357)) ([f2114a8](https://github.com/pythonscad/pythonscad/commit/f2114a8616b8243c072be581704174f1e5325bbb))
* **web:** use theme CSS variables for cheatsheet dark mode support ([#358](https://github.com/pythonscad/pythonscad/issues/358)) ([38f956e](https://github.com/pythonscad/pythonscad/commit/38f956eee0cb978975763233f49aae883c37e84c))

## [0.8.27](https://github.com/pythonscad/pythonscad/compare/v0.8.26...v0.8.27) (2026-01-15)


### Bug Fixes

* **ci:** use prerelease instead of draft for release workflow ([#351](https://github.com/pythonscad/pythonscad/issues/351)) ([148fc2a](https://github.com/pythonscad/pythonscad/commit/148fc2a6f0eee57d3a0b26a0ab39e009818ccdcc))

## [0.8.26](https://github.com/pythonscad/pythonscad/compare/v0.8.25...v0.8.26) (2026-01-14)


### Features

* **web:** improve releases page and add cheat sheet ([#348](https://github.com/pythonscad/pythonscad/issues/348)) ([821a064](https://github.com/pythonscad/pythonscad/commit/821a0643d6d29340445d382177b4a8cab0aa5150))

## [0.8.25](https://github.com/pythonscad/pythonscad/compare/v0.8.24...v0.8.25) (2026-01-14)


### Bug Fixes

* **ci:** AppImage libfive dependency, Qt5 support, ARM64 support, and distribution updates ([#347](https://github.com/pythonscad/pythonscad/issues/347)) ([c4f90fe](https://github.com/pythonscad/pythonscad/commit/c4f90fee300df4c3a0afb2a6c986a0db60db3038))
* **ci:** resolve RPM signing passphrase issue ([#345](https://github.com/pythonscad/pythonscad/issues/345)) ([2defd11](https://github.com/pythonscad/pythonscad/commit/2defd1163cd5e4f0067cc67632ae3eacb33fe4b5))

## [0.8.24](https://github.com/pythonscad/pythonscad/compare/v0.8.23...v0.8.24) (2026-01-14)


### Features

* add installation documentation to website ([#342](https://github.com/pythonscad/pythonscad/issues/342)) ([b397940](https://github.com/pythonscad/pythonscad/commit/b3979403ef422d76f73591bccfd6f51bf1ab855d))


### Bug Fixes

* **rpm:** resolve RPM build failures by forcing static linking ([#341](https://github.com/pythonscad/pythonscad/issues/341)) ([5059125](https://github.com/pythonscad/pythonscad/commit/50591251c51b55b30ade401e213533968d39faa0))
* uv ([460b288](https://github.com/pythonscad/pythonscad/commit/460b288b21795b37b194346547168aac3f41dd55))

## [0.8.23](https://github.com/pythonscad/pythonscad/compare/v0.8.22...v0.8.23) (2026-01-13)


### Bug Fixes

* **rpm:** resolve build failures and missing package signatures ([#336](https://github.com/pythonscad/pythonscad/issues/336)) ([3bf2d36](https://github.com/pythonscad/pythonscad/commit/3bf2d36b4a5359385e58a06cede3c31f7fb5c8c8))

## [0.8.22](https://github.com/pythonscad/pythonscad/compare/v0.8.21...v0.8.22) (2026-01-13)


### Features

* **web:** add website ([#334](https://github.com/pythonscad/pythonscad/issues/334)) ([7aefdd9](https://github.com/pythonscad/pythonscad/commit/7aefdd9c0b028dfb027c2429f44f92e758d4b503))

## [0.8.21](https://github.com/pythonscad/pythonscad/compare/v0.8.20...v0.8.21) (2026-01-13)


### Features

* **ci:** add automated website deployment workflow ([#332](https://github.com/pythonscad/pythonscad/issues/332)) ([0a9cc3a](https://github.com/pythonscad/pythonscad/commit/0a9cc3a48ad3567c316c63c00f1efadfae4859d3))


### Bug Fixes

* **build:** auto-generate lexer and parser files during pip install ([#329](https://github.com/pythonscad/pythonscad/issues/329)) ([202ae08](https://github.com/pythonscad/pythonscad/commit/202ae08664efdd76833f469d3a6e3726f44ccc4c))
* **macos:** correct CFBundleExecutable to match actual binary name ([#327](https://github.com/pythonscad/pythonscad/issues/327)) ([b7c8991](https://github.com/pythonscad/pythonscad/commit/b7c8991f557e976ae590f6fcbf22444ec095fee5))
* **windows:** bundle MSYS2 Python runtime for MXE cross-compiled builds ([#330](https://github.com/pythonscad/pythonscad/issues/330)) ([68fdb86](https://github.com/pythonscad/pythonscad/commit/68fdb86005b98cad9baffb75ed822b23d7cada9b)), closes [#328](https://github.com/pythonscad/pythonscad/issues/328)

## [0.8.20](https://github.com/pythonscad/pythonscad/compare/v0.8.19...v0.8.20) (2026-01-12)


### Bug Fixes

* **ci:** correct package file paths in APT repository ([#325](https://github.com/pythonscad/pythonscad/issues/325)) ([9d6b463](https://github.com/pythonscad/pythonscad/commit/9d6b463fbe564738272738990e5be7d118036791))

## [0.8.19](https://github.com/pythonscad/pythonscad/compare/v0.8.18...v0.8.19) (2026-01-12)


### Bug Fixes

* **ci:** correct shell syntax in APT repository migration ([#323](https://github.com/pythonscad/pythonscad/issues/323)) ([698a5f0](https://github.com/pythonscad/pythonscad/commit/698a5f0cd39b911e217414f4cb5a7076e9f72dca))

## [0.8.18](https://github.com/pythonscad/pythonscad/compare/v0.8.17...v0.8.18) (2026-01-12)


### Bug Fixes

* **ci:** use distribution-specific pools in APT repository ([#321](https://github.com/pythonscad/pythonscad/issues/321)) ([0201d61](https://github.com/pythonscad/pythonscad/commit/0201d6158a168f8d05bae8e5f5e72d303f901e91))

## [0.8.17](https://github.com/pythonscad/pythonscad/compare/v0.8.16...v0.8.17) (2026-01-12)


### Bug Fixes

* **ci:** fix issues with Debian macOS and Windows release builds ([#317](https://github.com/pythonscad/pythonscad/issues/317)) ([88cd316](https://github.com/pythonscad/pythonscad/commit/88cd316e3cffaa0212495d070d31742977bd051f))

## [0.8.16](https://github.com/pythonscad/pythonscad/compare/v0.8.15...v0.8.16) (2026-01-11)


### Bug Fixes

* **ci:** exclude MXE cross-compilation from MSYS2 Python detection ([#314](https://github.com/pythonscad/pythonscad/issues/314)) ([a1fafda](https://github.com/pythonscad/pythonscad/commit/a1fafda951e2cde3f9c18a30cab0ff147d3af35e))
* **ci:** fix apt-repo script exit and improve package naming ([#311](https://github.com/pythonscad/pythonscad/issues/311)) ([b12e6ad](https://github.com/pythonscad/pythonscad/commit/b12e6ad4b3246800be4c9459abfaeedc9fc3d6e0))
* **ci:** update libbrotlidec references to use bundled libbrotlicommon ([#313](https://github.com/pythonscad/pythonscad/issues/313)) ([e02af62](https://github.com/pythonscad/pythonscad/commit/e02af62154ded2168fe4f27d4bf5266c4913ca8d))
* **scripts:** extract heredoc Python code to separate script ([#307](https://github.com/pythonscad/pythonscad/issues/307)) ([9027d6d](https://github.com/pythonscad/pythonscad/commit/9027d6d268bd5ddfcd5506662c31230329245f47))

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
