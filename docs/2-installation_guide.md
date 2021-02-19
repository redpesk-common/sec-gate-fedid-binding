# Installation

## Redpesk

fedid-binding is part of redpesk-common and is available on any redpesk installation.

```bash
sudo dnf install fedid-binding afb-ui-devtools
```

## Other Linux Distributions

**Prerequisite**: should declare redpesk repository: [[instructions-here]]({% chapter_link host-configuration-doc.setup-your-build-host %})

```bash
# Fedora
sudo dnf install fedid-binding afb-ui-devtools bubblewrap libcap

# OpenSuse
sudo zypper install fedid-binding bubblewrap libcap-progs afb-ui-devtools

# Ubuntu
sudo apt-get install fedid-binding-bin afb-ui-devtools bubblewrap libcap2-bin
```

# Quick test

## start fedid-binding samples
```
AFB_FEDID_CONFIG=/var/local/lib/afm/applications/fedid-binding/etc \
afb-binder --name=afb-fedid --binding=/var/local/lib/afm/applications/fedid-binding/lib/afb-fedid.so --verbose
```
## Connect to HTML5 test page

Copy `localhost:1234/devtools/index.html`in your browser address bar to connect to HTML5 test page

*Optionally:*

* if you rather CLI interface to HTML5, feel free to replace 'afb-ui-devtools' with 'afb-client'.

## Rebuild 'fedid-binding' from sources

**Notice**: recompiling fedid-binding is not requirer to implement your own set of commands and/or sandbox containers. You should recompile 'fedid-binding' only when:

* targeting a not supported environment/distribution.
* changing code to fix bug or propose improvement *(contributions are more than welcome)*
* adding custom output formatting encoders. *note: for custom formatting you technically only recompile your new "custom-encoder". Nevertheless tool chain dependencies remain equivalent.*

### Install building dependencies

#### Prerequisite

* declare redpesk repositories (see previous step).
* install typical Linux C/C++ development tool chain gcc+cmake+....

#### Install AFB controller dependencies

* application framework 'afb-binder' & 'afb-binding-devel'
* binding controller 'afb-libcontroller-devel'
* binding helpers 'afb-libhelpers-devel'
* cmake template 'afb-cmake-modules'
* ui-devel html5 'afb-ui-devtools'

>Note: For Ubuntu/OpenSuse/Fedora specific instructions check [redpesk-developer-guide]({% chapter_link host-configuration-doc.setup-your-build-host#install-the-application-framework-1 %})

#### Install fedid-binding specific dependencies

* libcap-ng-devel
* libseccomp-devel
* liblua5.3-devel
* uthash-devel
* bwrap

>Note: all previous dependencies should be available out-of-the-box within any good Linux distribution. Note that Debian as Ubuntu base distro use '.dev' in place of '.devel' for package name.

### Download source from git

```bash
git clone https://github.com/redpesk-common/fedid-binding.git
```

### Build your binding

```bash
mkdir build
cd build
cmake ..
make
```

### Run a test from building tree

```bash
export AFB_FEDID_CONFIG=../conf.d/project/etc/fedid-simple-config.json
afb-binder --name=afb-fedid --binding=./package/lib/afb-fedid.so -vvv
```
