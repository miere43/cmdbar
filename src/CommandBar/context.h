//#pragma once
//#include "common.h"
//#include <vector>
//
//typedef struct
//{
//	wchar_t* data;
//	size_t   length;
//} CB_CommandArgument;
//
//struct CB_Context;
//typedef void (*CommandCallback)(CB_Context* context, CB_CommandArgument * args, size_t numArgs, void* userdata);
//
//struct CB_Macro
//{
//	wchar_t* name;
//	size_t   nameLength;
//
//	wchar_t* expansion;
//	size_t   expansionLength;
//};
//
////enum CB_CommandDefFlags
////{
////	CB_DONT_SPLIT_ARGUMENTS = 1,
////};
//
//struct CB_CommandDef
//{
//	int flags;
//
//	wchar_t* name;
//	size_t   nameLength;
//
//	wchar_t* help;
//	CommandCallback callback;
//	void* userdata;
//};
//
//struct CB_Context
//{
//	std::vector<CB_CommandDef> commands;
//	std::vector<CB_Macro> macros;
//};
//
//void CB_InitCoreFunctionality();
//bool CB_InitContext(CB_Context* context);
//void CB_AddCommand(CB_Context* context, wchar_t * name, wchar_t * help, CommandCallback callback, void * userdata);
//void CB_AddMacro(CB_Context* context, wchar_t * name, wchar_t * expansion);
//CB_Macro * CB_FindMacro(CB_Context * context, wchar_t * name, size_t nameLength);
//int  CB_EvaluateExpression(CB_Context * context, wchar_t * expression, size_t expressionLength);
//void CB_LoadCommands(CB_Context* context);
//
