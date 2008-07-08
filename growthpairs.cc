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

#include <iostream>
#include <cassert>
#include "growthpairs.h"

using namespace std;
static bool d = false;
bool isFaceRemovableFromPatch(const Patch& patch, Vertex start, Vertex direction) {
    Vertex current = start;
    Vertex prev = InVertex;
    Vertex next = direction;
    const VertexVector& list = patch.list;
    Neighbours nb;
    bool prevOnEdge = false;
    int differentOutsides = 0;

    begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
        // It lies on the border if the start and stop vertices lie on the
        // border _and_ the difference between start->direction is 1 (mod size):
        // otherwise we count edges having a single edge from one side of the patch
        // to the other
        if (current < patch.borderLength && next < patch.borderLength
            && ((patch.borderLength + next - current) % patch.borderLength) == 1) { // Does not connect 2 different regions of the border?
            // This edge lies on the border
            if (!prevOnEdge) {
                prevOnEdge = true;
                differentOutsides++;
            }
        } else {
            prevOnEdge = false;
        }
    } end_neighbour_iteration(start, current, next, nb, nextIndex);

    if (differentOutsides > 2)
        return false;
    else if (differentOutsides <= 1)
        return true;

    // There is the possibility that we saw 2 outsides, but it was only one!
    // For example if we started halfway the only outside, then cross
    // the internal part, and then come out of it, we will count the first part
    // again.
    // This only happens when the edge given as start is on the outside, as
    // is the next: the edge we recieve as start is actually the _last_ we visit,
    // so if differentOutsides == 2, and that vertex, and the next, are on the outside,
    // we maybe counted too much.

    if (start < patch.borderLength && direction < patch.borderLength
        && ((patch.borderLength + direction - start) % patch.borderLength) == 1) { // Prev on border
        nb = list.at(direction);
        int nextIndex = indexOfNextVertex(start, nb);
        next = nb.nb[nextIndex];
        // Next on border (startpos == direction, already there)
        if (next < patch.borderLength && ((patch.borderLength + next - direction) % patch.borderLength) == 1) {
            return true;
        }
    }

    return false;
}

// the mapping is the mapping from p1 to p2 (gets changed, even if return false, but if returns true, the mapping is 'complete')
bool isBorderIsomorphism(const Patch& p1, const Patch& p2, Vertex p1Start, Vertex p2Start, int p2Direction, vector<Vertex>& mapping) {
    // What we check, is that if we walk around p1's border from p1Start, in the direction of p1Direction (0 and 1 are
    // arbitrary indicators for left or right) and simultaneously we walk on p2 with the same parameters, we check if
    // each '2' bordervertex on p1 corresponds with a '2' vertex on p2, and the same for the '3' ones.

    Neighbours nb1, nb2;

    // We abuse the fact here that the vertices of the border are the first vertices of a patch
    for (unsigned int i = 0; i < p1.borderLength; i++) {
        bool hasOutEdge1 = false;
        bool hasOutEdge2 = false;
        Vertex v2;

        nb1 = p1.list.at( (p1Start + i) % p1.borderLength );
        if (p2Direction == 0) {
            v2 = (p2Start + i) % p2.borderLength;
        } else {
            v2 =  (p2.borderLength + p2Start - i) % p2.borderLength;
        }
        nb2 = p2.list.at(v2);

        for (int j = 0; j < 3; j++) {
            if (nb1.nb[j] == OutVertex)
                hasOutEdge1 = true;
            if (nb2.nb[j] == OutVertex)
                hasOutEdge2 = true;
        }

        if (hasOutEdge1 != hasOutEdge2)
            return false;

        assert(v2 < p2.borderLength);
        mapping.at(i) = v2;
    }

    return true;
}

bool isIrreducibleGrowthPair(const Patch& p1, const Patch& p2) {
    Neighbours nb;

    // What we choose for p1 is not really important ###
    Vertex p1Start = 0;
    vector<Vertex> mapping(p1.borderLength);
    vector<bool> visited(p1.borderLength);

    assert(p1.borderLength == p2.borderLength);

    // We choose one ordered edge in p1, and then
    // try mapping it to any possible ordered edge so that we get an
    // isomorphism of the border.
    for (unsigned int i = 0; i < p2.borderLength; i++) {
        nb = p2.list.at(i);
        for (int j = 0; j <= 1; j++) {
            // We have a border edge, see if it is indeed isomorph
            if (!isBorderIsomorphism(p1, p2, p1Start, i, j, mapping)) {
                continue;
            }

            // Now we iterate over each face on the boundary of p1, see
            // if it is removable. If it is, then we see what its image is
            // under the isomorphism. If that is also removable, we
            // see if they both are 5- or 6-gons. If they are,
            // the pair is reducible.

            // The border in the patch is 0->1->2->...->0, use that
            for (unsigned int k = 0; k < p1.borderLength; k++)
                visited.at(k) = false;

            bool allRemovableFacesMappedToDifferents = true;
            for (unsigned int k = 0; k < p1.borderLength; k++) {
                //if (visited.at(k))
                //    continue;
                // We get the edge k -> k+1 (mod length):
                Vertex p1To = (k + 1) % p1.borderLength;
                // Is the k->k+1 edge-bounded face removable?

                if (!isFaceRemovableFromPatch(p1, k, p1To))
                    continue;

                // Is the face bounded by the isomorph-edge on p2 removable?
                if (j == 0) {
                    // Direction not switched, regular direction
                    if (!isFaceRemovableFromPatch(p2, mapping.at(k), mapping.at(p1To)))
                        continue;
                } else {
                    // Direction switched, reverse direction ### isn't it a hack?
                    if (!isFaceRemovableFromPatch(p2, mapping.at(p1To), mapping.at(k)))
                        continue;
                }

                // Are the number of edges on both sides the same?
                int face1Edges = nGon(p1.list, k, p1To, 0);
                int face2Edges;
                if (j == 0) {
                    // Direction not switched, regular direction
                    face2Edges = nGon(p2.list, mapping.at(k), mapping.at(p1To), 0);
                } else {
                    // Direction switched, reverse direction ### isn't it a hack?
                    face2Edges = nGon(p2.list, mapping.at(p1To), mapping.at(k), 0);
                }

                // We have an isomorphism of the boundaries that maps a non
                if (face1Edges == face2Edges) {
                    allRemovableFacesMappedToDifferents = false;
                }
            }
            if (allRemovableFacesMappedToDifferents) {
                return true;
            }
        }
    }
    // Since we assume the patches of the pair have the same boundaries,
    // there is certainly an isomorphism. So, if we arrive here, we conclude
    // that an isomorphism of the boundaries exists, that does not map any
    // removable face of p1 to a removable face of p2 of the same size (or
    // we'd have return'ed already). And so this pair is irreducible.
    return false;
}

bool borderEncodingIsEmbeddableInFullerene(CanonicalBorder b, int length) {
    // Will block '(33333) (Will not detect HIDDEN stuff, or somesuch!)
    static const unsigned int mask = (1) | (1<<1) | (1<<2) | (1<<3) | (1<<4);
    if (length < 5)
        return true; // 'Safe' assumption
    for (int i = 0; i <= length - 5; i++) {
        if ((b & CanonicalBorder(mask << i)) == CanonicalBorder(mask << i))
            return false;
    }
    return true;
}

