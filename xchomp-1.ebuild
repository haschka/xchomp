# Distributed under the terms of the GNU General Public License v3

EAPI=8

inherit toolchain-funcs git-r3

DESCRIPTION="xchomp a Pac Man clone originally written by Jerry J Shekhel"
HOMEPAGE="https://github.com/haschka/xchomp"
EGIT_REPO_URI="https://github.com/haschka/xchomp.git"

SLOT="0"
KEYWORDS="amd64 ~x86 ~ppc"
IUSE=""

BDEPEND="x11-libs/libX11"
RDEPEND="x11-libs/libX11"

src_compile() {
        $(tc-getCC) -o -std=gnu89 main.c demo.c contact.c maze.c props.c \
	resources.c drivers.c status.c \
	-I. -I./bitmaps -I/usr/include -I/usr/include/X11 \
	-o xchomp -lX11 ${CFLAGS} ${LDFLAGS}
}

src_install() {
        dobin xchomp
}