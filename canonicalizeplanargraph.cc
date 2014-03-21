/*
    This file is part of a program to fill borders of patches.
    Copyright (C) 2007 Bart Coppens <kde@bartcoppens.be>

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
#include <streambuf>
#include <sstream>
#include <fstream>
#include <set>
#include <list>
#include <vector>
#include <ctime>
#include <cassert>

#include "patch.h"
#include "file.h"
#include "growthpairs.h"

using namespace std;

ostream* debugStream = 0;
ostream* finishedStream = 0;
FdAndMap dynamicFDStuff;

bool findBorderVertices(const VertexVector& list, Vertex startPos, Vertex direction, vector<Vertex>& borderVertices) {
    Vertex next = direction;
    Vertex current = startPos;
    Vertex prev = MaxVertex + 1;
    Neighbours nb;

    int edgesSeen = 0;
    borderVertices.clear();

    begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) {
        edgesSeen++;
        borderVertices.push_back(current);
    } end_neighbour_iteration(startPos, current, next, nb, nextIndex);

    if (edgesSeen > 6)
        return true;
    return false;
}

Patch inputPlanarCode(std::istream& in) {
    assert(in.good());

    VertexVector v;
    Patch patch;
    // Assumes the stream was stripped from the initial >>planar_code le<< (no \n!)
    // Number of vertices
    unsigned char numberOfVertices;
    numberOfVertices = (unsigned char) in.get();
    assert(in.good());

    bool skip_zero = false; // Format where the first thing to be read is a '0', and then each value is 2 chars instead of one (we will assume every first char == 0)
    if (numberOfVertices == 0) {
        //cerr << "Presumably format with 0s" << endl;
        skip_zero = true;
        numberOfVertices = (unsigned char) in.get();
    }
    assert(numberOfVertices > 6);

    vector<Vertex> borderVerticesWithTwo;
    // Loop over all vertices, then iterating their neighbours (clockwise!)
    for (int i = 0; i < numberOfVertices; i++) {
        unsigned char newNb;
        int neighbours = 0;
        Neighbours nb;

        // A list of neighbours follows, ending with a \0. The vertex labeling hence starts from _1_, normalize to 0
        if (skip_zero) {
            newNb = (unsigned char) in.get();
            assert(newNb == 0);
        }
        newNb = (unsigned char) in.get();

        assert(newNb != '\0');
        while (newNb != '\0' && neighbours < 3) {
            nb.nb[neighbours++] = newNb - 1;
            if (skip_zero) {
                newNb = (unsigned char) in.get();
                assert(newNb == 0);
            }
            newNb = (unsigned char) in.get();
        }

        assert(newNb == '\0');
        assert(neighbours == 2 || neighbours == 3);

        if (neighbours == 2) { // Border node!
            nb.nb[2] = OutVertex;
            borderVerticesWithTwo.push_back(v.size()); // If this is the 2nd Vertex, its label = 1, and there is already one in the vertexvector
        }
        v.push_back(nb);
    }

    // ### WAAROM IS DIT NODIG?
    if (skip_zero) {
        unsigned char c = (unsigned char) in.get();
        assert(c == 0);
    }

    // Ok, so now we have a) a vertexvector b) a list of possible border edges. Find the border amongst those. Assumes at least one '2' node (consistent with p < 6)
    assert(borderVerticesWithTwo.size() > 0);
    // We should always find the border after inspecting the FIRST 2 vertex: nGon from that vertex to each of his neighbours -> one should return 5|6, the other > 6
    // This is nb[0] or nb[1]

    vector<Vertex> borderVertices;
    bool reverseOrder = false;
    if (!findBorderVertices(v, borderVerticesWithTwo.at(0), v.at(borderVerticesWithTwo.at(0)).nb[0], borderVertices)) {
        // Tweede poging!
        bool ok = findBorderVertices(v, borderVerticesWithTwo.at(0), v.at(borderVerticesWithTwo.at(0)).nb[1], borderVertices);
        assert(ok);
        // The order of rotation is reverse to what we expect, don't forget to turn this around
        reverseOrder = true;
    }

    // The borderlength equals the amount of border vertices
    patch.borderLength = borderVertices.size();
    patch.list = v;

    // We now have all borderVertices. We relabel all vertices: borderVertices get 0 .. borderLength - 1; the rest gets the higher numbers.
    vector<Vertex> relabeling; // old -> new
    vector<Vertex> reverselabeling; // new -> old
    Vertex nextFreeLabel = 0;
    VertexVector relabeled;

    for (int i = 0; i < v.size(); i++) { // Init ### sneller opzoeken
        relabeling.push_back(-1);
        reverselabeling.push_back(-1);
        relabeled.push_back(Neighbours());
    }

    // Initialize the relabeling for the border, relabel the border
    for (int i = 0; i < patch.borderLength; i++) {
        // The order of the border here is reverse of what it should be in the list!
        if(reverseOrder) {
            reverselabeling.at(nextFreeLabel) = borderVertices.at(patch.borderLength - i - 1);
            relabeling.at(borderVertices.at(patch.borderLength - i - 1)) = nextFreeLabel++;
        } else {
            reverselabeling.at(nextFreeLabel) = borderVertices.at(i);
            relabeling.at(borderVertices.at(i)) = nextFreeLabel++;;
        }
    }

    list<Vertex> toRelabelQueue;
    // Relabel border
    for (int i = 0; i < patch.borderLength; i++) {
        Vertex relabelThis = borderVertices.at(i);
        Vertex newLabel = relabeling.at(relabelThis);
        Neighbours nb = v.at(relabelThis);
        for (int j = 0; j < 3; j++) {
            if (nb.nb[j] <= MaxVertex) {
                if (relabeling.at(nb.nb[j]) != -1) {
                    nb.nb[j] = relabeling.at(nb.nb[j]);
                } else {
                    Vertex origLabel = nb.nb[j];
                    nb.nb[j] = nextFreeLabel;
                    reverselabeling.at(nextFreeLabel) = origLabel;
                    relabeling.at(origLabel) = nextFreeLabel++;
                    toRelabelQueue.push_back(origLabel);
                }
            }
        }
        // Switch rotation? 012 -> 021
        if(reverseOrder && nb.nb[2] <= MaxVertex) { // [!] Indien het een randtop is met '2' buren, staat deze al in de correcte volgorde!
            Vertex temp = nb.nb[1];
            nb.nb[1] = nb.nb[2];
            nb.nb[2] = temp;
        }
        relabeled.at(newLabel) = nb;
    }

    // Relabel the patch
    while (toRelabelQueue.size() > 0) {
        Vertex relabelThis = toRelabelQueue.front();
        Vertex newLabel = relabeling.at(relabelThis);
        toRelabelQueue.pop_front();
        Neighbours nb = v.at(relabelThis);

        for (int j = 0; j < 3; j++) {
            if (nb.nb[j] <= MaxVertex) {
                if (relabeling.at(nb.nb[j]) != -1) {
                    nb.nb[j] = relabeling.at(nb.nb[j]);
                } else {
                    Vertex origLabel = nb.nb[j];
                    nb.nb[j] = nextFreeLabel;
                    reverselabeling.at(nextFreeLabel) = origLabel;
                    relabeling.at(origLabel) = nextFreeLabel++;
                    toRelabelQueue.push_back(origLabel);
                }
            }
        }
        // Switch rotation? 012 -> 021
        if(reverseOrder) {
            Vertex temp = nb.nb[1];
            nb.nb[1] = nb.nb[2];
            nb.nb[2] = temp;
        }
        relabeled.at(newLabel) = nb;
    }
    assert(nextFreeLabel == v.size());

    // Done
    patch.list = relabeled;
    //cout << computeCanonicalForm(patch, true) << endl;

    return patch;
}

int main(int argc, char** argv) {
    bool no_pairs = false;

    if (argc == 4) {
        if (strcmp(argv[3], "nopairs") == 0) {
            no_pairs = true;
            // HACK
            argc = 3;
        }
    }
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " inputfile outputfile" << endl;
        cout << "Usage: " << argv[0] << " inputfile outputfile nopairs" << endl;
        return -1;
    }

    ifstream in;
    in.open (argv[1], ifstream::in);


    // Get length of the file
    in.seekg (0, ios::end);
    int fileLength = in.tellg();
    in.seekg (0, ios::beg);

    // Skip de header (hopelijk is hij juist, best checken eigk!!!!)
// #define PLANARCODE_NO_LE
#ifdef PLANARCODE_NO_LE
    in.seekg(strlen(">>planar_code<<"), ios_base::beg);
#else
    in.seekg(strlen(">>planar_code le<<"), ios_base::beg);
#endif


    filebuf outbuffer;
    outbuffer.open(argv[2], ios::out | ios::binary);
    ostream out(&outbuffer);

    // ### Betere checks hier
    int processed = 0;

    while (in.tellg() < fileLength) {
        Patch patch1, patch2;
        // Read in a patch pair
        patch1 = inputPlanarCode(in);

#if 1
        if (no_pairs) {
            pair<CanonicalBorder, int> code1 = codeForBorder(patch1.list, 0, 1, patch1.borderLength/* onActualBorderLength */);
#if 1
            // Gunnar's code also outputs unembeddable ones, filter them away!
            if (!borderEncodingIsEmbeddableInFullerene(code1.first, code1.second)) 
                continue;
#endif
            CanonicalForm form1 = computeCanonicalForm(patch1, false);
            out << form1 << endl;
            processed++;
            continue;
        }

        patch2 = inputPlanarCode(in);

        // Compute their canonical forms
        vector<BorderAutoInfo> borderAutomorphisms1, borderAutomorphisms2; // They can be different!
        pair<CanonicalBorder, int> code1 = codeForBorder(patch1.list, 0, 1, patch1.borderLength/* onActualBorderLength */, &borderAutomorphisms1);
        pair<CanonicalBorder, int> code2 = codeForBorder(patch2.list, 0, 1, patch2.borderLength/* onActualBorderLength */, &borderAutomorphisms2);
        assert(code1.first == code2.first);
        assert(code1.first != -1); // ###

#if 1
        // Gunnar's code also outputs unembeddable ones, filter them away!
        if (!borderEncodingIsEmbeddableInFullerene(code1.first, code1.second)) 
            continue; 
#endif

        CanonicalForm form1 = computeCanonicalForm(patch1, false);
        CanonicalForm form2 = computeCanonicalForm(patch2, false);

        // We output in the order smallest|biggest\n:
        if (form1 < form2)
            out << form1 << "|" << form2 << endl;
        else
            out << form2 << "|" << form1 << endl;

        processed++;
#endif
    }

    if (no_pairs)
        cerr << "Processed " << processed << " graphs" << endl;
    else
        cerr << "Processed " << processed << " pair(s)" << endl;
}
