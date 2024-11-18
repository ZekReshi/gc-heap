//
// Created by flori on 09.09.2024.
//
#ifndef GC_HEAP_H
#define GC_HEAP_H

#include <string>
#include <map>

using namespace std;

class Heap {
    int heapsize;
    byte* heap;
    byte* freeList;
    map<string, int*> nameToDesc;
    map<int*, string> descToName;

    void mark(byte* root);
    void sweep();
public:
    Heap();
    ~Heap();

    byte* alloc(const string& type);
    void registerType(const string& type, int* descAdr);
    void gc(byte** roots);
    void dump();
};


#endif //GC_HEAP_H
