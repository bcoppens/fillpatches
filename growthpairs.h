/*
    This file is part of a program to fill borders of patches.
    Copyright (C) 2006-2007 Bart Coppens <kde@bartcoppens.be>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GROWTH_PAIRS_H
#define GROWTH_PAIRS_H

#include "patch.h"

// TODO: rename IrreducibleGrowth -> Irreducible; the Growth is not needed
bool isIrreducibleGrowthPair(const Patch& p1, const Patch& p2);
bool isFaceRemovableFromPatch(const Patch& patch, Vertex start, Vertex direction);
bool isBorderIsomorphism(const Patch& p1, const Patch& p2, Vertex p1Start, Vertex p2Start, int p2Direction, std::vector<Vertex>& mapping); // border edges, dir = 0|1

// Moet eigk 2333332 zijn!
bool borderEncodingIsEmbeddableInFullerene(CanonicalBorder b, int length); // Will block '(33333) (Will not detect HIDDEN stuff, or somesuch!)

#endif // GROWTH_PAIRS_H
