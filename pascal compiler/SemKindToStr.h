#pragma once
#include<unordered_map>
#include<string>
#include"global.h"
using namespace::std;
//在语义分析中，将枚举类型转为字符串类型用于打印
extern unordered_map<LexType, string> LexName;
extern unordered_map<NodeKind, string>  NodeKindName;
extern unordered_map<DecKind, string>  DecKindName;
extern unordered_map<StmtKind, string>  StmtKindName;
extern unordered_map<ExpKind, string>  ExpKindName;
extern unordered_map<ParamType, string>  ParamTypeName;
extern unordered_map<VarKind, string>  VarKindName;
extern unordered_map<ExpType, string>  ExpTypeName;
extern unordered_map<TypeKind, string> TypeKindToStr;
extern unordered_map<IdKind, string> IdKindToStr;

