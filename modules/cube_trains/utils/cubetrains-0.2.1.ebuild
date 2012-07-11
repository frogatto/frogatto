# Copyright 1999-2012 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header:

EAPI=4

inherit gnome2-utils games vcs-snapshot

DESCRIPTION="Solve routing puzzles in the inner city with Cube Trains"
HOMEPAGE="http://ddr0.github.com/"
SRC_URI="https://github.com/DDR0/Cube_Trains/tarball/${PV} -> ${P}.tar.gz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

RDEPEND=">=dev-libs/boost-1.35
media-libs/libsdl[X]
media-libs/sdl-image[png]
media-libs/sdl-mixer[vorbis]
media-libs/sdl-ttf[X]
media-libs/glew
sys-libs/zlib
virtual/opengl
virtual/glu"
DEPEND="${RDEPEND}"

src_prepare() {
epatch "${FILESDIR}"/${P}-build.patch
}

src_install() {
newgamesbin game ${PN}-bin
games_make_wrapper ${PN} ${PN}-bin "${GAMES_DATADIR}/${PN}"

insinto "${GAMES_DATADIR}"/${PN}
doins master-config.cfg *.ttf

insinto "${GAMES_DATADIR}"/${PN}/modules/cube_trains
doins -r modules/cube_trains/*

newicon -s 32 modules/cube_trains/images/window-icon.png ${PN}.png
make_desktop_entry ${PN}
prepgamesdirs
}

pkg_preinst() {
games_pkg_preinst
gnome2_icon_savelist
}

pkg_postinst() {
games_pkg_postinst
gnome2_icon_cache_update
}

pkg_postrm() {
gnome2_icon_cache_update
}