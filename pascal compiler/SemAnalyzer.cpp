#include"SemAnalyzer.h"
#include"semDS.h"
#include"symtable.h"
#include"tree.h"
#include <cassert>

//错误标志
bool SemanticError = false;


//语义分析器的构造函数
SemAnalyzer::SemAnalyzer(){
    root = new Symtable();//创建第0层符号表，赋值给根
    root->init();
    this->init();
}

//语义分析器的构造函数，能访问ui的版本
SemAnalyzer::SemAnalyzer(Ui::MainWindow * parent) {
    root = new Symtable();//创建第0层符号表，赋值给根
    root->init();
    this->init();
    this->mainwidow = parent;
}

//初始化基本类型内部表示函数-- 2
void SemAnalyzer::init() {
    intptr = new typeIR();
    intptr->kind = TypeKind::intTy;
    intptr->size = 1;

    charptr = new typeIR();
    charptr->kind = TypeKind::charTy;
    charptr->size = 1;

    boolptr = new typeIR();
    boolptr->kind = TypeKind::boolTy;
    boolptr->size = 1;
}

//语义分析主函数-- 1
symbtable* SemAnalyzer::Analyze(TreeNode* t) {
    //root为Symtable*类型
    root->CreatTable();
    init();
    TreeNode* p = t->child[1];//声明
    while (p) {//循环处理声明部分
        if (p->nodekind == NodeKind::TypeK)
            TypeDecPart(p->child[0]);
        else if (p->nodekind == NodeKind::VarK)
            VarDecList(p->child[0]);
        else if (p->nodekind == NodeKind::ProcDecK)
            ProcDecPart(p);
        p = p->sibling;
    }
    t = t->child[2];
    if (t->nodekind == NodeKind::StmLK)//处理主程序体
    {
        Body(t);
    }
    symbtable* res = root->DestroyTable();
    return res;
}


//=====类型分析处理函数-- 3 （调用4，5，6，7）=====
/*处理当前节点类型，构造当前类型的内部表示并将其返回;
int和char类型已经构建好了；其余类型需新构建。
*/
typeIR* SemAnalyzer::TypeProcess(TreeNode* t, DecKind d) {
    if (d == DecKind::IdK) {
        return nameType(t);//自定义类型内部结构分析函数
    }
    else if (d == DecKind::ArrayK) {
        return arrayType(t);
    }
    else if (d == DecKind::RecordK) {
        return recordType(t);
    }
    else if (d == DecKind::IntegerK) {
        return intptr;
    }
    else if (d == DecKind::CharK) {
        return charptr;
    }
    return nullptr;
}

//自定义类型内部结构分析函数-- 4
typeIR* SemAnalyzer::nameType(TreeNode* t) {
    symbtable* tmp = nullptr;
    //到符号表中找已定义的类型名
    bool present = root->FindEntry(t->typeName, true, tmp);

    if (!present) {
        SemanticError = true;
        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->typeName.c_str()+" 未声明");
        return nullptr;
    }
    else {
        if (tmp->attrIR.kind != IdKind::typeKind) {
            SemanticError = true;
            mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误: " + t->typeName.c_str()+" 非类型标识符");
            return nullptr;
        }
        else {
            return tmp->attrIR.idtype;
        }
    }
}

//构建数组类型内部表示-- 5
typeIR* SemAnalyzer::arrayType(TreeNode* t) {
    int low = t->attr.ArrayAttr.low,
        up = t->attr.ArrayAttr.up;
    if (up < low) {
        SemanticError = true;
        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: 数组上界小于下界");
        return nullptr;
    }
    else {
        //创建新的数组类型内部表示
        typeIR* arraytp = new typeIR();
        arraytp->kind = TypeKind::arrayTy;
        arraytp->size = up - low + 1;
        arraytp->More.ArrayAttr.up = up;
        arraytp->More.ArrayAttr.low = low;
        arraytp->More.ArrayAttr.elemTy = TypeProcess(t, t->attr.ArrayAttr.childtype);
        arraytp->More.ArrayAttr.indexTy = TypeProcess(t, DecKind::IntegerK);
        return arraytp;
    }
}

//构建记录类型的内部表示-- 6
typeIR* SemAnalyzer::recordType(TreeNode* t) {
    typeIR* recordtp = new typeIR();
    fieldChain* cur = new fieldChain();
    fieldChain* body = cur, * tmp;
    recordtp->kind = TypeKind::recordTy;

    t = t->child[0];
    int tmpOff = 0;
    while (t) {//循环处理记录中的域变量
        int i = 0, cnt = t->idnum;
        while (i < cnt) {
            tmp = new fieldChain();
            tmp->idname = t->name[i];
            tmp->unitType = TypeProcess(t, t->kind.dec);
            tmp->next = nullptr;
            tmp->offset = tmpOff;
            tmpOff += tmp->unitType->size;
            cur->next = tmp;
            cur = tmp;
            i++;
        }
        t = t->sibling;
    }
    recordtp->size = tmpOff;
    recordtp->More.body = body->next;
    return recordtp;
}

//类型声明部分分析处理函数-- 7
void SemAnalyzer::TypeDecPart(TreeNode* t) {
    //t现在指向子节点第一个声明
    symbtable* entry = nullptr;
    AttributeIR* attrIr = new AttributeIR();
    attrIr->kind = IdKind::typeKind;

    while (t) {
        bool present = root->Enter(t->name[0], attrIr, entry, false);//填写符号表
        if (present) {
            SemanticError = true;
            mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 重复定义");
            entry = nullptr;
        }
        else {
            entry->attrIR.idtype = TypeProcess(t, t->kind.dec);//构造类型内部表示
        }
        t = t->sibling;
    }
}
//===========================================

//=====变量声明部分分析处理函数-- 8======
void SemAnalyzer::VarDecList(TreeNode* t) {
    bool present = false;
    symbtable* entry = NULL;
    AttributeIR* attrIr = new AttributeIR();
    attrIr->kind = IdKind::varKind;
    while (t) {
        int i = 0;
        while (i < t->idnum) {
            attrIr->idtype = TypeProcess(t, t->kind.dec);//构造类型内部表示
            //判断值参还是变参(dir,indir)
            if (t->attr.ProcAttr.paramt == ParamType::varparamType) {//valparamType, varparamType
                attrIr->More.VarAttr.access = AccessKind::indir;//间接接变量
                attrIr->More.VarAttr.level = root->curLevel;//记录层数
                attrIr->More.VarAttr.off = root->CurOff;
                root->CurOff++;//偏移+1
            }
            else {
                attrIr->More.VarAttr.access = AccessKind::dir;//直接变量
                attrIr->More.VarAttr.level = root->curLevel;//记录层数
                /*计算值参的偏移*/
                if (attrIr->idtype != nullptr) {
                    attrIr->More.VarAttr.off = root->CurOff;
                    root->CurOff += attrIr->idtype->size;//偏移+size
                }
            }
            if (attrIr->idtype != nullptr) {
                present = root->Enter(t->name[i], attrIr, entry, false);//填写符号表
                if (present != false) {
                    SemanticError = true;
                   mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误: " + t->name[i].c_str() + " 重复定义");
                    entry = nullptr;
                }
                else {
                    //填写语法树中标识符在符号表中的入口
                    t->table[i] = entry;
                }
            }
            i++;
        }
        t = t->sibling;
    }
}
//===========================================

//=====过程声明部分分析处理函数-- 9（调用7，8，10，11，12）=====
void SemAnalyzer::ProcDecPart(TreeNode* t) {
    //t直接指向过程声明1
    TreeNode* p = t;
    //处理头（包含参数处理，以及建立新一层符号表）
    symbtable* entry = HeadProcess(t);
    t = t->child[1];
    //处理声明（类型声明，变量声明，过程声明）
    while (t) {
        if (t->nodekind == NodeKind::TypeK)
            TypeDecPart(t->child[0]);
        else if (t->nodekind == NodeKind::VarK)
            VarDecList(t->child[0]);
        else if (t->nodekind == NodeKind::ProcDecK) {
            break;
        }
        else {
            SemanticError = true;
            mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误 : 语法树无此类节点");
        }
        t = t->sibling;
    }
    while (t) {
        ProcDecPart(t);
        t = t->sibling;
    }
    t = p;
    //处理过程体
    Body(t->child[2]);
    //因为是局部符号表，所以在当前子过程处理结束后应撤销当前符号表
    entry->nextLevel = root->DestroyTable();
}

//过程声明头分析函数-- 10
symbtable* SemAnalyzer::HeadProcess(TreeNode* t) {
    //t指向过程声明1
    AttributeIR* attrIr = new AttributeIR();
    attrIr->kind = IdKind::procKind;
    attrIr->idtype = nullptr;
    attrIr->More.ProcAttr.level = root->curLevel;
    //code、size在目标代码生成时回填
    bool present = false;
    symbtable* entry = nullptr;

    if (t) {
        //填写该节点在符号表中的入口
        present = root->Enter(t->name[0], attrIr, entry, false);
        t->table[0] = entry;
    }
    //处理参数，并连接形参链表
    entry->attrIR.More.ProcAttr.param = ParaDecList(t->child[0]);
    return entry;
}

//形参分析处理函数-- 11
//此时 t 指向形参1位置
ParamTable* SemAnalyzer::ParaDecList(TreeNode* t) {
    //处理参数，建立新的符号表
    root->CreatTable();
    if (t == nullptr) {
        return nullptr;
    }
    //处理参数列表
    VarDecList(t);
    //构造形参信息表，并使其连接至符号表对应的param
    symbtable* cur = root->table[root->curLevel];
    ParamTable* head = new ParamTable(), * h = head;
    ParamTable* tmp = nullptr;
    while (cur->next) {
        tmp = new ParamTable();
        tmp->entry = cur;
        h->next = tmp;
        h = tmp;
        tmp = nullptr;
        cur = cur->next;
    }
    h = head->next;
    delete head;
    return h;
}

//=====================================================================

//执行体部分分析处理函数-- 12
void SemAnalyzer::Body(TreeNode* t) {
    //执行体部分全是语句节点，循环掉用语句分析处理函数
    if (t->nodekind == NodeKind::StmLK) {
        TreeNode* p = t->child[0];
        while (p) {
            statement(p);	//语句分析
            p = p->sibling;
        }
    }
}

//语句序列分析处理函数-- 13（调用17，18...23）
void SemAnalyzer::statement(TreeNode* t) {
    if (t->kind.stmt == StmtKind::IfK) {
        ifstatment(t);
    }
    else if (t->kind.stmt == StmtKind::WhileK) {
        whilestatement(t);
    }
    else if (t->kind.stmt == StmtKind::AssignK) {
        assignstatement(t);
    }
    else if (t->kind.stmt == StmtKind::ReadK) {
        readstatement(t);
    }
    else if (t->kind.stmt == StmtKind::WriteK) {
        writestatement(t);
    }
    else if (t->kind.stmt == StmtKind::CallK) {
        callstatement(t);
    }
    else if (t->kind.stmt == StmtKind::ReturnK) {
        returnstatement(t);
    }
    else {
        SemanticError = true;
        mainwidow->listWidget_2->addItem("[Error] lineno_" + QString::number(t->lineNo) + " : 语句类型非法");
    }
}

//表达式分析处理函数-- 14（17-23调用14，15，16）
//检查运算分量的类型相容性；返回表达式的类型
typeIR* SemAnalyzer::Expr(TreeNode* t, AccessKind* EAccKind) {
    bool present = false;
    symbtable* entry = nullptr;
    typeIR* Eptr = nullptr;//该表达式的类型
    typeIR* Eptr0 = nullptr;//可能存在的两个运算分量
    typeIR* Eptr1 = nullptr;
    while (t) {
        if (t->kind.exp == ExpKind::ConstK) {//处理常量 如a=4右部的4
            Eptr = TypeProcess(t, DecKind::IntegerK);
            Eptr->kind = TypeKind::intTy;
            if (EAccKind != nullptr) {
                *EAccKind = AccessKind::dir;
            }
        }
        else if (t->kind.exp == ExpKind::VariK) {//处理变量 如a=4左部的a
           if (t->child[0] == nullptr) {//变量是非结构体/数组类型
                present = root->FindEntry(t->name[0], true, entry);
                t->table[0] = entry;
                if (present) {
                    if (entry->attrIR.kind != IdKind::varKind) {
                        SemanticError = true;
                        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 不是变量类型");
                        Eptr = nullptr;
                    }
                    else {
                        Eptr = entry->attrIR.idtype;
                        if (EAccKind != nullptr) {
                            *EAccKind = AccessKind::indir;//变量
                        }
                    }
                }
                else {
                    SemanticError = true;
                    mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + "标识符变量未声明");
                }
            }
            else {//变量是数组或记录
                if (t->attr.ExpAttr.varkind == VarKind::ArrayMembV)
                    Eptr = arrayVar(t);//Eptr为数组单员的类型
                else if (t->attr.ExpAttr.varkind == VarKind::FieldMembV)
                    Eptr = recordVar(t);//Eptr为记录成员的类型
            }
        }
        else if (t->kind.exp == ExpKind::OpK) {//处理操作符（带操作符的表达式）
            AccessKind* acc1 = new AccessKind();
            AccessKind* acc2 = new AccessKind();
            //处理左分量
            Eptr0 = Expr(t->child[0], acc1);
            if (Eptr0 == nullptr)
                return nullptr;
            //处理右分量
            Eptr1 = Expr(t->child[1], acc2);
            if (Eptr1 == nullptr)
                return nullptr;
            if (Eptr0 == Eptr1) {
                if (t->attr.ExpAttr.op == LexType::EQ||t->attr.ExpAttr.op == LexType::LT)
                    Eptr = boolptr;
                else if (t->attr.ExpAttr.op == LexType::DIVIDE
                         ||t->attr.ExpAttr.op == LexType::PLUS
                         ||t->attr.ExpAttr.op == LexType::TIMES
                         ||t->attr.ExpAttr.op == LexType::MINUS)
                    Eptr = intptr;
                if (EAccKind != nullptr) {
                    if (*acc1 == AccessKind::dir && *acc2 == AccessKind::dir) {
                        *EAccKind = AccessKind::dir;
                    }
                    else {
                        *EAccKind = AccessKind::indir;
                    }
                }
            }
            else {
                SemanticError = true;
                mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误 : 操作符类型不匹配" );
            }
        }
        return Eptr;
    }
    return nullptr;
}

//数组变量的处理分析函数-- 15
typeIR* SemAnalyzer::arrayVar(TreeNode* t) {
    bool present = false;
    symbtable* entry = nullptr;
    typeIR* Eptr = nullptr;
    typeIR* Eptr0 = nullptr;//数组在符号表中登记的下标类型
    typeIR* Eptr1 = nullptr;//使用中的数组的下标类型

    present = root->FindEntry(t->name[0], true, entry);//找数组名是否存在
    t->table[0] = entry;

    if (present) {
        if (entry->attrIR.kind != IdKind::varKind) {
            SemanticError = true;
            mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 不是数组类型变量");
            Eptr = nullptr;
        }
        else {
            if (entry->attrIR.idtype->kind != TypeKind::arrayTy) {
                SemanticError = true;
                mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 不是数组类型变量");
                Eptr = nullptr;
            }
            else {
                //检查下标类型
                Eptr0 = entry->attrIR.idtype->More.ArrayAttr.indexTy;
                if (Eptr0 == nullptr) 	return nullptr;
                AccessKind* acc = new AccessKind();
                Eptr1 = Expr(t->child[0], acc);

                if (Eptr1 == nullptr) return nullptr;
                if (Eptr0 != Eptr1) {
                    present = false;
                }
                else {
                    present = true;
                }
                if (present != true) {
                    SemanticError = true;
                    mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误 : 下标类型不符");
                    Eptr = nullptr;
                }
                else {
                    Eptr = entry->attrIR.idtype->More.ArrayAttr.elemTy;
                }
            }
        }
    }
    else {
        SemanticError = true;
        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 标识符未声明");
    }
    return Eptr;
}

//记录变量中域变量的分析处理函数-- 16
typeIR* SemAnalyzer::recordVar(TreeNode* t) {
    symbtable* entry = nullptr;
    typeIR* Eptr = nullptr;
    typeIR* Eptr0 = nullptr;
    fieldChain* currentP = nullptr;
    bool present = false;
    present = root->FindEntry(t->name[0], true, entry);
    t->table[0] = entry;
    if (present) {
        if (entry->attrIR.kind != IdKind::varKind) {
            SemanticError = true;
            mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 不是记录类型变量");
            Eptr = nullptr;
        }
        else {
            if (entry->attrIR.idtype->kind != TypeKind::recordTy) {
                SemanticError = true;
                mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 不是记录类型变量");
                Eptr = nullptr;
            }
            else {
                //Expr0代表此record类型
                Eptr0 = entry->attrIR.idtype;
                currentP = Eptr0->More.body;
                bool result = false;
                while (currentP && !result) {
                    //检查域名是否存在
                    result = (t->child[0]->name[0] == currentP->idname);
                    if (result)
                        Eptr = currentP->unitType;
                    else {
                        currentP = currentP->next;
                    }
                }

                if (currentP == nullptr) {
                    if (!result) {
                        SemanticError = true;
                        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->child[0]->name[0].c_str() + " 没有此域名");
                        Eptr = nullptr;
                    }
                }
            }
        }
    }
    else {
        SemanticError = true;
        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 标识符变量未声明");
    }
    return Eptr;
}

//赋值语句分析函数-- 17
void SemAnalyzer::assignstatement(TreeNode* t) {
    symbtable* entry = nullptr;
    bool present = false;
    typeIR* Eptr = nullptr;
    typeIR* ptr = nullptr;

    TreeNode* child1 = t->child[0];		//左
    TreeNode* child2 = t->child[1];		//右

    if (child1->child[0] == nullptr) {
        present = root->FindEntry(child1->name[0], true, entry);//符号表中查找
        if (present) {
            if (entry->attrIR.kind != IdKind::varKind) {
                SemanticError = true;
                mainwidow->listWidget_2->addItem(QString::number(child1->lineNo) + "\t行 语义错误: " + child1->name[0].c_str() + " 不是变量");
            }
            else {//是变量类型
                Eptr = entry->attrIR.idtype;
                child1->table[0] = entry;
            }
        }
        else {
            SemanticError = true;
            mainwidow->listWidget_2->addItem( QString::number(child1->lineNo) + "\t行 语义错误: " + child1->name[0].c_str() + " 符号表无此标识符");
        }
    }
    else {
        if (child1->attr.ExpAttr.varkind == VarKind::ArrayMembV) {
            Eptr = arrayVar(child1);
        }
        else {
            if (child1->attr.ExpAttr.varkind == VarKind::FieldMembV) {
                Eptr = recordVar(child1);
            }
        }
    }

    if (Eptr) {
        if ((t->nodekind == NodeKind::StmtK) && (t->kind.stmt == StmtKind::AssignK)) {
            AccessKind* acc = new AccessKind();
            ptr = Expr(child2, acc);
            if (ptr != Eptr) {
                SemanticError = true;
                mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误 : 赋值号两端类型不相容" );
            }
        }
    }
}

//过程调用语句分析处理函数-- 18
void SemAnalyzer::callstatement(TreeNode* t) {
    AccessKind* Ekind = new AccessKind();
    bool present = false;
    symbtable* Entry = nullptr;
    present = root->FindEntry(t->child[0]->name[0], true, Entry);
    t->child[0]->table[0] = Entry;

    if (present == false) {
        SemanticError = true;
        mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误: " + t->child[0]->name[0].c_str() + " 函数未定义");
    }
    else {
        if (Entry->attrIR.kind != IdKind::procKind) {
            SemanticError = true;
            mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误: " + t->child[0]->name[0].c_str() + " 非过程名");
        }
        else {
            //处理形实参结合：p为形参，paramP实参，检查二者是否匹配
            TreeNode* p = t->child[1];
            ParamTable* paramP = Entry->attrIR.More.ProcAttr.param;
            while (p && paramP) {
                symbtable* paraEntry = paramP->entry;
                typeIR* Etp = Expr(p, Ekind);
                if ((paraEntry->attrIR.More.VarAttr.access == AccessKind::indir) && (Ekind != nullptr) && (*Ekind == AccessKind::dir)) {
                    SemanticError = true;
                    mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误 : 形参与值参不匹配");
                }
                else {
                    if(Etp==nullptr){
                        //mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误 : 标识符未声明");
                    }
                    else if (!(*(paraEntry->attrIR.idtype) == *Etp)) {////////////////////////////////////////////
                        SemanticError = true;
                        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误 : 参数类型不匹配");
                    }
                }
                p = p->sibling;
                paramP = paramP->next;
            }
            if (p || paramP) {
                SemanticError = true;
                mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误 : 参数个数不匹配");
            }
        }
    }
}

//条件语句分析处理函数-- 19
void SemAnalyzer::ifstatment(TreeNode* t) {
    AccessKind* acc = new AccessKind();
    typeIR* Etp = Expr(t->child[0], acc);//处理if条件表达式
    if (Etp) {
        if (Etp->kind != TypeKind::boolTy) {
            SemanticError = true;
            mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误 : 条件表达式非布尔类型" );
        }
        else {
            TreeNode* p = t->child[1];
            while (p) {//then部分
                statement(p);
                p = p->sibling;
            }
            p = t->child[2];
            while (p) {//else部分
                statement(p);
                p = p->sibling;
            }
        }
    }
}

//循环语句分析处理函数-- 20
void SemAnalyzer::whilestatement(TreeNode* t) {
    AccessKind* acc = new AccessKind();
    typeIR* Etp = Expr(t->child[0], acc);//处理while条件表达式
    if (Etp) {
        if (Etp->kind != TypeKind::boolTy) {
            SemanticError = true;
            mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误 : 条件表达式非布尔类型");
        }
        else {//处理then
            t = t->child[1];
            while (t) {
                statement(t);
                t = t->sibling;
            }
        }
    }
}

//读语句分析处理函数-- 21
void SemAnalyzer::readstatement(TreeNode* t) {
    symbtable* entry = nullptr;
    bool present = false;
    present = root->FindEntry(t->name[0], true, entry);
    t->table[0] = entry;
    if (present == false) {
        SemanticError = true;
        mainwidow->listWidget_2->addItem(QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 未声明");
    }
    else if (entry->attrIR.kind != IdKind::varKind)
    {
        SemanticError = true;
        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: " + t->name[0].c_str() + " 非变量标识符");
    }
}

//返回语句分析处理函数-- 22
void SemAnalyzer::returnstatement(TreeNode* t) {
    if (root->curLevel == 0) {
        SemanticError = true;
        mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + "\t行 语义错误: 返回语句在主程序中出现");
    }
}

//写语句分析处理函数-- 23
void SemAnalyzer::writestatement(TreeNode* t) {
    AccessKind* acc = new AccessKind();
    typeIR* Etp = Expr(t->child[0], acc);
    if (Etp) {
        if (Etp->kind == TypeKind::boolTy) {
            SemanticError = true;
            mainwidow->listWidget_2->addItem( QString::number(t->lineNo) + " : 表达式类型出错");
        }
    }
}
