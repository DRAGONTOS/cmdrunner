pkgname=cmdrunner-git
_gitname=cmdrunner
pkgver=0.1.0
pkgrel=1
pkgdesc="A dmenu/rofi-compatible command runner in the terminal."
url="https://github.com/DRAGONTOS/cmdrunner"
arch=('x86_64')
license=('GPLv3')
makedepends=('git' 'cmake' 'ninja')
provides=('cmdrunner')
conflicts=('cmdrunner')
source=('git+https://github.com/DRAGONTOS/cmdrunner.git')
md5sums=('SKIP')

pkgver() {
    cd ${_gitname}
    sed -n 's/.*"version": "\([^"]*\)".*/\1/p' props.json
}

prepare() {
    rm -rf build
}

build() {
    cd ${_gitname}
    cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -S . -B ./build -G Ninja
    cmake --build ./build
}

package() {
    cd ${_gitname}
    install -Dm755 ./build/cmdrunner ${pkgdir}/usr/bin/cmdrunner
}
