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
#include <cassert>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>

#include "file.h"

using namespace std;

//#define PRELOAD
#define PRELOAD_MEGS 64
#define PRELOAD_BYTES (1024*1024*PRELOAD_MEGS)

off_t bitOffsetForBorderCode(CanonicalBorder borderCode, int borderLength) {
    assert(borderCode);
    assert(borderCode != -1);
    //coutBitString(borderCode);
    //coutBitString(borderCode >> (borderLength - 2));
    assert((borderCode >> (borderLength - 2)) == 0);
    assert(((borderCode >> (borderLength - 2)) & 3) == 0);
    assert(borderLength > 4);
    //if ((borderCode & 1) == 0)
    //    coutBitString(borderCode);
    assert((borderCode & 1) == 1);

    // This is in BIT positions! Could potentially overflow *haha* ;-)
    off_t startOfThisRun = (long(1)<<(borderLength - long(1))) - long(3); // 2^{l+1} - 2 (starts with '00') - 1 (ends with '1')
    off_t bitPosition = startOfThisRun + (borderCode >> long(1)); // ends with '1'

    assert(bitPosition >= startOfThisRun);
    assert(bitPosition < ((long(1)<<(borderLength)) - long(3)));

    return bitPosition;
}

void addBorderInfoToFile(CanonicalBorder borderCode, int borderLength, CanHaveFilling hasFilling, char* fileMapping, const FdAndMap& fm) {
    // Safety
    //assert(borderHasFillingFromFile(borderCode, borderLength, fileMapping, fm) == CanHaveMaybe); //CanHaveNo);

    if (hasFilling != CanHaveNo)
        return;

    if (borderCode == 0) // Dit is een ZESHOEK! Negeer informatie
        return;

    off_t bitOffset = bitOffsetForBorderCode(borderCode, borderLength);
    off_t byteOffset = bitOffset / 8;
    off_t bitIndex = 7 - (bitOffset % 8); // We want border == 0 ==> position 0 at 0 -> bitShift == 7

    // Terug omhoog moven!
#ifdef PRELOAD
    if (fm.preloaded && (byteOffset < fm.preloadsize)) {
        fm.preloaded[byteOffset] = fm.preloaded[byteOffset] | (long(1) << bitIndex);
    }
#endif // BEIDE DOEN! Synch houden

    fileMapping[byteOffset] = fileMapping[byteOffset] | (long(1) << bitIndex);
    // assert(fileMapping[byteOffset]);
}

CanHaveFilling borderHasFillingFromFile(CanonicalBorder borderCode, int borderLength, char* fileMapping, const FdAndMap& fm) {
    if (borderCode == 0) {
        // Special case: border = 2^n. Only true if hexagon: n == 5 or n == 6
        if ((borderLength == 5) || (borderLength == 6))
            return CanHaveYes;
        return CanHaveNo;
    }

    off_t bitOffset = bitOffsetForBorderCode(borderCode, borderLength);
    off_t byteOffset = bitOffset / 8;
    off_t bitIndex = 7 - (bitOffset % 8); // We want border == 0 ==> position 0 at 0 -> bitShift == 7


#ifdef PRELOAD
    if (fm.preloaded && (byteOffset < fm.preloadsize)) {
        if ((fm.preloaded[byteOffset] & (1 << bitIndex)) == 0x0)
            return CanHaveMaybe;
        return CanHaveNo;
    }
#endif

    if ((fileMapping[byteOffset] & (1 << bitIndex)) == 0x0)
        return CanHaveMaybe; // Can't return CanHaveYes, since we can't make the important distinction between the two
    return CanHaveNo;
}

bool isInPreloaded(CanonicalBorder borderCode, int borderLength, const FdAndMap& fm) {
#ifdef PRELOAD
    if (borderCode == 0)
        return true;
    off_t bitOffset = bitOffsetForBorderCode(borderCode, borderLength);
    off_t byteOffset = bitOffset / 8;
    return (fm.preloaded && (byteOffset < fm.preloadsize));
#else
    return false;
#endif // Preload
}


CanHaveFilling borderHasFillingFromPreloaded(CanonicalBorder borderCode, int borderLength, const FdAndMap& fm) { // true -> has filling
    if (borderCode == 0) {
        // Special case: border = 2^n. Only true if hexagon: n == 5 or n == 6
        if ((borderLength == 5) || (borderLength == 6))
            return CanHaveYes;
        return CanHaveNo;
    }

#ifdef PRELOAD
    off_t bitOffset = bitOffsetForBorderCode(borderCode, borderLength);
    off_t byteOffset = bitOffset / 8;
    off_t bitIndex = 7 - (bitOffset % 8); // We want border == 0 ==> position 0 at 0 -> bitShift == 7

    if (fm.preloaded && (byteOffset < fm.preloadsize)) {
        if ((fm.preloaded[byteOffset] & (1 << bitIndex)) == 0x0)
            return CanHaveMaybe;
        return CanHaveNo;
    }

#endif // Preload
    return CanHaveMaybe; // Indien buiten preload valt -> goed antwoord
}

static int createCrossProcessFile(const char* file, int size) {
    int fd = open(file, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU); // No O_WRONLY because we mmap MAP_PRIVATE
    ftruncate(fd, size); // + 1?
    return fd;
}

FdAndMap loadCrossProcessFile(const char* file, FileOpenMode mode, int maxBorderLength) {
    FdAndMap fm;
    long pagesize = sysconf(_SC_PAGESIZE);
    fm.size = 1 << (maxBorderLength - 3); // 2^{l+1} - 2 with '00', and we complete the final one, so actually we want the size of (l+1) - 1 from ends on 1
    fm.size = fm.size + pagesize - (fm.size % pagesize);
    cerr << "Crossprocess filesize: " << fm.size << endl;

    if (mode == Create) {
        fm.fd = createCrossProcessFile(file, fm.size);
        fm.map = (char*) mmap(0, fm.size, PROT_READ | PROT_WRITE, MAP_SHARED, fm.fd, 0);
    } else if (mode == ReUse) {
        fm.fd = open(file, O_RDWR);
        struct stat buf;
        int r = fstat(fm.fd, &buf);
        if (r == -1) {
            perror("fstat");
            // File probably didn't exist, so fail gracefully by trying to create a new file
            fm.fd = createCrossProcessFile(file, fm.size);
            fm.map = (char*) mmap(0, fm.size, PROT_READ | PROT_WRITE, MAP_SHARED, fm.fd, 0);
        }

        if (fm.size > buf.st_size) // warning: comparison between signed and unsigned integer expressions ???
            ftruncate(fm.fd, fm.size); // + 1?
        fm.map = (char*) mmap(0, fm.size, PROT_READ | PROT_WRITE, MAP_SHARED, fm.fd, 0);
    } else {
        fm.fd = open(file, O_RDONLY);
        fm.map = (char*) mmap(0, fm.size, PROT_READ, MAP_PRIVATE, fm.fd, 0);
    }

    if (fm.map == MAP_FAILED)
        perror("mmap");
    assert(fm.map != MAP_FAILED);


#ifdef PRELOAD
    fm.preloadsize = PRELOAD_BYTES;
    if (fm.size < fm.preloadsize)
        fm.preloadsize = fm.size;
    fm.preloaded = (char*) malloc(fm.preloadsize);
    memcpy(fm.preloaded, fm.map, fm.preloadsize);
#else // Preload
    fm.preloaded = 0;
#endif // Preload

    return fm;
}

void unloadCrossProcessFile(FdAndMap fm) {
    msync(fm.map, fm.size, MS_SYNC); // TODO, check waarom dit nodig is! man mmap zegt van niet, praktijk leert van wel!
    munmap(fm.map, fm.size);
    close(fm.fd);
#ifdef PRELOAD
    free(fm.preloaded);
#endif
}
