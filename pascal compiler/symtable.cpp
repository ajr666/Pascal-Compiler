#include"semDS.h"
#include"symtable.h"
//#include"macro.h"
#include"SemKindToStr.h"
#include <string>

int initOff = 7;

Symtable::Symtable() {

}

void Symtable::init() {
    CurOff = 0;
    curLevel = -1;
}

//1.进入一个新的局部化单位时调用本程序，创建一个空的符号表
void Symtable::CreatTable() {
    if (curLevel != -1) {
        //存上一层的符号表尾巴，为了嵌套层结束继续插入
        tableTail[curLevel] = curSymtable;
    }
    CurOff = initOff;
    table[++curLevel] = new symbtable();
    curSymtable = table[curLevel];
}

//2.撤销本层符号表
//返回被撤销符号表的头
symbtable* Symtable::DestroyTable() {
    symbtable* head = table[curLevel];
    table[curLevel] = nullptr;
    tableTail[curLevel] = nullptr;

    curLevel--;
    //撤销当前符号表之后，cur_symtable应该放在上一层的尾巴，因为后面可能还要插入
    if (curLevel >= 0) {
        curSymtable = tableTail[curLevel];
    }
    else {
        curSymtable = nullptr;
    }
    return head;
}

//在当前符号表查找Id，若找到则用entry指向它
bool Symtable::SearchoneTable(string Id, int currentLevel, symbtable*& entry) {
    symbtable* head = table[currentLevel];
    while (head) {
        if (head->name == Id) {
            entry = head;
            return true;
        }
        head = head->next;
    }
    entry = nullptr;
    return false;
}

//4.在整个符号表中查找Id,若找到则用entry指向它
bool Symtable::FindEntry(string Id, bool flag, symbtable*& entry) {
    //flag为0，当前符号表。否则全局符号表
    if (!flag) {
        return SearchoneTable(Id, curLevel, entry);
    }
    else {
        for (int i = curLevel; i >= 0; i--) {
            if (SearchoneTable(Id, i, entry)) {
                return true;
            }
        }
        entry = nullptr;
        return false;
    }
}

//5.在域表中查找域名
bool Symtable::FindField(string Id, fieldChain* head, fieldChain*& entry) {
    fieldChain* ah = head;
    while (ah) {
        if (ah->idname == Id) {
            entry = ah;
            return true;
        }
        ah = ah->next;
    }
    entry = nullptr;
    return false;
}

//3.把标识符和属性登记到符号表
//先查找，再登记；true:找到  false:没找到
bool Symtable::Enter(string Id, AttributeIR* att, symbtable*& entry, bool flag) {
    if (FindEntry(Id, flag, entry)) {
        entry = nullptr;
        return true;
    }
    else {
        curSymtable->attrIR = *att;
        curSymtable->name = Id;
        curSymtable->next = new symbtable();
        entry = curSymtable;
        curSymtable = curSymtable->next;
        return false;
    }
}

//====转换string====
string Symtable::typeKindShow(typeIR* tr) {
    string ans;
    if (!tr)
        return ans;
    ans += "[TypeKind]: " + TypeKindToStr[tr->kind] + "\n[TypeSize]: " + to_string(tr->size) + "\n";
    if (tr->kind == TypeKind::arrayTy) {
        ans += "[indexTy]:\n";
        ans += typeKindShow(tr->More.ArrayAttr.indexTy);
        ans += "[elemTy]:\n";
        ans += typeKindShow(tr->More.ArrayAttr.elemTy);
    }
    else if (tr->kind == TypeKind::recordTy) {
        fieldChain* head = tr->More.body;
        while (head)
        {
            ans += head->idname + ":";
            ans += typeKindShow(head->unitType);
            head = head->next;
        }
    }
    return ans;
}

string Symtable::varKindShow(AttributeIR& at) {
    string ans;
    ans += "[AccessKind]:";
    if (at.More.VarAttr.access == AccessKind::dir)
        ans += "dir\n";
    else
        ans += "indir\n";
    ans += "[Off]: " + to_string(at.More.VarAttr.off) + "\n";
    return ans;
}

string Symtable::procKindShow(AttributeIR& at) {
    //printf("[Level]: %d\n", at.More.ProcAttr.level);
    //return "[Level]: " + to_string(at.More.ProcAttr.level) + "\n";
    return "";
}
//==================

//打印整个符号表
void Symtable::PrintSymbTable() {
    for (int i = 1; i <= curLevel; i++) {
        printf("当前层数：%d\n", i);
        symbtable* head = table[i];
        while (head) {
            AttributeIR& at = head->attrIR;
            //show
            printf("[Id_Name]: %s\n", head->name.c_str());
            printf("[Id_Kind]: %s\n", IdKindToStr[at.kind].c_str());
            if (at.kind == IdKind::typeKind) {
                //类型标识符
                typeKindShow(at.idtype);
            }
            else if (at.kind == IdKind::varKind) {
                //变量标识符
                varKindShow(at);
            }
            else {
                //过函标识符
                procKindShow(at);
            }
            puts("");
            head = head->next;
        }
    }
}

//
void Symtable::print(symbtable* root, int level)
{
    symbtable* head = root;
    while (head && head->next)
    {
        AttributeIR& at = head->attrIR;
        //show
        printf("[Id_Name]: %s\n", head->name.c_str());
        printf("[Id_Kind]: %s\n", IdKindToStr[at.kind].c_str());
        if (at.kind == IdKind::typeKind) {
            //类型标识符
            cout << typeKindShow(at.idtype);
        }
        else if (at.kind == IdKind::varKind) {
            //变量标识符
            cout << "[Level]: " + to_string(at.More.VarAttr.level) + "\n";
            cout << varKindShow(at);
        }
        else {
            //过函标识符
            cout << "[Level]: " + to_string(at.More.ProcAttr.level) + "\n";
            cout << endl;
            print(head->nextLevel);
        }
        cout << endl;
        head = head->next;
    }
}
