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

#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <limits>
#include <list>
#include <iostream>
#include <utility>
#include <map>

typedef unsigned int Vertex;
const Vertex OutVertex = std::numeric_limits<Vertex>::digits;
const Vertex InVertex = OutVertex - 1;
const Vertex MinVertex = 0;
const Vertex MaxVertex = InVertex - 1;

// rotatie volgorde: klokwijze
struct Neighbours {
    Vertex nb[3];
};
typedef std::vector<Neighbours> VertexVector;

struct Patch {
    Patch() : hexagons(0), pentagons(0), borderLength(0) {}
    VertexVector list;
    int hexagons;
    int pentagons;
    unsigned int borderLength;
};

// length is the number of vertices
// code looks like <neighbours for v0, including OutVertex><v1><...><vlength-1>
struct CanonicalForm {
    CanonicalForm() : length(0), code(0) {}
    CanonicalForm(const CanonicalForm& rhs) { clone(rhs, false); }
    CanonicalForm& operator=(const CanonicalForm& rhs) { clone(rhs, true); return *this; }
    ~CanonicalForm() { delete[] code; }
    int length;
    Vertex* code;
    void clone(const CanonicalForm& rhs, bool dirty);
};

typedef unsigned long CanonicalBorder;

bool operator<(const CanonicalForm& lhs, const CanonicalForm& rhs);
bool operator==(const CanonicalForm& lhs, const CanonicalForm& rhs);
std::ostream& operator<<(std::ostream& os, const CanonicalForm& form);

inline int indexOfVertex(Vertex vertex, const Neighbours& nb) {
    for (int i = 0; i < 3; i++)
        if (nb.nb[i] == vertex)
            return i;
    // Oops! XXX ###
    return -1;
}

inline int indexOfNextVertex(Vertex prev, const Neighbours& nb) {
    for (int i = 0; i < 3; i++)
        if (nb.nb[i] == prev)
            return (i+1)%3;
    // Oops! XXX
    return 0;
}

inline int indexOfPrevVertex(Vertex next, const Neighbours& nb) {
    for (int i = 0; i < 3; i++)
        if (nb.nb[i] == next)
            return (i+2)%3; // ((i+3)-1)%3, because we don't want to underflow (tricky semantics)
    // Oops! XXX
    return 0;
}

//#define DEBUG 1
//#define DEBUG 0
#define DEBUG 0
#define LOCALDEBUG 0

// Changes nb!
inline void get_next_real_neighbour(Vertex prev, Vertex current, Vertex& next,
                                    const VertexVector& list) {
    Neighbours nb = list.at(current);
    int nextIndex = indexOfNextVertex(prev, list.at(current));
    next = nb.nb[nextIndex];
    if (next > MaxVertex) {
        next = nb.nb[(nextIndex + 1) % 3];
    }
}

// XXX Hier get_next_real_neighbour in gebruiken? ### die break!
#define begin_neighbour_iteration(prev, current, next, nb, list, nextIndex) \
    do { \
        if (DEBUG || LOCALDEBUG) cout << prev << ">" << current << ">" << next << endl; \
        prev = current; \
        current = next; \
        if (current > MaxVertex) { cout << "CURRENT>MAX!!!" << endl; break; }\
        nb = list.at(current); \
        \
        unsigned int nextIndex = indexOfNextVertex(prev, nb); \
        next = nb.nb[nextIndex];

// XXX Maak die if optioneel???
#define end_neighbour_iteration(start, current, next, nb, nextIndex) \
        if (next > MaxVertex) { \
            next = nb.nb[(nextIndex + 1) % 3]; \
            assert(next <= MaxVertex); \
        } \
    } while (current != start);

// XXX mss die debugStream en debuggedCount optioneel?
#define debug_graph(list) \
    if (debugStream) { \
        outputWriteGraph2D(list, *debugStream); \
    } else { \
        cout << "Graph " << debuggedCount++ << " follows" << endl; \
    }

int numberOfPentagonsInducedByBorder(Vertex start, Vertex direction, const VertexVector& list);
int computeHexagonsMax(Vertex startPos, Vertex direction, VertexVector& list, int pentagonsNow);
std::vector< std::vector<short int> > toAdjacencyMatrix(const VertexVector& list);

// .first = startpos, .second = direction
std::pair<Vertex, Vertex>
        findStartPos(const VertexVector& list, Vertex startPos, Vertex direction);

typedef enum { BeforeFilling, InFilling } AddMode;
// -1 = unknown, -2 = indeterminable
std::vector<Patch> tryAddNGonToBorder(AddMode mode, Patch& patch, Vertex startPos, Vertex direction,
                                      int hexagonsMax = -1, int pentagonsMax = -1);
bool isNGon(int n, const VertexVector& list, Vertex startPos, Vertex direction);
int nGon(const VertexVector& list, Vertex startPos, Vertex direction, bool* hasInVertex = 0);
bool pentagonsConditionMayContinue(/*VertexVector& list, Vertex startPos, Vertex direction, */int pentagonsNow);
std::vector<Patch> addPentagonToBorderAndFill(Patch& patch, Vertex startPos, Vertex direction);

struct PatchAutoInfo {
    PatchAutoInfo(const CanonicalForm& c, Vertex s, Vertex t, int d) : form(c), start(s), to(t), direction(d) {}
    CanonicalForm form;
    Vertex start;
    Vertex to;
    int direction;
};
struct BorderAutoInfo {
    BorderAutoInfo(Vertex s, Vertex t, int d) : start(s), to(t), direction(d) {}
    Vertex start;
    Vertex to;
    int direction;
};
CanonicalForm computeCanonicalForm(const Patch& patch, bool d=false /*debug*/, std::vector<PatchAutoInfo>* autoMorphisms = 0);


struct BorderInformation {
    int twos;
    int threes;
    int length; /* Maybe make it an inline function? */
    int pentagons; /* return 6 - twos + threes; */
    CanonicalBorder borderCode;
    bool canBeQueried; /* ditto */
    Vertex startPos;
    Vertex direction;
};

// !!! Automorphisms only makes sense when the border is the border of the PATCH!
BorderInformation analyzeBorder(const VertexVector& list, Vertex startPos, Vertex direction, int onActualBorderLength = 0, std::vector<BorderAutoInfo>* automorphisms = 0);
void assertAnalyzedInformationOK(const BorderInformation& info, const VertexVector& list, Vertex startPos, Vertex direction, int onActualBorderLength = 0);



Vertex addVertexToList(VertexVector& list); // Adds vertex, and returns the added Vertex (does not connect it)
//void removeLastFromList(VertexVector& list, Vertex connectTo, Vertex direction);

VertexVector readBorder(std::istream& is);
std::istream& operator>>(std::istream& is, VertexVector& v);
std::ostream& operator<<(std::ostream& os, const VertexVector& vector);
void outputWriteGraph2D(const VertexVector& list, std::ostream& out, bool addInVertex = false, bool addOne = true);
void outputPlanarCode(const VertexVector& list, std::ostream& out);
void addFinishedGraph(VertexVector& list);


// first = code, second = code length; code == -1 -> zal niet toegelaten worden (kan sowieso niet, een (unsigned long)(-1) == 1111111111, te weinig 2 om in te vullen
enum CanHaveFilling { CanHaveYes, CanHaveNo, CanHaveMaybe };
// Ingevulde rand zal alleen werken als de patch de volledige patch is! Dat is, als de eerste borderLength toppen van de list de rand bepalen, en alleen die!
std::pair<CanonicalBorder, int> codeForBorder(const VertexVector& list, Vertex startPos, Vertex direction, int onActualBorderLength = 0, std::vector<BorderAutoInfo>* automorphisms = 0); // > 0 -> is border of filled patch, automorphisms -> if non-0 will clear and then append a list of automorphisms of the border
CanHaveFilling canBorderHaveFilling(CanonicalBorder borderCode, int borderLength);
void addBorderCanHaveFilling(CanonicalBorder borderCode, int borderLength, CanHaveFilling can);

extern std::ostream* debugStream; // Tijdelijk meer grafen uitvoeren via deze stream
extern std::ostream* finishedStream; // Voltooide grafen gaan hier in
extern int debuggedCount; // Zodat ik kan volgen in de output
extern bool hexagonDEBUG;

// Cross-process dynamic stuff
struct FdAndMap;
extern FdAndMap dynamicFDStuff;

//inline int borderFillingMaxLen() { return 10; }
//inline int borderFillingMinLen() { return 10; }
//#define BorderFillingMaxLen 31
#define BorderFillingMaxLen 55
#define BorderFillingMinLen 6
//extern std::map<int, bool> borderFillingMap[BorderFillingMaxLen - BorderFillingMinLen]; // bool ignored: contains() -> geen invulling //bool: false -> geen invulling

void printCycle(Vertex current, Vertex next, const VertexVector& list);

void printBorderFillingInfo();

namespace {
// Default = 'long(64)'
template<typename T> void coutBitString(T bits) {
    for (int i = 63; i >= 0; i--)
        std::cout << ((bits >> i) & 1);
    std::cout << std::endl;
}

template<> void coutBitString(char bits) {
    for (int i = 7; i >= 0; i--)
        std::cout << ((bits >> i) & 1);
    std::cout << std::endl;
}
}

#endif // GRAPH_H
