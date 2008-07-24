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

#include <cassert>
#include <cmath>
#include <iostream>
#include <string.h>

#include <sstream> // DEBUG

#include "patch.h"
#include "file.h" // Crossprocess

//#define SLOW_BUT_SAFE_CHECKS

// map<int, bool> borderFillingMap[BorderFillingMaxLen - BorderFillingMinLen];

// bool ignored: contains() -> geen invulling //bool: false -> geen invulling
// 1e int -> map met lengte
static std::map< int, std::map<CanonicalBorder, CanHaveFilling> > borderFillingMap = std::map< int, std::map<CanonicalBorder, CanHaveFilling> >();

#define FILLING_CACHE_DEBUG 1
#ifdef FILLING_CACHE_DEBUG
static int borderFillingMapOKHits = 0;
static int borderFillingMapFailHits = 0;
static int borderFillingMapMaybeHits = 0;
#endif

using namespace std;

int numberOfPentagonsInducedByBorder(Vertex start, Vertex direction, const VertexVector& list) {
    Vertex current = start;
    Vertex next = direction;
    Vertex prev = MaxVertex + 1;
    Neighbours nb;

    uint twos = 0;
    uint threes = 0;

    do {
        prev = current;
        current = next;
        nb = list.at(current);

        int nextIndex = indexOfNextVertex(prev, nb);
        next = nb.nb[nextIndex];
        if (next == InVertex) {
            threes++;
            next = nb.nb[(nextIndex + 1) % 3];
        } else {
            twos++;
            if (next == OutVertex) {
                next = nb.nb[(nextIndex + 1) % 3];
            }
        }
    } while (current != start);

    // twos - threes = 6 - p
    return 6 - twos + threes;
}

vector< vector<short int> > toAdjacencyMatrix(const VertexVector& list) {
    int size = list.size();
    vector< vector<short int> > result(size);

    for (int i = 0; i < size; i++) {
        vector<short int> row(size);
        for (int j = 0; j < 3; j++) {
            Vertex v = list.at(i).nb[j];
            if (v != InVertex && v != OutVertex)
                row.at(v) = 1;
        }
        result.at(i) = row;
    }

    return result;
}


VertexVector readBorder(istream& is) {
    char c;
    VertexVector v;
    Vertex vertex = MinVertex;

    while (is >> c) {
        if (c == '2') {
            Neighbours b;
            b.nb[0] = vertex - 1;
            b.nb[1] = ++vertex;
            b.nb[2] = OutVertex;
            v.push_back(b);
        } else if (c == '3') {
            Neighbours b;
            b.nb[0] = vertex - 1;
            b.nb[1] = InVertex;
            b.nb[2] = ++vertex;
            v.push_back(b);
        }
    }

    // Make it cyclic
    v.front().nb[0] = vertex - 1;
    if (v.back().nb[1] == InVertex)
        v.back().nb[2] = MinVertex;
    else
        v.back().nb[1] = MinVertex;

    return v;
}

istream& operator>>(istream& is, VertexVector& v) {
    v = readBorder(is);
    return is;
}

ostream& operator<<(ostream& os, const VertexVector& vector) {
    int end = vector.size();
    for (int i = 0; i < end; i++) {
        Neighbours v = vector.at(i);
         os << '[';
         for (int j = 0; j < 3; j++) {
/*             switch (v.nb[j]) {
                 case InVertex: os << "In"; break;
                 case OutVertex: os << "Out"; break;
                 default: os << v.nb[j];
             }*/
             if (v.nb[j] == InVertex) os << "In";
             else if (v.nb[j] == OutVertex) os << "Out";
             else os << v.nb[j];
             os << ((j < 2) ? ',' : ']');
        }
    }
    return os;
}

// De startPos hier returned is een '3', de next is de eerste '2' indien die bestaat ###
pair<Vertex, Vertex> findStartPos(const VertexVector& list, Vertex startPos, Vertex direction) {
    // Find best starting position: this is the position where in clockwise
    // search the as much '2's are to be found consecutively
    Vertex start = startPos;
    Vertex currentlyCounting = MaxVertex + 1;
    int currentTwos = 0;
    int bestTwos = 0;

    // Check dit!
    Vertex next = direction;
    Vertex current = start;
    Vertex prev = MaxVertex + 1;
    Neighbours nb;

    // First, make sure that we start at a position with '3'

    begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
        if (next == InVertex) {
            currentlyCounting = current;
            break;
        }
    } end_neighbour_iteration(start, current, next, nb, nextIndex);

    // Check if we actually are at a position with a 3
    // If not, then this might be a good idea to check if this is 5 or 6 big and fail otherwise
    if (currentlyCounting == MaxVertex + 1) {
        return pair<Vertex, Vertex>(startPos, next);
    }

    Vertex best = currentlyCounting;
    Vertex bestNext = MaxVertex + 1;
    Vertex currentlyNext = next;
    current = currentlyCounting;
    start = currentlyCounting;
    get_next_real_neighbour(prev, current, currentlyNext, list);

    // Check dit! (prev)
    get_next_real_neighbour(prev, current, next, list);

    begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
        if (next == InVertex) {
            if (currentTwos > bestTwos) {
                bestTwos = currentTwos;
                best = currentlyCounting;
                bestNext = currentlyNext;
            }
            currentlyCounting = current;
            get_next_real_neighbour(prev, current, currentlyNext, list);
            currentTwos = 0;
        } else {
            currentTwos++;
        }
    } end_neighbour_iteration(start, current, next, nb, nextIndex);

    if (currentTwos > bestTwos) {
        bestTwos = currentTwos;
        best = currentlyCounting;
        bestNext = currentlyNext;
    }

    //cout << "Best starting pos: " << best << " (next: " <<  bestNext
    //     << ") with " << bestTwos << " twos" << endl;

    // ###
    if (bestTwos == 0) {
        return pair<Vertex, Vertex>(prev, current);
    }
    return pair<Vertex, Vertex>(best, bestNext);
}

bool pentagonsConditionMayContinue(int pentagonsNow) {
    if (pentagonsNow < 0)
        return false;

    if (pentagonsNow >= 6) // ### !
        return false;

    return true;
}

// TODO Aparte code indien we de rand van een INGEVULDE patch willen bepalen (geen InVertex meer!) (?)
// Ingevulde rand zal alleen werken als de patch de volledige patch is! Dat is, als de eerste borderLength toppen van de list de rand bepalen, en alleen die!
pair<CanonicalBorder, int> codeForBorder(const VertexVector& list, Vertex startPos, Vertex direction, int onActualBorderLength, std::vector<BorderAutoInfo>* automorphisms) {
    // TODO lookup tabel voor de te &'en stuff ofzo
    // bit 1 -> InVertex, bit 0 -> no Invertex
    Vertex next = direction;
    Vertex current = startPos;
    Vertex prev = MaxVertex + 1;
    Neighbours nb;
    CanonicalBorder code = 0;
    CanonicalBorder smallestCode = 0;

    // FIXME we moeten bij die automorfismen letten op startPos _EN_ direction! (?)
    int edgesSeen = 0;

    // Initialize
    if (onActualBorderLength == 0) {
        begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
            edgesSeen++;
            code <<= CanonicalBorder(1);
            if (next == InVertex) {
                code |= CanonicalBorder(1);
            }
        } end_neighbour_iteration(startPos, current, next, nb, nextIndex);
    } else {
        assert(onActualBorderLength > 0);
        // ### Vertrekken we dan vanaf startpos? Maakt as such
        for (int i = 0; i < onActualBorderLength; i++) {
            nb = list.at((startPos + i + 1) % onActualBorderLength); // OPGEPAST! Bij == 0 is StartPos niet de eerste top die we bekijken!!!! => + 1 hier
            edgesSeen++;
            code <<= CanonicalBorder(1);

            // On an actual border, we look for the non-existance of OutVertex in the neighbours
            if (nb.nb[0] != OutVertex && nb.nb[1] != OutVertex && nb.nb[2] != OutVertex)
                code |= CanonicalBorder(1);
        }
    }

    assert(edgesSeen < 65); // Or we'd have overflown (_PLATFORMONAFHANKELIJK MAKEN_ FIXME)

    smallestCode = code;
    //assert(smallestCode != 0);
    // We have to startPos + edgesSeen - 1 % edgesSeen here, because we start one too far. ### Or is that good, actually?? Also in reverse direction, or not?
    if (automorphisms) {
        automorphisms->clear();
        automorphisms->push_back(BorderAutoInfo((startPos + 1) % edgesSeen, (startPos + 2) % (edgesSeen), 0));
    }

    // Loop over all cyclical shifts (length - 1 ones), shift to the right
    for (int i = 1; i < edgesSeen; i++) {
        code = code >> CanonicalBorder(1) | (CanonicalBorder(code & CanonicalBorder(1)) << CanonicalBorder(edgesSeen - 1));
        if (code < smallestCode) {
            smallestCode = code;
            // Each loop, I go one position backwards (### direction??)
            if (automorphisms) {
                automorphisms->clear();
                automorphisms->push_back(BorderAutoInfo((startPos + i * (edgesSeen - 1) + 1) % (edgesSeen), (startPos + i * (edgesSeen - 1) + 2) % (edgesSeen), 0));
            }
        } else if (code == smallestCode) {
            if (automorphisms) {
                automorphisms->push_back(BorderAutoInfo((startPos + i * (edgesSeen - 1) + 1) % (edgesSeen), (startPos + i * (edgesSeen - 1) + 2) % (edgesSeen), 0));
            }
        }
    }

    // Mirror
    next = direction;
    current = startPos;
    code = 0;
    int edges = edgesSeen;
    edgesSeen = 0;
    // Initialize OPGEPAST! Start pos is niet de eerste top die we bekijken!!!!
    if (onActualBorderLength == 0) {
        begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
            edgesSeen++;
            if (next == InVertex) {
                code |= CanonicalBorder(1) << CanonicalBorder(edgesSeen - 1); // 322 -> 001, so first 3 -> 0 shift
            }
        } end_neighbour_iteration(startPos, current, next, nb, nextIndex);
    } else {
        assert(onActualBorderLength > 0);
        // ### Vertrekken we dan vanaf startpos? Maakt as such
        for (int i = 0; i < onActualBorderLength; i++) {
            nb = list.at((startPos + i + 1) % onActualBorderLength); // OPGEPAST! Bij == 0 is StartPos niet de eerste top die we bekijken!!!! => + 1 hier
            edgesSeen++;
            // On an actual border, we look for the non-existance of OutVertex in the neighbours
            if (nb.nb[0] != OutVertex && nb.nb[1] != OutVertex && nb.nb[2] != OutVertex)
                code |= CanonicalBorder(1) << CanonicalBorder(edgesSeen - 1);
        }
    }

    if (code < smallestCode) {
        smallestCode = code;
        if (automorphisms) {
            automorphisms->clear();
            automorphisms->push_back(BorderAutoInfo((startPos) % (edgesSeen), (startPos + edgesSeen - 1)  % (edgesSeen), 1));
        }
    } else if (code == smallestCode) {
        if (automorphisms) {
             automorphisms->push_back(BorderAutoInfo((startPos) % (edgesSeen), (startPos + edgesSeen - 1)  % (edgesSeen), 1));
        }
    }

    // Loop over all cyclical shifts (length - 1 ones), shift to the right
    for (int i = 1; i < edgesSeen; i++) {
        code = code >> CanonicalBorder(1) | ((code & CanonicalBorder(1)) << CanonicalBorder(edgesSeen - 1));
        if (code < smallestCode) {
            smallestCode = code;
            if (automorphisms) {
                automorphisms->clear();
                automorphisms->push_back(BorderAutoInfo((startPos + i) % (edgesSeen), (startPos + i + edgesSeen - 1) % (edgesSeen), 1));
            }
        } else if (code == smallestCode) {
            if (automorphisms) {
                automorphisms->push_back(BorderAutoInfo((startPos + i) % (edgesSeen), (startPos + i + edgesSeen - 1) % (edgesSeen), 1));
            }
        }
    }

    // Dan hebben we tenminste de automorfismen
    if (edgesSeen < BorderFillingMinLen)
        return pair<CanonicalBorder, int>(-1,0);
    if (edgesSeen > BorderFillingMaxLen)
        return pair<CanonicalBorder, int>(-1,0);

    return pair<CanonicalBorder,int>(smallestCode, edgesSeen);
}

//#define USE_CROSSPROCESS_FILE
//#define USE_INTERNAL_TREE
//#define LIMIT_TREE
#define INTERNAL_TREE_LIMIT 20
//#define TRY_PRELOADED_FIRST

CanHaveFilling canBorderHaveFilling(CanonicalBorder borderCode, int borderLength) {
#if 1
    assert(borderLength <= BorderFillingMaxLen && borderLength >= BorderFillingMinLen);

#ifdef TRY_PRELOADED_FIRST
    if (isInPreloaded(borderCode, borderLength, dynamicFDStuff)) {
        CanHaveFilling c = borderHasFillingFromPreloaded(borderCode, borderLength, dynamicFDStuff);
#ifdef FILLING_CACHE_DEBUG
        if (c == CanHaveYes)
            borderFillingMapOKHits++;
        else if (c == CanHaveNo)
            borderFillingMapFailHits++;
        else
            borderFillingMapMaybeHits++;
#endif // Debug
        return c;
    }
#endif // Try preloaded first (WILL try again if failed?)

#ifdef USE_INTERNAL_TREE
#ifdef LIMIT_TREE
    if (borderLength <= INTERNAL_TREE_LIMIT) {
#endif // Limit
    const map<CanonicalBorder, CanHaveFilling>& bmap = borderFillingMap[borderLength];
    const map<CanonicalBorder, CanHaveFilling>::const_iterator found = bmap.find(borderCode);

    if (found != bmap.end()) {
#ifdef FILLING_CACHE_DEBUG
        if ((*found).second == CanHaveYes)
            borderFillingMapOKHits++;
        else
            borderFillingMapFailHits++;
#endif // Debug

        return (*found).second;
    }
#ifdef LIMIT_TREE
    }
#endif // Limit

#endif // Use internal tree

#ifdef USE_CROSSPROCESS_FILE
    // If the internal tree was used: at this point the information was not yet in the tree (so a No already got handled)
    CanHaveFilling c = borderHasFillingFromFile(borderCode, borderLength, dynamicFDStuff.map, dynamicFDStuff);

#ifdef FILLING_CACHE_DEBUG
    if (c == CanHaveYes)
        borderFillingMapOKHits++;
    else if (c == CanHaveNo)
        borderFillingMapFailHits++;
    else
        borderFillingMapMaybeHits++;
#endif // Debug
    return c;
#endif // Crossprocess

// This will be dead code with crossprocess code

#ifdef FILLING_CACHE_DEBUG
    borderFillingMapMaybeHits++;
#endif

#endif // Enabled
    return CanHaveMaybe;
}

void addBorderCanHaveFilling(CanonicalBorder borderCode, int borderLength, CanHaveFilling can) {
    assert(borderLength <= BorderFillingMaxLen);
    assert(borderLength >= BorderFillingMinLen);

#ifdef USE_INTERNAL_TREE
    map<CanonicalBorder, CanHaveFilling>& bmap = borderFillingMap[borderLength];
    bmap[borderCode] = can;
#endif // Use internal tree

#ifdef USE_CROSSPROCESS_FILE
    // We only call this function in case we have NEW information
    if (can == CanHaveNo) { // The only useful information
        addBorderInfoToFile(borderCode, borderLength, CanHaveNo, dynamicFDStuff.map, dynamicFDStuff);
    }
#endif // Crossprocess
}

vector<Patch> tryAddNGonToBorder(AddMode mode, Patch& patch, Vertex startPos, Vertex direction, int hexagonsMax, int pentagonsMax) {
    if (mode == BeforeFilling) {
        //VertexVector original = patch.list;
        // Keep track of seen boundaries, and bail out early if we know for sure it won't have any fillings
        BorderInformation info = analyzeBorder(patch.list, startPos, direction);
#ifdef SLOW_BUT_SAFE_CHECKS
        assertAnalyzedInformationOK(info, patch.list, startPos, direction);
#endif

        CanonicalBorder code = info.borderCode;
        int len = info.length;
        CanHaveFilling canHave = CanHaveMaybe;
        if (info.canBeQueried) {
            canHave = canBorderHaveFilling(code, len);
            if (canHave == CanHaveNo) {
                return vector<Patch>();
            }
        }

        int p = info.pentagons;

        // If it is canhaveyes, don't bother the pentagons thingie (though hexagons still may apply, look into it! ### Cache pentagons count??)
        if (canHave == CanHaveMaybe) {
            // (### Aja, en hier moeten we dan wel ook invullen in de dynamic db eh)
            if (!pentagonsConditionMayContinue(p)) {
                return vector<Patch>();
            }
        }

        // Deze vullen de rand recursief, door deze functie terug aan te roepen
        vector<Patch> l = tryAddNGonToBorder(InFilling, patch, info.startPos, info.direction, hexagonsMax, p);

        if (info.canBeQueried) {
            if (canHave == CanHaveMaybe) {
                addBorderCanHaveFilling(code, len, (l.size() == 0) ? CanHaveNo : CanHaveYes);
            }
        }

        return l;
    }

    assert(mode == InFilling);

    Vertex current = startPos;
    Vertex next = direction;
    Vertex prev = MaxVertex + 1;
    Neighbours nb;

    VertexVector& list(patch.list);
    vector<Patch> resultPatches;

    int edgesSeen = 0;

    // If we'd start on a position where we the next vertex is an InVertex,
    // try to move one place backwards. Technically, current is wrongt then ###
    if (next > MaxVertex) {
        next = current;
        current = InVertex;
    }

    do {
//        if (DEBUG)
//            cout << prev << ">" << current << ">" << next << endl;

        prev = current;
        current = next;
        nb = list.at(current);
        edgesSeen++;

        // If the number of edges we saw will definitely be bigger than the
        // size of our ngon, fail

        if (edgesSeen > 6)
            return resultPatches;

        int nextIndex = indexOfNextVertex(prev, nb);
        next = nb.nb[nextIndex];

        if (next == InVertex) {
            // If the current vertex is an InVertex, and we already
            // saw 6 edges, we'd have at least a 7-gon, so stop.
            // Also, if we currently are at the startPos again,
            // then we had a full circle, so abort too.
            if (edgesSeen == 6 || current == startPos) // kan breken met die next = current; hierboven? ###
                return resultPatches;
#if 1
            int ngonEdges = edgesSeen + 1;

            // If we made it to 5 or 6 edges, we must (try to connect it)
            // to the start position.
            if (ngonEdges == 6 || ngonEdges == 5) { // Making the ngon complete
                // Dit kan enkel voorkomen indien de rand geen exacte 5 of 6 hoek is (anders hadden we hier geen invertex)
                // Don't add double edges:
                Vertex v;
                get_next_real_neighbour(prev, current, v, list);
                if (v == startPos) {
                    return resultPatches;
                }

                assert(current != startPos);

                // Connect the nodes with an edge
                int prevIndex = indexOfPrevVertex(direction, list.at(startPos));
                Vertex origNext = list.at(current).nb[nextIndex];
                Vertex origPrev = list.at(startPos).nb[prevIndex];

                list.at(current).nb[nextIndex] = startPos;
                list.at(startPos).nb[prevIndex] = current;

                // Now recursively try to fill this
                if (pentagonsMax == 6) { // If p = 6, then add a pentagon to the border, not a hexagon! (is probably '-1')
                    if (ngonEdges == 5) {
                        resultPatches = tryAddNGonToBorder(BeforeFilling, patch, startPos, current, hexagonsMax, pentagonsMax);
                    }
                } else {
                    if (ngonEdges == 6)
                        patch.hexagons++;
                    resultPatches = tryAddNGonToBorder(BeforeFilling, patch, startPos, current, hexagonsMax, pentagonsMax);
                    if (ngonEdges == 6)
                        patch.hexagons--;
                }

                list.at(current).nb[nextIndex] = origNext;
                list.at(startPos).nb[prevIndex] = origPrev;
            }

            // If we didn't yet come to the maximum of edges (6), we may try
            // to form a penta or hexagon by adding a new vertex or connecting it to another
            if (ngonEdges <= 5)
#endif
            {
                // Possibility 1: Add a vertex
                Vertex added = addVertexToList(list);
                list.at(added).nb[0] = current;
                int prevIndex = indexOfNextVertex(prev, list.at(current));
                Vertex origPrev = list.at(current).nb[prevIndex];
                list.at(current).nb[prevIndex] = added;

                // Now try again to fill this up with a n-gon.
                vector<Patch> l2 = tryAddNGonToBorder(InFilling, patch, startPos, direction, hexagonsMax, pentagonsMax);

                for (uint i = 0; i < l2.size(); i++) {
                    // ### !!! De andere zijde hoeft niet meer te worden opgevuld, omdat deze al door de vorige RECURSIEVE stap zal zijn ingevuld!
                    // Namelijk, van op het moment dat de recursieve stap een vijf- of zeshoek zal toegevoegd hebben, zal hij automatisch deze kant ook vullen!
                    resultPatches.push_back(l2.at(i));
                }

                list.pop_back();
                list.at(current).nb[prevIndex] = origPrev;
#if 1
            } // ### Die indentatie klopt hier niet meer, en die #ifs zijn vast fout met die accolades tegenwoordig..

        if (ngonEdges <= 4) { // If we already saw 4 edges, don't even try!!
                // Possibility 2: make a connection to all other vertices in the current 'hole'
                // .first = vertex, .second = next
                vector< pair<Vertex, Vertex> > possibleTargets;
                Vertex next2 = direction;
                Vertex current2 = startPos;
                Vertex prev2 = MaxVertex;

                // Make a list of all possible targets. That is, all In-neighbours in this hole

                // We can overwrite nb, since we won't loop the mainloop after this
                begin_neighbour_iteration(prev2, current2, next2, nb, list, nextIndex2) {
                    // ### Wat als we vertex hebben met nb ~= [ X, In, In ]?
                    if (next2 == InVertex) {
                        Vertex next3;
                        Vertex x = current2;
                        get_next_real_neighbour(prev2, current2, next3, list);
                        // Continue in the right (= other) direction
                        // ### in feite moet ik de 2 In's toevoegen, eh, voor meerdere oplossingen die mogelijk zijn! (mss best met indices werken, niet nextburen)
                        if (next3 == InVertex) {
                            next2 = prev2;
                            assert(x == current);
                        }
                        possibleTargets.push_back(pair<Vertex, Vertex>(current2, next2));
                    }
                } end_neighbour_iteration(startPos, current2, next2, nb, nextIndex2);

                // Now, for each possible target, try that target:
                int s = possibleTargets.size();

                for (int i = 0; i < s; i++) {

                    // Connect current to target[i], and conquer both sides
                    pair<Vertex, Vertex> p = possibleTargets.at(i);
                    Vertex v = p.first;
                    Vertex n = p.second;

                    // The current node will also be in the list, skip (move this up?)
                    if (current == v)
                        continue;

                    // Don't add double edges: if the current node
                    // is already a neighbour of this node, skip it (move up as well?)
                    if (indexOfVertex(v, list.at(current)) != -1) {
                        continue;
                    }

                    // ### maak niet gewoon een vijfhoek dus

                    assert(startPos != n); // ###

                    // Connect the nodes
                    int prevIndex = indexOfVertex(n, list.at(v));
                    Vertex origNext = list.at(current).nb[nextIndex];
                    Vertex origPrev = list.at(v).nb[prevIndex];

                    list.at(current).nb[nextIndex] = v;
                    list.at(v).nb[prevIndex] = current;

                    // We'll fill one side, but first check if the pentagon condition on the other side is satisfied:
                    // Check the cache (### die condities hier en hierboven in 1 (of 2, met de add erachter) functie gooien??
                    //pair<CanonicalBorder, int> p2 = codeForBorder(list, v, current);
                    BorderInformation p2 = analyzeBorder(list, v, current);
                    int len = p2.length;
                    CanHaveFilling canHave = CanHaveMaybe;
                    if (p2.canBeQueried) {
                        canHave = canBorderHaveFilling(p2.borderCode, len);
                        if (canHave == CanHaveNo) {
                            list.at(current).nb[nextIndex] = origPrev;
                            list.at(v).nb[prevIndex] = origNext;
                            continue;
                        }
                    }
                    // Met die code > 0, ook wegwerken? ###
                    int pentagonsOtherSide = numberOfPentagonsInducedByBorder(v, current, list); // ### Zwier de borderinfo mee in de params!!!

                    if (!pentagonsConditionMayContinue(pentagonsOtherSide)) {
                        list.at(current).nb[nextIndex] = origPrev;
                        list.at(v).nb[prevIndex] = origNext;
                        continue;
                    }

                    // Try to fill one side

                    vector<Patch> l2 = tryAddNGonToBorder(BeforeFilling, patch, current, v, hexagonsMax, pentagonsMax);
                    // in case canHave == maybe for the second internal border, it will get filled in during the call here.
#if 1       // ### WAAROM IS HIER EEN ELSE ???
                    for (uint j = 0; j < l2.size() && canHave != CanHaveNo; j++) {
                        // We succeeded, try to fill the other side
                        vector<Patch> l3 = tryAddNGonToBorder(BeforeFilling, l2.at(j), v, current, hexagonsMax, pentagonsMax);
                        if (l3.size() > 0) {
                            // Both sides got filled: OK!
                        } else {
                        }
                        if (p2.canBeQueried && canHave == CanHaveMaybe) {
                            canHave = (l3.size() == 0) ? CanHaveNo : CanHaveYes;
                            addBorderCanHaveFilling(p2.borderCode, len, canHave);
                        }
                        for (uint k = 0; k < l3.size(); k++) {
                            resultPatches.push_back(l3.at(k));
                        }
                    }
#else       // ### WAAROM IS HIER EEN ELSE ???
                    if (l2.size() > 0) {
                        // If we have at least one filling of the first border, make all fillings of the other part. Then combine all OTHER fillings of left with right
                        vector<Patch> l3 = tryAddNGonToBorder(BeforeFilling, l2.at(0), v, current, hexagonsMax, pentagonsMax);
                        for (int k = 0; k < l3.size(); k++) {
                            resultPatches.push_back(l3.at(k));
                        }

                        // Now we added one of all possible filling combinations: one left with all right. Now we use that information to combine
                        // l2[0] should still 
                        int baseOffset = l2.at(0).list.size();
                        Neighbours nb2;
                        // TODO alle l.size() calls in forlussen naar buiten brengen, snelheid
                        for (int j = 1; j < l2.size(); j++) {
                            /* So what we need to do. We take the l3 list. This is the list of all fillings of the 'unfilled' part of l2[0].
                               We'll then take all other fillings of the left part, that is, l2[1..l2.size]. We then add to each of these l2[i]
                              all possible fillings that we get from l3. What we need to take care of: l2[0] might have more/less (new) nodes
                              than l2[i], so while adding these new nodes to l2[i], we have to see that they are connected to the right nodes.
                              New nodes in l3 start at baseOffset. New nodes in l2[i] should start at l2[i].size, hence if we see a neighbour in
                              l3 >= baseOffset (and <= MaxVertex), it should be translated as l2[i].size() + (neigbhour - baseOffset). But first
                              loop over the border itself. FIXME kijk naar het aantal zeshoeken hier!
                            */
                            for (int k = 0; k < l3.size(); k++) {
                                Vertex next2 = current;
                                Vertex current2 = v;
                                Vertex prev2 = MaxVertex;
                                Patch pa = l2.at(i); // ### Ik hoop dat dit de list kopieert!
                                int copyOffset = pa.list.size();
                                // Vertexnumbers of the border should be equal in l2[0] and l2[i]
                                VertexVector& li(pa.list);
                                // We change nb because we won't loop the main loop anymore
                                begin_neighbour_iteration(prev2, current2, next2, nb, li, nextIndex2) {
                                    nb2 = li.at(current2);
                                    for (int b = 0; b < 3; b++) {
                                        if ( (nb.nb[i] > MaxVertex) || (nb.nb[i] < baseOffset) )
                                            assert(nb2.nb[i] == nb.nb[i]); // Should already be filled in!
                                        else
                                            nb2.nb[i] = copyOffset + (nb.nb[i] - baseOffset);
                                    }
                                    li.at(current2) = nb2;
                                } end_neighbour_iteration(startPos, current2, next2, nb, nextIndex2);
                                // Now we copied the border information, we will add all vertices that were added to l2[0] to l2[i]
                                // We change nb because we won't loop the main loop anymore
                                for (int l = baseOffset; l < l3.at(k).list.size(); l++) {
                                    nb = l3.at(k).list.at(l);
                                    for (int b = 0; b < 3; b++) {
                                        if ( (nb.nb[i] > MaxVertex) || (nb.nb[i] < baseOffset) )
                                            nb2.nb[i] = nb.nb[i];
                                        else
                                            nb2.nb[i] = copyOffset + (nb.nb[i] - baseOffset);
                                    }
                                    li.push_back(nb2);
                                }
                                resultPatches.push_back(pa);
                            }
                        }
                    }
#endif
                    if (l2.size() == 0) {
                        assert(!isNGon(6, list, current, v));
                    }

                    list.at(current).nb[nextIndex] = origPrev;
                    list.at(v).nb[prevIndex] = origNext;

                }
#endif
            }

            // The next node is an InVertex, but we couldn't find
            // any filling for it that is OK. Thus, we fail :-(
            return resultPatches;
        } else {
            if (next == OutVertex) {
                next = nb.nb[(nextIndex + 1) % 3];
            }
        }
    } while (current != startPos);

    // We might just have made a complete ngon
    if (edgesSeen == 5) {
        resultPatches.push_back(patch);
        return resultPatches; // TRUE
    } else if ((edgesSeen == 6) && pentagonsMax < 6) { // p = 6 -> only add a pentagon to the border (probably == '-1'
        resultPatches.push_back(patch);
        return resultPatches; // TRUE
    }

    return resultPatches;
}

vector<Patch> addPentagonToBorderAndFill(Patch& patch, Vertex startPos, Vertex direction) {
    // Idea: if p = 6, add a pentagon at all possible positions in the border, and then fill in
    // To do this: find all '3's in the border, call the tryAddNGonToBorder function for p = 6 and InFilling (which specialcases this): this will do all the work
    // The reason I don't do this manually: the fact that a pentagon can also 'cross' to the other side of the border!
    // Note: I don't care for symmetries at the moment -> we too much work! ### TODO

    Vertex current = startPos;
    Vertex prev = InVertex;
    Vertex next = direction;
    Neighbours nb;

    vector<Patch> result;

    begin_neighbour_iteration(prev, current, next, nb, patch.list, nextIndex) {
        if (next == InVertex) {
            // We hebben een '3' top gevonden in de rand. Kies de volgende als direction
            Vertex currentStart = current;
            get_next_real_neighbour(prev, current, next, patch.list);
            Vertex to = next;

            vector<Patch> r = tryAddNGonToBorder(InFilling, patch, currentStart, to, -1, 6);

            for (uint i = 0; i < r.size(); i++)
                result.push_back(r.at(i));
        }

    } end_neighbour_iteration(startPos, current, next, nb, nextIndex);

    return result;
}

void CanonicalForm::clone(const CanonicalForm& rhs, bool dirty) {
    if (rhs.length) {
        if (dirty && length == rhs.length) {
            memcpy(code, rhs.code, 3 * length * sizeof(Vertex));
        } else {
            length = rhs.length;
            if (dirty)
                delete[] code;
            code = new Vertex[3*length];
            memcpy(code, rhs.code, 3 * length * sizeof(Vertex));
        }
    } else {
        length = 0;
        if (dirty)
            delete[] code;
        code = 0;
    }
}

bool operator<(const CanonicalForm& lhs, const CanonicalForm& rhs) {
    if (lhs.length != rhs.length)
        return (lhs.length < rhs.length);
    int size = 3 * lhs.length;
    for (int i = 0; i < size; i++) {
        if (lhs.code[i] > rhs.code[i])
            return false;
        if (lhs.code[i] < rhs.code[i])
            return true;
    }
    // If we got here, lhs[size-1] == rhs[size-1], so it is not '<'
    return false;
}

bool operator==(const CanonicalForm& lhs, const CanonicalForm& rhs) {
    if (lhs.length != rhs.length)
        return false;
    int size = 3 * lhs.length;
    for (int i = 0; i < size; i++)
        if (lhs.code[i] != rhs.code[i])
            return false;
    return true;
}

ostream& operator<<(ostream& os, const CanonicalForm& form) {
    os << "<" << form.length << ">";
    for (int i = 0; i < form.length; i++) {
        os << "[" << form.code[3*i] << ","
                  << form.code[3*i+1] << ","
                  << form.code[3*i+2] << "]";
    }
    return os;
}


// Compute the smallest canonical form, with the start top starting on the edge
CanonicalForm computeCanonicalForm(const Patch& patch, bool d, vector<PatchAutoInfo>* autoMorphisms) {
    bool haveCanonicalForm = false;
    CanonicalForm canonicalForm;
    const VertexVector list = patch.list;
    int automorphisms = 0;
    vector<PatchAutoInfo> found;
    canonicalForm.length = list.size();
    canonicalForm.code = new Vertex[3*canonicalForm.length];
    vector<Vertex> mapping(list.size()); // Old -> New
    vector<Vertex> rmapping(list.size()); // New -> Old
    vector<bool> mapped(list.size());

    // The first borderLength vertices are the border, loop over them
//#define DONT_USE_BORDERAUTS // ### Dit werkt niet meer, maar toch stond het aan sinds r165 (27 Jun 2007), waarom?
#ifdef DONT_USE_BORDERAUTS
    for (int i = 0; i < patch.borderLength; i++) {
        // Currently this has no 'early-abort', fix that! ### is that true?
        Neighbours nb = list.at(i);
        for (int j = 0; j < 3; j++) {
            if (nb.nb[j] > MaxVertex)
                continue;
            for (int direction = 0; direction <= 1; direction++) {
#else
    vector<BorderAutoInfo> borderAuts;
    // ### onActualBorder is TRUE! Ongetest!
    codeForBorder(patch.list, 0, 1, patch.borderLength, &borderAuts);
    for (vector<BorderAutoInfo>::const_iterator it = borderAuts.begin(); it != borderAuts.end(); ++it ) {
        int i = (*it).start;
        Neighbours nb = list.at(i);
        int direction = (*it).direction;
#endif

        assert(i >= 0);
        assert(static_cast<unsigned int>(i) < patch.borderLength);
        for (int j = 0; j < 3; j++) {
            if (nb.nb[j] > MaxVertex)
                continue;
                // Picked a start vertex, oriented edge (start-to) and a direction
                // So now fill in the maybe canonical form:
                Vertex* code = canonicalForm.code;
                int codePos = 0;
                bool overWriting = !haveCanonicalForm;
                haveCanonicalForm = true;

                // We will be 'moving' around the labels of the edges, store that permutation here

                int firstFreeLabel = 0;
                for (uint k = 0; k < list.size(); k++) {
                    mapped.at(k) = false;
                }

                // Map start vertex:
                mapped.at(i) = true; // Set to true, or we'll try to change it later on
                mapping.at(i) = 0;
                rmapping.at(0) = i;
                firstFreeLabel++;
                bool abort = false;

                for (uint l = 0; l < list.size() &&!abort; l++) {
                    Vertex v = rmapping.at(l);
                    Neighbours nb2 = list.at(v);

                    int offset = 0;
                    if (l == 0) { // We fix the first neighbor
                        offset = j; // In this case, nb2 == list.at(rmap(0)) = list.at(i) == nb
                    } else {
                        // Choose the smallest neighbor
                        Vertex x = OutVertex;
                        for (int n = 0; n < 3; n++) {
                            if (nb2.nb[n] <= MaxVertex && mapped.at(nb2.nb[n])) {
                                Vertex y = mapping.at(nb2.nb[n]);
                                if (y < x) {
                                    x = y;
                                    offset = n;
                                }
                            }
                        }
                    }

                    for (int m = 0; m < 3 &&!abort; m++) {
                        Vertex currentNeighbor;
                        if (direction == 0) // FIXME arbitrary?
                            currentNeighbor = nb2.nb[(offset + m)%3];
                        else
                            currentNeighbor = nb2.nb[(3+offset-m)%3];
                        // See if we need to chose a smaller numbering for this neighbor
                        if (currentNeighbor <= MaxVertex) {
                            if (mapped.at(currentNeighbor)) {
                                currentNeighbor = mapping.at(currentNeighbor);
                            } else {
                                mapped.at(currentNeighbor) = true;
                                mapping.at(currentNeighbor) = firstFreeLabel;
                                rmapping.at(firstFreeLabel) = currentNeighbor;
                                currentNeighbor = firstFreeLabel;
                                firstFreeLabel++;
                            }
                        }
                        if (!overWriting) {
                            if (currentNeighbor < code[codePos]) {
                                overWriting = true;
                            } else if (currentNeighbor > code[codePos]) {
                                abort = true;
                            }
                        }
                        if (overWriting) {
                            code[codePos] = currentNeighbor;
                        }
                        codePos++;
                    }
                }
                ///*
                if (overWriting) {
                    found.clear();
                    found.push_back(PatchAutoInfo(canonicalForm, i, nb.nb[j], direction));
                } else if (!abort) {
                    found.push_back(PatchAutoInfo(canonicalForm, i, nb.nb[j], direction));
                }//*/
#ifdef DONT_USE_BORDERAUTS
            } //
    }
#endif
        } //
    }
    // Tel automorfismen:
    if(autoMorphisms)
        autoMorphisms->clear();
    if (d || autoMorphisms) {
        for (uint i = 0; i < found.size(); i++) {
            if (found.at(i).form == canonicalForm) {
                if (d) {
                    automorphisms++;
                    cerr << "Automorfisme: (" << found.at(i).start << " -> " << found.at(i).to << ", dir: " << found.at(i).direction << ")" << endl;
                }
                if (autoMorphisms) {
                    autoMorphisms->push_back(found.at(i));
                }
            }
        }
        if (d) {
            cerr << automorphisms << " automorfismen gevonden" << endl;
        }
    }

    return canonicalForm;
}

bool isNGon(int n, const VertexVector& list, Vertex startPos, Vertex direction) {
    Vertex next = direction;
    Vertex current = startPos;
    Vertex prev = MaxVertex + 1;
    Neighbours nb;

    int edgesSeen = 0;
    begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
        edgesSeen++;
        if (edgesSeen > n) {
            return false;
        }
        if (next > MaxVertex) {

            return false;
        }
    } end_neighbour_iteration(startPos, current, next, nb, nextIndex);

    return edgesSeen == n;
}

int nGon(const VertexVector& list, Vertex startPos, Vertex direction, bool* hasInVertex) {
    Vertex next = direction;
    Vertex current = startPos;
    Vertex prev = MaxVertex + 1;
    Neighbours nb;

    int edgesSeen = 0;

    begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
        edgesSeen++;
        if (next > MaxVertex) {
            if (hasInVertex) {
                *hasInVertex = true;
            }
        }
    } end_neighbour_iteration(startPos, current, next, nb, nextIndex);

    return edgesSeen;
}


Vertex addVertexToList(VertexVector& list) {
    Neighbours nb;
    Vertex newVertex = list.size();
    nb.nb[0] = InVertex;
    nb.nb[1] = InVertex;
    nb.nb[2] = InVertex;
    list.push_back(nb);

    return newVertex;
}


// ### Zie dat we hier niet vertices hebben die overflowen
void outputWriteGraph2D(const VertexVector& list, ostream& out, bool addInVertex, bool addOne) {
    bool addedIn = false;
    vector<Vertex> toIn;
    // This assumes that the stream has been 'initialized' by having a
    // '>>writegraph2d<<\n' line as first line
    Vertex fakeInVertex = list.size() + 1;
    for (uint i = 0; i < list.size(); i++) {
        if (!addOne)
            out << i << " ";
        else
            out << (i + 1) << " "; // Starts with vertex called '1' instead of '0'
        out << "0 0 "; // No coordinates
        for (int j = 0; j < 3; j++) {
            if (list.at(i).nb[j] <= MaxVertex) // ### InVertex om te debuggen
                if (addOne)
                    out << (list.at(i).nb[j] + 1) << " ";
                else
                    out << (list.at(i).nb[j]) << " ";
            else if (list.at(i).nb[j] == InVertex && addInVertex) {
                if (toIn.size() > 0 && toIn.back() == Vertex(i+1)) {
                    continue;
                }
                out << fakeInVertex << " ";
                toIn.push_back(Vertex(i + 1));
                addedIn = true;
            }
        }
        out << "\n";
    }
    if (addedIn) {
        out << fakeInVertex << " 0 0 ";
        for (uint i = 0; i < toIn.size(); i++)
            if (addOne)
                out << toIn.at(i) << " ";
            else
                out << (toIn.at(i) - 1)<< " ";
        out << "\n";
    }
    if (list.size() > 0) {
        out << "0\n"; // Done with this graph
    } else {
        cerr << "list.size == 0 in outputWriteGraph2D" << endl;
    }
}

//#define OUTPUT_SHORTS

void outputPlanarCode(const VertexVector& list, std::ostream& out) {
    // Assumes the stream was initialized by >>planar_code le<<
    // Output the number of vertices:
    //cout << "PlanarCode: " << list.size() << endl;
    char c = (char) list.size();
    out << c;
    // Loop over all vertices, then iterating their neighbours (clockwise!)
    for (uint i = 0; i < list.size(); i++) {
        for (uint j = 0; j < 3; j++) {
            if (list.at(i).nb[j] <= MaxVertex) {
                assert(list.at(i).nb[j] < 256); // Fits in a char!
#ifdef OUTPUT_SHORTS // ### HACK
                c = 0;
                out << c;
#endif
                c = (char) (list.at(i).nb[j] + 1);
                out << c;
            }
        }
        // End with a 0
        c = 0;
        out << c; // Ook hier???? ###
#ifdef OUTPUT_SHORTS // ### HACK
        out << c;
#endif
    }
    // Don't end with a 0
}

void addFinishedGraph(VertexVector& list) {
    assert(finishedStream);
    outputWriteGraph2D(list, *finishedStream, false);
}

void printCycle(Vertex current, Vertex next, const VertexVector& list) {
    Vertex start = current;
    Vertex prev = InVertex;
    Neighbours nb;

    begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
        // For the [ X, In, In] situation
    if (next > MaxVertex) {
        next = prev;
        cout << "3";
    } else {
        cout << "2";
    }
        get_next_real_neighbour(prev, current, next, list);
        // We go back in the other direction
        if (next > MaxVertex) {
            next = prev;
        }
    } end_neighbour_iteration(start, current, next, nb, nextIndex);

    cout << endl;
}

void printBorderFillingInfo() {
#ifdef FILLING_CACHE_DEBUG
    typedef map< int, std::map<CanonicalBorder, CanHaveFilling> >::iterator BMapIterator;
    cerr << "Different sizes in map: " << borderFillingMap.size() << endl;
    for (BMapIterator it = borderFillingMap.begin(); it != borderFillingMap.end(); ++it) {
        cerr << "Map[" << (*it).first << "] Size: " << (*it).second.size() << endl;
    }
    cerr << "Hits Maybe: " << borderFillingMapMaybeHits << endl;
    cerr << "Hits OK: " << borderFillingMapOKHits << endl;
    cerr << "Hits No: " << borderFillingMapFailHits << endl;
    if (borderFillingMapFailHits > 0) {
        cerr << "Percent of hits that gave early bailout: "
             << ((double(borderFillingMapFailHits) / double(borderFillingMapFailHits+borderFillingMapOKHits+borderFillingMapMaybeHits))*100.0) << endl;
    }
#endif
}

// TODO Aparte code indien we de rand van een INGEVULDE patch willen bepalen (geen InVertex meer!) (?)
// Ingevulde rand zal alleen werken als de patch de volledige patch is! Dat is, als de eerste borderLength toppen van de list de rand bepalen, en alleen die!

// !!! Automorphisms only makes sense when the border is the border of the PATCH!
BorderInformation analyzeBorder(const VertexVector& list, Vertex startPos, Vertex direction, int onActualBorderLength, std::vector<BorderAutoInfo>* automorphisms) {
    // TODO lookup tabel voor de te &'en stuff ofzo
    // bit 1 -> InVertex, bit 0 -> no Invertex

    Vertex next = direction;
    Vertex current = startPos;
    Vertex prev = MaxVertex + 1;
    Neighbours nb;
    CanonicalBorder code = 0;
    CanonicalBorder smallestCode = 0;

    BorderInformation result;
    result.twos = 0;
    result.threes = 0;
    result.length = 0;
    result.pentagons = 0;
    result.borderCode = 0;
    result.canBeQueried = true;
    result.startPos = 0;
    result.direction = 0;

    vector<Vertex> offsetToVertex;

    // FIXME we moeten bij die automorfismen letten op startPos _EN_ direction!
    int edgesSeen = 0;

    // Initialize
    if (onActualBorderLength == 0) {
        begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
            edgesSeen++;
            code <<= CanonicalBorder(1);
            if (next == InVertex) {
                code |= CanonicalBorder(1);
                result.threes++;
            } else {
                result.twos++;
            }
            /* Take note here: we need to take a step 'backwards'. Example: imagine the initial try is the shortest one. Hence, code will start with '0',
               which will be the startPos as well. This is wrong, we need the prev*/
            offsetToVertex.push_back(prev); // Op positie i is te vinden: de ide vertex die we tegen kwamen.
        } end_neighbour_iteration(startPos, current, next, nb, nextIndex);
    } else {
        assert(onActualBorderLength > 0);
        // ### Vertrekken we dan vanaf startpos? Maakt as such
        for (int i = 0; i < onActualBorderLength; i++) {
            nb = list.at((startPos + i + 1) % onActualBorderLength); // OPGEPAST! Bij == 0 is StartPos niet de eerste top die we bekijken!!!! => + 1 hier
            edgesSeen++;
            code <<= CanonicalBorder(1);

            // On an actual border, we look for the non-existance of OutVertex in the neighbours
            if (nb.nb[0] != OutVertex && nb.nb[1] != OutVertex && nb.nb[2] != OutVertex) {
                code |= CanonicalBorder(1);
                result.threes++;
            } else {
                result.twos++;
            }
            offsetToVertex.push_back((startPos + i) % onActualBorderLength); // Op positie i is te vinden: de ide vertex die we tegen kwamen. ### (Same as above)
        }
    }

    result.length = result.twos + result.threes;
    result.pentagons = 6 - result.twos + result.threes;
    if ((edgesSeen < BorderFillingMinLen) || (edgesSeen > BorderFillingMaxLen))
        result.canBeQueried = false;

    assert(static_cast<uint>(result.length) == offsetToVertex.size());

    result.startPos = offsetToVertex.at(0); // 0
    result.direction = offsetToVertex.at(1); // 1

    assert(edgesSeen < 65); // Or we'd have overflown PLATFORMONAFHANKELIJK FIXME

    smallestCode = code;
    //assert(smallestCode != 0);

    // We have to startPos + edgesSeen - 1 % edgesSeen here, because we start one too far. ### Or is that good, actually?? Also in reverse direction, or not?
    if (automorphisms) {
        automorphisms->clear();
        automorphisms->push_back(BorderAutoInfo((startPos + 1) % edgesSeen, (startPos + 2) % (edgesSeen), 0));
    }

    // Loop over all cyclical shifts (length - 1 ones), shift to the right
    for (int i = 1; i < edgesSeen; i++) {
        code = (code >> CanonicalBorder(1)) | (CanonicalBorder(code & CanonicalBorder(1)) << CanonicalBorder(edgesSeen - 1));
        if (code < smallestCode) {
            smallestCode = code;
            // Each loop, I go one position backwards (### de direction??)
            if (automorphisms) {
                automorphisms->clear();
                automorphisms->push_back(BorderAutoInfo((startPos + i * (edgesSeen - 1) + 1) % (edgesSeen), (startPos + i * (edgesSeen - 1) + 2) % (edgesSeen), 0));
            }

            result.startPos = offsetToVertex.at((result.length - i) % result.length); // i
            result.direction = offsetToVertex.at((result.length - i + 1) % result.length); // (i + 1) % result.length
        } else if (code == smallestCode) {
            if (automorphisms) {
                automorphisms->push_back(BorderAutoInfo((startPos + i * (edgesSeen - 1) + 1) % (edgesSeen), (startPos + i * (edgesSeen - 1) + 2) % (edgesSeen), 0));
            }
        }
    }

    // ____ Mirror ____
    next = direction;
    current = startPos;
    code = 0;
    int edges = edgesSeen;
    edgesSeen = 0;
    // Initialize OPGEPAST! Start pos is niet de eerste top die we bekijken!!!!
    if (onActualBorderLength == 0) {
        begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
            edgesSeen++;
            if (next == InVertex) {
                code |= CanonicalBorder(1) << CanonicalBorder(edgesSeen - 1); // 322 -> 001, so first 3 -> 0 shift
            }
        } end_neighbour_iteration(startPos, current, next, nb, nextIndex);
    } else {
        assert(onActualBorderLength > 0);
        // ### Vertrekken we dan vanaf startpos? Maakt as such
        for (int i = 0; i < onActualBorderLength; i++) {
            nb = list.at((startPos + i + 1) % onActualBorderLength); // OPGEPAST! Bij == 0 is StartPos niet de eerste top die we bekijken!!!! => + 1 hier
            edgesSeen++;
            // On an actual border, we look for the non-existance of OutVertex in the neighbours
            if (nb.nb[0] != OutVertex && nb.nb[1] != OutVertex && nb.nb[2] != OutVertex)
                code |= CanonicalBorder(1) << CanonicalBorder(edgesSeen - 1);
        }
    }

    if (code < smallestCode) {
        smallestCode = code;
        if (automorphisms) {
            automorphisms->clear();
            automorphisms->push_back(BorderAutoInfo((startPos) % (edgesSeen), (startPos + edgesSeen - 1)  % (edgesSeen), 1));
        }
    } else if (code == smallestCode) {
        if (automorphisms) {
            automorphisms->push_back(BorderAutoInfo((startPos) % (edgesSeen), (startPos + edgesSeen - 1)  % (edgesSeen), 1));
        }
    }

    // Loop over all cyclical shifts (length - 1 ones), shift to the right
    for (int i = 1; i < edgesSeen; i++) {
        code = (code >> CanonicalBorder(1)) | ((code & CanonicalBorder(1)) << CanonicalBorder(edgesSeen - 1));
        if (code < smallestCode) {
            smallestCode = code;
            if (automorphisms) {
                automorphisms->clear();
                automorphisms->push_back(BorderAutoInfo((startPos + i) % (edgesSeen), (startPos + i + edgesSeen - 1) % (edgesSeen), 1));
            }
        } else if (code == smallestCode) {
            if (automorphisms) {
                automorphisms->push_back(BorderAutoInfo((startPos + i) % (edgesSeen), (startPos + i + edgesSeen - 1) % (edgesSeen), 1));
            }
        }
    }

    result.borderCode = smallestCode;

    return result;
}

void assertAnalyzedInformationOK(const BorderInformation& info, const VertexVector& list, Vertex startPos, Vertex direction, int onActualBorderLength) {
    pair<Vertex, Vertex> foundStartPos = findStartPos(list, startPos, direction);
    /* ### BAH, this relation does not hold, so untestable (similar initial 000s
       can have different 1011 at the end, which might incur a difference between findStartPos and the comparison-based borderinfo*/
    /*
    assert(foundStartPos.first == info.startPos);
    assert(foundStartPos.second == info.direction);
    */

    if (info.borderCode != CanonicalBorder(0)) { // Regular n-gon
        Neighbours nb;
        nb = list.at(info.startPos);
        assert ( (nb.nb[0] == InVertex) || (nb.nb[1] == InVertex) || (nb.nb[2] == InVertex));
    }

    if (onActualBorderLength == 0) { // ###
        int initial2_found = 0;
        int initial2_info = 0;
        {
            Vertex prev = MaxVertex + 1;
            Vertex current = foundStartPos.first;
            Vertex next = foundStartPos.second;
            Vertex startPos = foundStartPos.first;
            Neighbours nb;

            bool initialrun = true;

            begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
                if (next == InVertex) {
                    initialrun = false;
                } else {
                    if (initialrun)
                        initial2_found++;
                }
            } end_neighbour_iteration(startPos, current, next, nb, nextIndex);
        }
        {
            Vertex prev = MaxVertex + 1;
            Vertex current = info.startPos;
            Vertex next = info.direction;
            Vertex startPos = foundStartPos.first;
            Neighbours nb;

            bool initialrun = true;

            begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
                if (next == InVertex) {
                    initialrun = false;
                } else {
                    if (initialrun)
                        initial2_info++;
                }
            } end_neighbour_iteration(startPos, current, next, nb, nextIndex);
        }
        assert(initial2_found == initial2_info);
    }

    int pentagons = numberOfPentagonsInducedByBorder(startPos, direction, list);
    assert(pentagons == info.pentagons);

    int n = nGon(list, startPos, direction);
    assert(n == info.length);

    // Untested: twos/threes (but implicit in length & p)

    // Untested: AUTOMORPHISMS! ###
    pair<CanonicalBorder, int> code = codeForBorder(list, startPos, direction, onActualBorderLength);

    if (info.canBeQueried) {
        assert(info.borderCode == code.first);
        assert(code.second == info.length);
    } else {
        assert(code.first == static_cast<CanonicalBorder>(-1)); // ### SUCKT
        assert(code.second == 0);
    }
}

