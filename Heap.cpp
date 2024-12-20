//
// Created by flori on 09.09.2024.
//

#include <iostream>
#include "Heap.h"

#define readInt(a) *((int*)(a))
#define writeInt(a, b) *((int*)(a)) = b
#define readIntP(a) *((int**)(a))
#define writeIntP(a, b) *((int**)(a)) = b
#define readLongLong(a) *((long long*)(a))
#define writeLongLong(a, b) *((long long*)(a)) = b
#define readByte(a) *((byte*)(a))
#define writeByte(a, b) *((byte*)(a)) = b
#define readByteP(a) *((byte**)(a))
#define writeByteP(a, b) *((byte**)(a)) = b
#define typeTag(a) (a - sizeof(byte*))

#define readTypeDescP(a) (int*)(readLongLong(typeTag(a)) - readLongLong(typeTag(a)) % 4)
#define writeTypeDescP(a, b) writeIntP(typeTag(a), b)
#define isFree(a) (readLongLong(typeTag(a)) & 2)
#define setFree(a) writeLongLong(typeTag(a), readLongLong(typeTag(a)) | 2)
#define isUsed(a) (!isFree(a))
#define setUsed(a) writeLongLong(typeTag(a), readLongLong(typeTag(a)))
#define isMarked(a) (readLongLong(typeTag(a)) & 1)
#define setMarked(a) writeLongLong(typeTag(a), readLongLong(typeTag(a)) | 1)
#define setUnmarked(a) writeLongLong(typeTag(a), readLongLong(typeTag(a)) & ~1)
#define readLength(a) readInt(readTypeDescP(a))
#define readNext(a) readByteP(a + sizeof(int))
#define writeNext(a, b) writeByteP(a + sizeof(int), b)
#define readMask(a) ((*((unsigned long*)(typeTag(a)))) % 4)
#define readOffset(a) *((int*)(*((byte**)(typeTag(a))) - readMask(a)))
#define incTag(a) writeTypeDescP(a, readIntP(typeTag(a)) + 1)
#define restoreTag(a, b) writeTypeDescP(a, readIntP(typeTag(a))+b/4)

Heap::Heap() {
    heapsize = 32 * 1024 * sizeof(byte);
    cout << "Heap size: " << heapsize << endl;
    heap = static_cast<byte *>(malloc(heapsize));
    cout << "Heap address: " << heap << endl;
    freeList = heap + sizeof(int**);
    // necessary?
    for (int i = 0; i < heapsize; i++) {
        writeByte(heap + i, static_cast<byte>(0));
    }
    writeIntP(typeTag(freeList), reinterpret_cast<int *>(freeList));
    setFree(freeList);
    writeInt(freeList, heapsize);
    writeNext(freeList, freeList);
}

Heap::~Heap() {
    free(heap);
}

byte* Heap::alloc(const string& type) {
    int* descAdr = nameToDesc[type];
    int size = *descAdr;
    byte* cur = freeList;
    byte* prev = freeList;
    freeList = readNext(freeList);
    while (readLength(cur) < size + sizeof(int**) && cur != freeList) {
        prev = cur;
        cur = readNext(cur);
    }
    if (readLength(cur) < size) {
        cout << "HEAP OVERFLOW" << endl;
        return nullptr;
    }
    byte* p = freeList;
    int newLen = readLength(p) - size;
    if (newLen >= sizeof(int**) + sizeof(int) + sizeof(byte*)) { // split block
        p += readLength(p) - size;
        writeInt(p, size);
        writeInt(freeList, newLen);
    } else { // remove block from list
        if (freeList == prev) {
            freeList = nullptr;
        } else {
            writeNext(prev, readNext(freeList));
            freeList = prev;
        }
    }
    for (int i = 0; i < size - sizeof(int**); i++) {
        writeByte(p + i, static_cast<byte>(0));
    }
    writeTypeDescP(p, descAdr);
    setUsed(p);
    return p;
}

void Heap::registerType(const string& type, int* descAdr) {
    nameToDesc[type] = descAdr;
    descToName[descAdr] = type;
    cout << "Registered type " << type << " at " << descAdr << endl;
    cout << "- Size: " << *descAdr << endl;
    cout << "- Pointer offsets:";
    const int* cur = descAdr + 1;
    while (*cur >= 0) {
        cout << " " << *cur;
        cur++;
    }
    cout << endl;
}

void Heap::gc(byte** roots) {
    byte** cur = roots;
    while (*cur != nullptr) {
        mark(*cur);
        cur++;
    }
    sweep();
}

void Heap::mark(byte* root) {
    byte *cur = root;
    byte *prev = nullptr;
    setMarked(cur);
    for (;;) {
        incTag(cur);
        int off = readOffset(cur);
        if (off >= 0) { // advance
            byte *p = readByteP(cur + off);
            if (p != nullptr && !isMarked(p)) {
                writeByteP(cur + off, prev);
                prev = cur;
                cur = p;
                setMarked(cur);
            }
        } else { // off < 0: retreat
            restoreTag(cur,off);
            if (prev == nullptr) return;
            byte *p = cur;
            cur = prev;
            off = readOffset(cur);
            prev = readByteP(cur + off);
            writeByteP(cur + off, p);
        }
    }
}

void Heap::sweep() {
    byte* free = nullptr;
    byte* cur = heap + sizeof(int**);
    while (cur < heap + heapsize) {
        if (isMarked(cur))
            setUnmarked(cur);
        else { // free: collect p
            int size = readInt(readTypeDescP(cur));
            byte* q = cur + size;
            while (q < heap + heapsize && !isMarked(q)) {
                size += readInt(readTypeDescP(q)); // merge
                q += readInt(readTypeDescP(q));
            }
            writeByteP(typeTag(cur),cur);
            setFree(cur);
            writeInt(cur ,size);
            writeNext(cur,free);
            free = cur;
        }
        cur += readLength(cur);
    }
    freeList =free;
}

void Heap::dump() {
    cout << "Objects:" << endl;
    int used = 0;
    byte* cur = heap + sizeof(int**);
    while (cur < heap + heapsize) {
        if (isUsed(cur)) {
            used += readLength(cur);
            cout << "- Address: " << hex << cur << "/" << dec << cur - heap << endl;
            cout << "  Type: " << descToName[readTypeDescP(cur)] << endl;
            cout << "  Content: " << hex;
            for (int i = 0; i < 4; i++) {
                cout << static_cast<int>(readByte(cur + i));
            }
            cout << dec << endl;
            cout << "  Pointers: " << endl;
            const int* offset = readTypeDescP(cur);
            offset++;
            while (*offset >= 0) {
                cout << "    Offset " << *offset;
                cout << ", Value " << hex << readIntP(cur + *offset) << dec << endl;
                offset++;
            }
        }
        cur += readLength(cur);
    }
    cout << "Memory used: " << used << endl;
    cout << "Free Blocks:" << endl;
    int free = 0;
    cur = freeList;
    if (cur != nullptr) {
        do {
            cout << "- Free block at " << hex << cur << "/" << dec << cur - heap << " sized " << readLength(cur) << endl;
            free += readLength(cur);
            cur = readNext(cur);
        } while (cur != nullptr && cur != freeList);
    }
    cout << dec << "Free Memory: " << free << endl;
}
