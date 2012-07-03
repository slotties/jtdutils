# Contributor: Stefan Lotties <slotties@gmail.com>
pkgname=jtdutils
pkgver=0.6
pkgrel=1
pkgdesc="A collection of tools for analysing Java thread dumps."
arch=('i686' 'x86_64')
url="https://github.com/slotties/jtdutils"
license=('GPL3')
groups=()
depends=('expat' 'ncurses')
makedepends=()
# optdepends=('zsh=4.3.10')
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
source=($pkgname-$pkgver-src.tar.gz)
noextract=()
md5sums=('2534f3b9fdc60560a17fb4011f3b0da1')

build() {
  cd "$srcdir/$pkgname-$pkgver"
  
  mkdir -p $pkgdir/usr/bin
  mkdir -p $pkgdir/usr/share/man/man1
  
  make install BINPATH=$pkgdir/usr/bin MANPATH=$pkgdir/usr/share/man/man1
}

