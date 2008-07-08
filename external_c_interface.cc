/*
    This file is part of a program to fill borders of patches.
    Copyright (C) 2008 Bart Coppens <kde@bartcoppens.be>

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
#include <algorithm>
#include <set>
#include <list>
#include <ctime>
#include <cassert>
#include <string.h>
#include "patch.h"
#include "file.h"


#include "external_c_interface.h"

using namespace std;

ostream* debugStream = 0;
ostream* finishedStream = 0;
// Crossprocess code
FdAndMap dynamicFDStuff;

struct Initializer {
    Initializer() {
        // Crossprocess code
        dynamicFDStuff = loadCrossProcessFile("./main3__dynamic__programming__crossprocess", ReUse, /*maxLength ### */ 20);
    }

    ~Initializer() {
        // Crossprocess code
        unloadCrossProcessFile(dynamicFDStuff);
    }
};

static Initializer init;

int number_of_fillings(const char* borderCode) {
    set<CanonicalForm> generatedPatches;
    Patch patch;
    VertexVector v;
    stringstream s(borderCode);
    s >> v;

    BorderInformation info = analyzeBorder(v, 0, 1, 0/* onActualBorderLength */, 0 /*borderAutomorphisms*/);
    patch.borderLength = info.length;

    if (patch.borderLength > 8*sizeof(CanonicalBorder)) {
        cerr << "Warning: Border length of the border is greater than allowed by the border code!" << endl;
        cerr << " (" << patch.borderLength << " > " << (8*sizeof(CanonicalBorder)) << endl;
        return -1;
    }

    patch.list = v;


    // Fill
    vector<Patch> l;
    if (info.pentagons < 6) {
        l = tryAddNGonToBorder(BeforeFilling, patch, 0, 1);
    } else if (info.pentagons == 6) {
        l = addPentagonToBorderAndFill(patch, 0, 1);
    } else {
        assert(false);
    }

    // Filter isomorphic
    int outputtable = 0;

    for (uint i = 0; i < l.size(); i++) {
        // Isomorfism rejection
        CanonicalForm f = computeCanonicalForm(l.at(i));
        if (generatedPatches.find(f) == generatedPatches.end()) {
            generatedPatches.insert(f);
            outputtable++;
        }
    }

    return outputtable;
}
