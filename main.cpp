#include <iostream>
#include "Heap.h"

struct Lecture {
    int id;
    string name;
    int semester;
};

struct LectNode {
    LectNode* next;
    Lecture* lect;
};

struct Student {
    int id;
    string name;
    LectNode* lect;
};

struct StudNode {
    StudNode* next;
    Student* stud;
};

struct StudentList {
    StudNode* first;
};

int main() {
    std::cout << "Hello, World!" << std::endl;

    Heap heap;
    int studentListDesc[] = { sizeof(int*) + sizeof(StudentList), 0, (int)sizeof(int) * -2 };
    heap.registerType("StudentList", &studentListDesc[0]);
    int studNodeDesc[] = { sizeof(int*) + sizeof(StudNode), 0, 8, (int)sizeof(int) * -3 };
    heap.registerType("StudNode", &studNodeDesc[0]);
    int studentDesc[] = { sizeof(int*) + sizeof(Student), 40, (int)sizeof(int) * -2 };
    heap.registerType("Student", &studentDesc[0]);
    int lectNodeDesc[] = { sizeof(int*) + sizeof(LectNode), 0, sizeof(LectNode*), (int)sizeof(int) * -3 };
    heap.registerType("LectNode", &lectNodeDesc[0]);
    int lecture[] = { sizeof(int*) + sizeof(Lecture), (int)sizeof(unsigned) * -1 };
    heap.registerType("Lecture", &lecture[0]);

    heap.dump();

    Lecture* ssw = (Lecture*) heap.alloc("Lecture");
    ssw->id = 1;
    ssw->name = "System Software";
    ssw->semester = 20242;
    Lecture* qc = (Lecture*) heap.alloc("Lecture");
    qc->id = 2;
    qc->name = "Quantum Computing";
    qc->semester = 20242;

    LectNode* sswNode = (LectNode*) heap.alloc("LectNode");
    sswNode->lect = ssw;
    LectNode* qcNode = (LectNode*) heap.alloc("LectNode");
    qcNode->lect = qc;
    sswNode->next = qcNode;
    cout << sswNode << " " << &sswNode->next << " " << &sswNode->lect << endl;

    LectNode* qcNode2 = (LectNode*) heap.alloc("LectNode");
    qcNode2->lect = qc;

    Student* fs = (Student*) heap.alloc("Student");
    fs->id = 1;
    fs->name = "Florian Schwarcz";
    fs->lect = sswNode;
    Student* mw = (Student*) heap.alloc("Student");
    mw->id = 2;
    mw->name = "Maximilian Wahl";
    mw->lect = qcNode2;

    StudNode* fsNode = (StudNode*) heap.alloc("StudNode");
    fsNode->stud = fs;
    StudNode* mwNode = (StudNode*) heap.alloc("StudNode");
    mwNode->stud = mw;
    fsNode->next = mwNode;

    StudentList* list = (StudentList*) heap.alloc("StudentList");
    list->first = fsNode;

    heap.dump();

    mw->lect = nullptr;
    byte* roots[] = { (byte *) list, nullptr };
    heap.gc(roots);
    std::cout << "Deleted MW's lecture and let gc run" << std::endl;
    heap.dump();

    byte* roots2[] = { nullptr };
    heap.gc(roots2);
    std::cout << "Deleted root and let gc run" << std::endl;
    heap.dump();

    return 0;
}
