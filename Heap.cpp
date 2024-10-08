//
// Created by flori on 09.09.2024.
//

#define heapAt(a) *((unsigned*) (heap + a))
#define adrAt(a) *((int**) (heap + a))
#define lenAt(a) setFree(heapAt(a - sizeof(unsigned)))
#define setUsed(a) (a | 0x80000000)
#define setFree(a) (a & 0x7fffffff)
#define isFreeAt(a) (~heapAt(a - sizeof(unsigned)) & 0x80000000)
#define isUsedAt(a) (heapAt(a - sizeof(unsigned)) & 0x80000000)
#define setMark(a) *((long*)(a - sizeof(byte*))) |= 1
#define incTag(a) *((byte**)(a - sizeof(byte*))) += sizeof(int)
#define isMarked(a) (((*((long*)(a - sizeof(byte*)))) & 1) != 0)

#include <iostream>
#include "Heap.h"

Heap::Heap() {
    heapsize = 32 * 1024 * sizeof(byte);
    cout << "Heap size: " << heapsize << endl;
    heap = (byte*) malloc(heapsize);
    cout << "Heap address: " << heap << endl;
    freeList = sizeof(unsigned);
    // necessary?
    for (unsigned i = 0; i < heapsize; i++) {
        heap[i] = (byte) 0;
    }
    heapAt(freeList - sizeof(unsigned)) = setFree(heapsize);
    heapAt(freeList) = sizeof(unsigned);
}

Heap::~Heap() {
    free(heap);
}

byte* Heap::alloc(const string& type) {
    int* descAdr = nameToDesc[type];
    int size = *descAdr;

    //cout << "Searching size " << size << endl;
    unsigned cur = freeList;
    unsigned prev = freeList;
    while (lenAt(cur) < size + sizeof(unsigned) && cur != freeList) {
        prev = cur;
        cur = heapAt(cur);
    }
    if (lenAt(cur) < size + sizeof(unsigned)) {
        cout << "HEAP OVERFLOW" << endl;
        return nullptr;
    }
    else {
        unsigned newLen = lenAt(cur) - (size + 4);
        if (newLen >= 8) { // split block
            unsigned newStart = cur + lenAt(cur) - (size + 4);
            //cout << "NS: " << newStart << endl;
            //cout << "SP4: " << (size + 4) << endl;
            //cout << "SP4M: " << setUsed(size + 4) << endl;
            heapAt(newStart - sizeof(unsigned)) = setUsed(size + sizeof(unsigned));
            //cout << "NLM: " << setFree(newLen) << endl;
            //cout << "P: " << prev << endl;
            heapAt(prev-4) = setFree(newLen);
            for (unsigned i = newStart; i < newStart + size; i++) {
                heap[i] = (byte) 0;
            }
            adrAt(newStart) = descAdr;
            return heap + newStart + sizeof(unsigned*);
        } else { // remove block from list
            if (cur == prev) { // last free block
                freeList = 0;
            } else {
                heapAt(prev) = heapAt(cur);
                freeList = prev;
            }
            heapAt(cur - sizeof(unsigned)) = setUsed(heapAt(cur - sizeof(unsigned)));
            for (unsigned i = cur; i < cur + size; i++) {
                heapAt(i) = 0;
            }
            adrAt(cur) = descAdr;
            return heap + cur + sizeof(unsigned*);
        }
    }
}

void Heap::registerType(const string& type, int* descAdr) {
    nameToDesc[type] = descAdr;
    descToName[descAdr] = type;
    cout << "Registered type " << type << " at " << descAdr << endl;
    cout << "- Size: " << *descAdr << endl;
    cout << "- Pointer offsets:";
    int* cur = descAdr + 1;
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
    }
}

void Heap::sweep() {
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
    }
}

void Heap::dump() {
    //cout << "H0: " << heapAt(0) << endl;return;
    cout << "Objects:" << endl;
    unsigned acc = 0;
    unsigned cur = sizeof(unsigned);
    if (isFreeAt(cur)) {
        cur = cur + lenAt(cur);
    }
    //cout << cur << " " << lenAt(cur) << " " << isUsedAt(cur) << endl;
    while (cur < heapsize) {
        //cout << cur << ": " << lenAt(cur) << endl;
        if (isUsedAt(cur)) {
            cout << "- Address: " << hex << cur << "/" << &heapAt(cur) << endl;
            cout << "  Type: " << descToName[adrAt(cur)] << endl;
            cout << "  Content: " << hex;
            for (int i = 0; i < 4 && i < (*adrAt(cur) - sizeof(unsigned*)) / sizeof(unsigned); i++) {
                cout << to_integer<int>(heap[cur + sizeof(unsigned*) + i]);
            }
            cout << dec << endl;
            acc += lenAt(cur);
        }
        cur += lenAt(cur);
    }
    cout << "Memory used: " << acc << endl;
    cout << "Free Blocks:" << endl;
    acc = 0;
    cur = freeList;
    do {
        cout << "- Free block at " << cur << " sized " << lenAt(cur) << endl;
        acc += lenAt(cur);
        cur = heapAt(cur);
    } while (cur != freeList);
    cout << "Free Memory: " << acc << endl << endl;
}
