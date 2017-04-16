NAME
====

Resultcat Run - a dumb runner game, but in C because Flash is even dumber


SYNOPSIS
========

Resultcat runs on his own. Click your mouse or press return, space or the up
arrow to jump. The escape key quits the game.


BUILDING
========

You need a C compiler that somewhat supports the C standard from 2011.

You also need the [SDL 2](https://www.libsdl.org/),
[SDL\_image 2](https://www.libsdl.org/projects/SDL_image/) and
[SDL\_ttf 2](https://www.libsdl.org/projects/SDL_ttf/) libraries. They are
usually available in your package manager, if your system has one of those.

See the [Makefile](Makefile) for the incantation to compile the game with
GCC-like compilers.


LEVELS
======

The game's levels are just PNG images in the [assets directory](assets). They
consist of black pixels for solid blocks and white pixels for emptiness.

You can add levels simply by putting them in that folder and calling them
`levelX.png`, where `X` is some number between 0 and 127. The numbers do not
need to be consecutive. If you have a crazy amount of levels for some reason,
you can raise the maximum limit of 128 by changing the `#define MAX_LEVELS` in
[the code](rcrun.c) and recompiling.

The level with the lowest number (`level0.png` by default) is special: it'll
always be the first level to be loaded when the game starts and will never
appear again. This is used to start you in a safe situation while the screen
fades in.

To test your own level without having to wait for it to be randomly picked,
just remove all levels aside from `level0.png` and yours from the assets
directory. Then the game will have no choice but to pick your level each time.


BUGS
====

The collision detection is different from the Flash version, because the way it
works there is just too funky. The difference in gameplay is relatively minor
though.


LICENSE
=======

Copyright 2017 askmeaboutloom.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the [GNU General Public License](LICENSE)
along with this program.  If not, see <http://www.gnu.org/licenses/>.

The font [Roboto Regular](assets/Roboto-Regular.ttf) is owned by Google and
distributed under the Apache License, version 2.0. You can find a copy of the
license in [assets/Roboto-Regular-LICENSE](assets/Roboto-Regular-LICENSE).
