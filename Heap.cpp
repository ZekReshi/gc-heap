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

#define readTypeDescP(a) (int*)(readLongLong(a - sizeof(int**)) - readLongLong(a - sizeof(int**)) % 4)
#define writeTypeDescP(a, b) writeIntP(a - sizeof(int**), b)
#define isFree(a) (readLongLong(a - sizeof(int**)) & 2)
#define setFree(a) writeLongLong(a - sizeof(int**), readLongLong(a - sizeof(int**)) | 2)
#define isUsed(a) (!isFree(a))
#define setUsed(a) writeLongLong(a - sizeof(int**), readLongLong(a - sizeof(int**)))
#define isMarked(a) (readLongLong(a - sizeof(byte*)) & 1)
#define setMarked(a) writeLongLong(a - sizeof(int**), readLongLong(a - sizeof(int**)) | 1)
#define readLength(a) readInt(readTypeDescP(a))
#define readNext(a) readByteP(a + sizeof(int))
#define writeNext(a, b) writeByteP(a + sizeof(int), b)

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
    writeIntP(freeList - sizeof(int**), reinterpret_cast<int *>(freeList));
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
        cur = readNext(cur);;
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

void Heap::mark(byte* root) {/*
    byte *cur = root;
    byte *prev = nullptr;
    //cout << "Root: " << root << endl;
    //cout << "Marking: " << descToName[*(int**)(cur - sizeof(byte*))] << endl;
    //cout << "Pre-mark: " << *((int**)(cur - sizeof(byte*))) << endl;
    setMark(cur);
    for (;;) {
        //cout << "Post-mark: " << *((int**)(cur - sizeof(byte*))) << endl;
        incTag(cur);
        //cout << "Post-increase: " << *((int**)(cur - sizeof(byte*))) << endl;
        unsigned mask = (*((unsigned long*)(cur - sizeof(byte*)))) % 4;
        //cout << cur << "/"
        //    << mask << "/"
        //    << cur - sizeof(byte*) << "/"
        //    << *((int**)(cur - sizeof(byte*))) << "/"
        //    << *((int**)(cur - sizeof(byte*))) - mask << "/"
        //    << *(*((int**)(cur - sizeof(byte*))) - mask) << endl;
        int off = *((int*)(*((byte**)(cur - sizeof(byte*))) - mask));
        //cout << "Offset " << off << endl;
        if (off >= 0) { // advance
            byte *p = *(byte **) (cur + off);
            //cout << "Points to: " << p << endl;
            if (p != nullptr && !isMarked(p)) {
                *(byte **) (cur + off) = prev;
                prev = cur;
                cur = p;
                //cout << "Marking: " << descToName[*(int**)(cur - sizeof(unsigned*))] << endl;
                //cout << "Pre-mark: " << *((int**)(cur - sizeof(byte*))) << endl;
                setMark(cur);
            }
        } else { // off < 0: retreat
            //cout << "Pre-restore: " << *((int**)(cur - sizeof(byte*))) << endl;
            *((int**)(cur - sizeof(byte*))) += off / sizeof(int); // restore tag
            //cout << "Post-restore: " << *((int**)(cur - sizeof(byte*))) << endl;
            //cout << "Previous: " << prev << endl;
            if (prev == nullptr) return;
            byte *p = cur;
            cur = prev;
            mask = (*((unsigned long*)(cur - sizeof(byte*)))) % 4;
            //cout << "Previous Tag: " << *((int**)(cur - sizeof(byte*))) << endl;
            off = *((int*)(*((byte**)(cur - sizeof(byte*))) - mask));
            //cout << "Previous offset: " << off << endl;
            prev = *(byte **) (cur + off);
            *(byte **) (cur + off) = p;
        }
    }*/
}

void Heap::sweep() {/*
    unsigned p = sizeof(unsigned);
    unsigned free = 0;
    while (p < heapsize) {
        if (isMarked()) p.marked = false;
        else { // free: collect p
            int size = p.tag.size;
            Pointer q = p + size;
            while (q < heapEnd && !q.marked) {
                size += q.tag.size; // merge
                q = q + q.tag.size;
            }
            p.tag = p; p.tag.size = size;
            p.next = free; free = p;
        }
        p += p.tag.size;
    }*/
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
        } while (cur != freeList);
    }
    cout << dec << "Free Memory: " << free << endl;
}
