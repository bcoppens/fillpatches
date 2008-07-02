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

#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <unistd.h>

#include "patch.h"
// The data on what is in the file and what is not must be ALWAYS synchronized with this information!
// #define stuff
// Assumes length > 4, borderCode starts with '00'
// Mss soort van informatie inbouwen op begin file.
off_t bitOffsetForBorderCode(CanonicalBorder borderCode, int borderLength);
void addBorderInfoToFile(CanonicalBorder borderCode, int borderLength, CanHaveFilling hasFilling, char* fileMapping, const FdAndMap& fm);
CanHaveFilling borderHasFillingFromFile(CanonicalBorder borderCode, int borderLength, char* fileMapping, const FdAndMap& fm); // true -> has filling
bool isInPreloaded(CanonicalBorder borderCode, int borderLength, const FdAndMap& fm);
CanHaveFilling borderHasFillingFromPreloaded(CanonicalBorder borderCode, int borderLength, const FdAndMap& fm); // true -> has filling

struct FdAndMap {
    int fd;
    char* map;
    char* preloaded;
    off_t size; // ### for fstat...
    size_t preloadsize;
};
typedef enum { Create, Read, ReUse } FileOpenMode; // ReUse assumes file exists
FdAndMap loadCrossProcessFile(const char* file, FileOpenMode mode, int maxBorderLength); // Since the initial borderlength is an upper bound for max, this is OK
void unloadCrossProcessFile(FdAndMap fm);

#endif // FILE_H
