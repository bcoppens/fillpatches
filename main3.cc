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
#include <streambuf>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>
#include <list>
#include <ctime>
#include <cassert>
#include <string.h> // debug
#include "patch.h"
#include "growthpairs.h"
#include "file.h"

using namespace std;

#define OUTPUT_METHOD_COMPARE_GUNNAR

ostream* debugStream = 0;
ostream* finishedStream = 0;

bool time_generation = false;
bool filter_unembeddable = false;
unsigned long long patches_generated = 0;
unsigned long long time_taken = 0;

inline int twosNeededForPentagon(int threes, int pentagons) {
    // v_2-v_3=6-p, so v_2 = 6 - p + v_3
    return 6 - pentagons + threes;
}

struct ProcessBorderStack {
    typedef enum { GrowthPair, IsomerisationPair, IsomerisationPatch, OutputAll, None } OutputType;

    ProcessBorderStack(ostream& o, set<CanonicalBorder>& cBS, set<CanonicalForm>& gP, int l, int p, OutputType t)
        : out(o), canonicalBordersSeen(cBS), generatedPatches(gP), length(l), pentagons(p), outputType(t) {}
    ostream& out;
    set<CanonicalBorder>& canonicalBordersSeen;
    set<CanonicalForm>& generatedPatches;
    int length;
    int pentagons;
    OutputType outputType;
};
struct CreateStringsStack {
    CreateStringsStack(int l) : length(l), shouldPushStartPos(false), consecutiveThrees(0) {}
    int length;
    list<int> startPosVector;
    vector<int> startPosVectorReverse;
    bool shouldPushStartPos;
    int consecutiveThrees;
};

bool isCanonicalSpecialized(const char* const string, int length, list<int>& startPosVector, vector<int>& startPosVectorReverse) {
    // Forward
    list<int>::iterator end = startPosVector.end();
    for (list<int>::iterator it = startPosVector.begin(); it != end; ++it) {
        int offSet = *it;
        char forwardChar, normalChar;
        assert(offSet < length);
        for (int i = 0; i < length; i++) {
            forwardChar = string[(offSet + i) % length];
            normalChar = string[i];
            if (forwardChar < normalChar)
                return false;
            if (forwardChar > normalChar)
                break;
        }
    }

    // Backward
    vector<int>::iterator vEnd = startPosVectorReverse.end();
    for (vector<int>::iterator it = startPosVectorReverse.begin(); it != vEnd; ++it) {
        int offSet = *it;
    // ###
    //for (int offSet = 0; offSet < length; offSet++) {
        //assert(offSet > 0);
        assert(offSet < length);
        char reverseChar, normalChar;
        for (int i = 0; i < length; i++) {
            reverseChar = string[(offSet - i + 2*length) % length];
            normalChar = string[i];
            if (reverseChar < normalChar) {
                return false;
            }
            if (reverseChar > normalChar) {
                break;
            }
        }
    }

    return true;
}

void processBorder(const char* string, ProcessBorderStack& stack);

//#define VANILLA_RECURSION
#ifdef VANILLA_RECURSION
void createStrings(int twosToPlace, int threesToPlace, char* totalString, char* nextPlacePos, ProcessBorderStack& stack, CreateStringsStack& cstack, int consecutiveTwos = 0) {
    if ((cstack.consecutiveThrees >= 5)) // Dit werkt natuurlijk niet volledig, omwille van geen canonische vorm
        return;

    if (threesToPlace == 0) {
        if (twosToPlace == 0) {
            processBorder(totalString, stack);
            return;
        }
    }

    if (twosToPlace > 0) {
        *nextPlacePos = '2';
        int ct = cstack.consecutiveThrees;
        cstack.consecutiveThrees = 0;
        createStrings(twosToPlace - 1, threesToPlace, totalString, nextPlacePos + 1, stack, cstack, consecutiveTwos + 1);
        cstack.consecutiveThrees = ct;
    }
    if (threesToPlace > 0) {
        *nextPlacePos = '3';
        cstack.consecutiveThrees++;
        createStrings(twosToPlace, threesToPlace - 1, totalString, nextPlacePos + 1, stack, cstack, 0);
        cstack.consecutiveThrees--;
    }
}
#else

void processBorderSpecialized(const char* string, ProcessBorderStack& stack, CreateStringsStack& cstack) {
    if (isCanonicalSpecialized(string, stack.length, cstack.startPosVector, cstack.startPosVectorReverse)) {
        processBorder(string, stack);
    }
}

void createStrings(int twosToPlace, int threesToPlace, char* totalString, char* nextPlacePos, ProcessBorderStack& stack, CreateStringsStack& cstack, int consecutiveTwos = 0) {
    if ((cstack.consecutiveThrees >= 5))
        return;
    if ((consecutiveTwos > 6) || ((cstack.length > 6) && (consecutiveTwos > 4))) // Filter out '2^5' ###
        return;

#define USE_STARTPOSSES
#ifdef USE_STARTPOSSES
    // Only allow 'canonical' ones
    list<int>& startPosses = cstack.startPosVector; // Short-hand, basically
    list<int>::iterator end = startPosses.end();
    for (list<int>::iterator it = startPosses.begin(); it != end; ++it) {
        const int currentStartPos = *it;
        //assert(nextPlacePos + (-1-currentStartPos) >= totalString);
        char compChar = nextPlacePos[-1-currentStartPos]; // IEW
        //assert(nextPlacePos > totalString);
        char curChar = nextPlacePos[-1]; // IEW
        assert(compChar == '2' || compChar == '3');
        assert(curChar == '2' || curChar == '3');
        if (curChar < compChar) {
            return;
        }
        if (curChar > compChar) {
            it = --(startPosses.erase(it));
        }
    }
#endif
#define USE_REVERSESTARTPOSSES
#ifdef USE_REVERSESTARTPOSSES
    char* normalString = totalString;
    char* reverseString = nextPlacePos - 1;
    while (reverseString >= totalString) {
        char compChar = *normalString;
        char reverseChar = *reverseString;
        if (reverseChar < compChar) {
            return;
        }
        if (reverseChar > compChar) {
            break;
        }

        normalString++;
        reverseString--;
    }
    //assert(reverseString == string);
    //assert(normalString == nextPlacePos - 1);
    /// ### DIT DUS
    if (reverseString == totalString - 1)
        cstack.startPosVectorReverse.push_back(nextPlacePos - totalString - 1);
#endif
    if (threesToPlace == 0) { // ### Dit voor, of na die for zetten?
        if (twosToPlace == 0) {
            // threesToPlace == 0 -> End of recursion
            processBorderSpecialized(totalString, stack, cstack); // ###
            return;
        }
        // Doesn't end with a '3', so no canonical border
        return;
    }

    if (twosToPlace > 0) {
        *nextPlacePos = '2';
        int ct = cstack.consecutiveThrees;
        cstack.consecutiveThrees = 0;
        if (cstack.shouldPushStartPos) {
            int possibleStartPos = nextPlacePos - totalString;
            cstack.startPosVector.push_back(possibleStartPos);
            cstack.shouldPushStartPos = false;
            createStrings(twosToPlace - 1, threesToPlace, totalString, nextPlacePos + 1, stack, cstack, consecutiveTwos + 1);
            if (cstack.startPosVector.size() > 0 && cstack.startPosVector.back() == possibleStartPos)
                cstack.startPosVector.pop_back();
        } else {
            cstack.shouldPushStartPos = false;
            createStrings(twosToPlace - 1, threesToPlace, totalString, nextPlacePos + 1, stack, cstack, consecutiveTwos + 1);
        }
        cstack.consecutiveThrees = ct;
    }
    if (threesToPlace > 0 && (nextPlacePos != totalString) && (nextPlacePos != totalString + 1)) { // Don't place a '3' at the first two positions ###
        *nextPlacePos = '3';
        cstack.consecutiveThrees++;
        cstack.shouldPushStartPos = true;
        createStrings(twosToPlace, threesToPlace - 1, totalString, nextPlacePos + 1, stack, cstack, 0);
        cstack.consecutiveThrees--;
    }

#ifdef USE_REVERSESTARTPOSSES
    if (reverseString == totalString - 1)
        cstack.startPosVectorReverse.pop_back();
#endif
}
#endif

void processBorder(const char* string, ProcessBorderStack& stack)
{
    Patch patch;
    VertexVector v;
    stringstream s(string);
    s >> v;
    patch.borderLength = stack.length;

    vector<BorderAutoInfo> borderAutomorphisms;
    BorderInformation info = analyzeBorder(v, 0, 1, 0/* onActualBorderLength */, &borderAutomorphisms);

    if (info.canBeQueried) {
        assert(info.length == stack.length);
        if (stack.canonicalBordersSeen.find(info.borderCode) != stack.canonicalBordersSeen.end()) {
            return;
        }
        stack.canonicalBordersSeen.insert(info.borderCode);

        // Remove unembeddable in fullerene
        if (filter_unembeddable && (!borderEncodingIsEmbeddableInFullerene(info.borderCode, info.length)))
            return;
    }

    if (patch.borderLength > 8*sizeof(CanonicalBorder)) {
        cerr << "Warning: Border length of the border is greater than allowed by the border code!" << endl;
        cerr << " (" << patch.borderLength << " > " << (8*sizeof(CanonicalBorder)) << endl;
    }

    patch.list = v;
    clock_t start = clock();

    vector<Patch> l;
    if (stack.pentagons < 6) {
        l = tryAddNGonToBorder(BeforeFilling, patch, 0, 1);
    } else if (stack.pentagons == 6) {
        l = addPentagonToBorderAndFill(patch, 0, 1);
    } else {
        assert(false);
    }
    clock_t end = clock();
    time_taken += end - start;

    patches_generated += l.size();

    if (stack.outputType == ProcessBorderStack::None) { // Do nothing! :-)
        return;
    } else if(stack.outputType == ProcessBorderStack::OutputAll) {
        vector<Patch> outputtable;

        for (uint i = 0; i < l.size(); i++) {
            // Isomorfism rejection
            CanonicalForm f = computeCanonicalForm(l.at(i));
            if (stack.generatedPatches.find(f) == stack.generatedPatches.end()) {
                stack.generatedPatches.insert(f);
                outputtable.push_back(l.at(i));
            }
        }

        for (uint i = 0; i < outputtable.size(); i++) {
            outputPlanarCode(outputtable.at(i).list, stack.out);
        }
        cerr << "Non-Isomorphic: " << outputtable.size() << endl;
    } else if (stack.outputType == ProcessBorderStack::IsomerisationPatch) {
        // Isomerisation patches
        if (borderAutomorphisms.size() > 1) {
            // At least 2 automorphisms, check each patch that has _less_ automorphisms
            for (uint i = 0; i < l.size(); i++) {
                vector<PatchAutoInfo> patchAutomorphisms;
                // ### Hier moeten we gebruik maken van het feit dat we reeds de automorfismen van de rand hebben!
                CanonicalForm f = computeCanonicalForm(l.at(i), false /*debug*/, &patchAutomorphisms);
                if (patchAutomorphisms.size() < borderAutomorphisms.size()) {
                    if (stack.generatedPatches.find(f) == stack.generatedPatches.end()) {
                        stack.generatedPatches.insert(f);

                        // Check irreducable (for automorphisms, so check against itself
                        if (isIrreducibleGrowthPair(l.at(i), l.at(i))) {
                            // Output! It is an irreducible isomerization patch
                            outputPlanarCode(l.at(i).list, stack.out);
                            // Naming scheme and output debug
                            cout << "(" << stack.length << "," << l.at(i).list.size() << ","
                                    << borderAutomorphisms.size() << "/" << patchAutomorphisms.size() << ")" << endl;
                        }
                    }
                }
            }
        }
    } else { // Pairs
        // Isomerisation pairs (mutually excludes isomerization code pathway)
        vector<Patch> outputtable;

        for (uint i = 0; i < l.size(); i++) {
            // Isomorfism rejection
            CanonicalForm f = computeCanonicalForm(l.at(i));
            if (stack.generatedPatches.find(f) == stack.generatedPatches.end()) {
                stack.generatedPatches.insert(f);
                outputtable.push_back(l.at(i));
            }
        }
        if (outputtable.size() > 1) {
            for (uint i = 0; i < outputtable.size(); i++) {
                for (uint j = i + 1; j < outputtable.size(); j++) { // j = i + 1, so that we don't get doubles (and don't test ourselves)
                    if (stack.outputType == ProcessBorderStack::GrowthPair) {
                        if (outputtable.at(i).list.size() == outputtable.at(j).list.size()) // Not a growth pair
                        continue;
                    } else if (stack.outputType == ProcessBorderStack::IsomerisationPair) {
                        // Only isomerization pairs
                        if (outputtable.at(i).list.size() != outputtable.at(j).list.size())
                            continue;
                    } else {
                        assert(0);
                    }
                    // Difference with growth patches catalogue!!: if there are three isomeric patches, output 2 pairs instead of a single list!
                    if (isIrreducibleGrowthPair(outputtable.at(i), outputtable.at(j))) { // 'Growth' pair naming scheme is bad here, in case of isomerisation...
                        outputPlanarCode(outputtable.at(i).list, stack.out);
                        outputPlanarCode(outputtable.at(j).list, stack.out);
                        cout << "(" << stack.length << "," << outputtable.at(i).list.size() << ")" << endl;
#ifndef OUTPUT_METHOD_COMPARE_GUNNAR
                        // Visual method to see the difference between two sequences
                        VertexVector temp;
                        Neighbours n;
                        n.nb[0] = 2;
                        n.nb[1] = OutVertex;
                        n.nb[2] = 1;
                        temp.push_back(n); // 0
                        n.nb[0] = 0;
                        n.nb[1] = OutVertex;
                        n.nb[2] = 2;
                        temp.push_back(n); // 1
                        n.nb[0] = 1;
                        n.nb[1] = OutVertex;
                        n.nb[2] = 0;
                        temp.push_back(n); // 2
                        outputPlanarCode(temp, stack.out);
#endif
                    }
                }
            }
        }
    }
}

// Crossprocess code
FdAndMap dynamicFDStuff;

void usage(const string& progname) {
    cout << "Usage:" << endl;
    cout << progname << " -borders <bordermode> -outputmethod <outputmode> [opts]" << endl;
    cout << " -borders <bordermode> selects the way the program will find the borders to fill." << endl;
    cout << "   <bordermode> is one of:" << endl;
    cout << "      generatelist: generates its own list of potentially non-isomorphic borders" << endl;
    cout << "                    that satisfy the criteria from the REQUIRED parameters -pentagons, -minlength, -maxlength" << endl;
    cout << "      borders_01_k: fills a border of the form (01)^k, where k is set by '-k <k>'" << endl;
    cout << "      borders_0_01_k: fills a border of the form 0(01)^k, where k is set by '-k <k>'" << endl;
    cout << "      stdin: reads the list of borders from standard input"<< endl;
    cout << "      param: reads a single border to fill from the REQUIRED parameter -border" << endl;
    cout << " -outputmethod <outputmode> sets what will be written to file:" << endl;
    cout << "    <outputmode> is one of:" << endl;
    cout << "      pairs_out: will output each isomerisation/growth pair found as 2 subsequent patches." << endl;
    cout << "                 This REQUIRES you to set a pair selection method with -pairs <pairmethod>" << endl;
    cout << "      isopatches: will output all isomerisation patches" << endl;
    cout << "      patches_out: just output all patches found" << endl;
    cout << "      none: fill all borders, but don't do anything with the result (handy in combination with -time)" << endl;
    cout << " -pairs <pairmethod> is to chose which kind of pairs will be outputted," << endl;
    cout << "    <pairmethod> is one of:" << endl;
    cout << "      growth: output the growth patches (NOTICE: outputs information on standard output)" << endl;
    cout << "      iso: output the isomerisation patches (NOTICE: outputs information on standard output)" << endl;
    cout << " -time: when filling all borders, keep time information about that, and output this at program exit on stderr" << endl;
    cout << " -border <bordercode> sets the border to be filled with -borders param. Should consist of 2s and 3s" << endl;
    cout << " -k <k> sets the parameter 'k' for borders like 0(01)^k and (01)^k" << endl;
    cout << " -o <filename> sets an outputfile for the filled patches (is standard output if not specified)" << endl;
    cout << " -pentagons <p> sets the number of pentagons that all the generated borders should have" << endl;
    cout << " -minlength <l> sets the minimum length of the generated borders" << endl;
    cout << " -maxlength <l> sets the maximum length of the generated borders (inclusive)" << endl;
    cout << " -filterunembeddable filters away patches/borders that are 'obviously' not embeddable in a fullerene" << endl;
}

int main(int argc, char** argv) {
    Patch patch;
    int minLength = 0, maxLength = 4, pentagons = 0, k = -1;
    filebuf buffer;
    ProcessBorderStack::OutputType type = ProcessBorderStack::None;
    bool use_stdout = true; // By default use standard output, unless -o is specified
    bool use_stdin = false;
    enum { K_0_01, K_01, K_None } k_mode = K_None;
    string paramborder("");
    bool minSet = false, maxSet = false, pSet = false;
    bool sawBorders = false, sawOutput = false;

    string progname(argv[0]);
    argv++; // skip argv[0]
    for ( ; argc > 0 && *argv; --argc, ++argv) {
        string opt(*argv);
        if (opt == "-borders") {
            // Read bordermode:
            sawBorders = true;
            ++argv; --argc;
            string mode(*argv);
            if (mode == "generatelist") {
                ;
            } else if (mode == "borders_0_01_k") {
                k_mode = K_0_01;
            } else if (mode == "borders_01_k") {
                k_mode = K_01;
            } else if (mode == "stdin") {
                use_stdin = true;
            } else if (mode == "param") {
                // Is set by using -border...
            } else {
                usage(progname);
                return -1;
            }
        } else if (opt == "-outputmethod") {
            // Read outputmode:
            sawOutput = true;
            ++argv; --argc;
            string mode(*argv);
            if (mode == "pairs_out") {
                // Mode will be selected in -pairs...
            } else if (mode == "patches_out") {
                type = ProcessBorderStack::OutputAll;
            } else if (mode == "isopatches") {
                type = ProcessBorderStack::IsomerisationPatch;
            } else if (mode == "none") {
                type = ProcessBorderStack::None;
            } else {
                usage(progname);
                return -1;
            }
        } else if (opt == "-pairs") {
            // Read pairmethod:
            ++argv; --argc;
            string mode(*argv);
            if (mode == "growth") {
                type = ProcessBorderStack::GrowthPair;
            } else if (mode == "iso") {
                type = ProcessBorderStack::IsomerisationPair;
            } else {
                usage(progname);
                return -1;
            }
        } else if (opt == "-time") {
            time_generation = true;
        } else if (opt == "-border") {
            ++argv; --argc;
            paramborder = *argv;
            if (paramborder == "") {
                cout << "Expected a border!" << endl;
                usage(progname);
                return -1;
            }
            uint len = paramborder.length();
            for (int i = 0; i < len; i++) {
                if (paramborder[i] != '2' && paramborder[i] != '3') {
                    cout << "Expected a string of 2s and 3s as border!" << endl;
                    usage(progname);
                    return -1;
                }
            }
        } else if (opt == "-k") {
            ++argv; --argc;
            if (k_mode == K_01) {
                // Borders (01)^k
                k = atoi(*argv);
                minLength = maxLength = 2*k;
                pentagons = 6;
            } else if (k_mode == K_0_01) {
                // Borders 0(01)^k
                k = atoi(argv[2]);
                minLength = maxLength = 2*k+1;
                pentagons = 5;
            } else {
                cout << "-k needs to have -borders set (first) to borders_0_01_k or borders_01_k" << endl;
                usage(progname);
                return -1;
            }
        } else if (opt == "-o") {
            // Read filename
            ++argv; --argc;
            buffer.open(*argv, ios::out);
            use_stdout = false;
        } else if (opt == "-pentagons") {
            ++argv; --argc;
            pentagons = atoi(*argv);
            pSet = true;
        } else if (opt == "-minlength") {
            ++argv; --argc;
            minLength = atoi(*argv);
            minSet = true;
        } else if (opt == "-maxlength") {
            ++argv; --argc;
            maxLength = atoi(*argv);
            maxSet = true;
        } else if (opt == "-filterunembeddable") {
            filter_unembeddable = true;
        } else {
            usage(progname);
            return -1;
        }
    }

    if (!sawBorders || !sawOutput) {
        usage(progname);
        return -1;
    }

    // Crossprocess code
    dynamicFDStuff = loadCrossProcessFile("./main3__dynamic__programming__crossprocess", ReUse, maxLength);

    //debugStream = &out;
    //finishedStream = &out;

    ostream* out_ = 0;
    if (use_stdout)
        out_ = &cout;
    else
        out_ = new ostream(&buffer);
    ostream& out = *out_;
    out << ">>planar_code le<<";

    if (use_stdin || paramborder != "") {
        while ((!cin.eof() && cin.good())  || paramborder != "") {
            string border;
            if (paramborder != "")
                border = paramborder;
            else
                cin >> border;

            // For this length
            set<CanonicalBorder> canonicalBordersSeen;
            set<CanonicalForm> generatedPatches;

            generatedPatches.clear();

            int length = border.length();

            patch.borderLength = length;

            int twos = 0;
            int threes = 0;
            int pentagons = 0;

            for (int i = 0; i < length; i++) {
                if (border[i] == '2')
                    twos++;
                if (border[i] == '3')
                    threes++;
            }

            pentagons = 6 - twos + threes;

            if (((length+pentagons) % 2 != 0) || length == 0) {
                continue;
            }

            cerr << "Next length: " << length << " p = " << pentagons << endl;

            ProcessBorderStack pbs(out, canonicalBordersSeen, generatedPatches, length, pentagons, type);

            processBorder(border.c_str(), pbs);

            if (paramborder != "")
                break;
        }
    } else {
        if (!pSet || !minSet || !maxSet) {
            cout << "Need to have pentagons, minLength and maxLength set!" << endl;
            usage(progname);
            return -1;
        }
        for (int length = minLength; length <= maxLength; length++) {
            if ((length+pentagons) % 2 != 0) {
                continue;
            }
            int twos = (length-pentagons)/2+3;
            int threes = (length+pentagons)/2-3;
            // For this length
            set<CanonicalBorder> canonicalBordersSeen;
            set<CanonicalForm> generatedPatches;

            generatedPatches.clear();
            patch.borderLength = length;
            char* string = new char[length+1];
            string[length] = '\0';
            cerr << "Next length: " << length << endl;

            ProcessBorderStack pbs(out, canonicalBordersSeen, generatedPatches, length, pentagons, type);

            if (k != -1) { // (01)^k or 0(01)^k
                for (int i = 0; i < k; i++) {
                    string[2*i] = '2';
                    string[2*i+1] = '3';
                }
                if (length % 2 == 1) {
                    // 0(01)^k
                    string[2*k] = '2';
                }
                cerr << string << endl;
                processBorder(string, pbs);
            } else {
                CreateStringsStack cstack(length);
                createStrings(twos, threes, string, string, pbs, cstack);
            }

            delete[] string;
        }
    }

    // Crossprocess code
    unloadCrossProcessFile(dynamicFDStuff);

    printBorderFillingInfo();

    if (time_generation) {
        cerr << "Generated " << patches_generated << " patches" << endl;
        cerr << "Internal generating took " << (double(time_taken)/double(CLOCKS_PER_SEC)) << "s" << endl;
    }

    if (!use_stdout)
        delete out_;
}

