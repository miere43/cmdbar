//#include "context.h"
//#include <Windows.h>
//#include "filesystem.h"
//#include "common.h"
//#include "config.h"
//


//void CB_AddMacro(CB_Context * context, wchar_t * name, wchar_t * expansion)
//{
//	CB_Macro macro;
//	macro.name = name;
//	macro.nameLength = lstrlenW(name);
//	macro.expansion = expansion;
//	macro.expansionLength = lstrlenW(expansion);
//
//	context->macros.push_back(macro);
//}
//
//CB_Macro * CB_FindMacro(CB_Context * context, wchar_t * name, size_t nameLength)
//{
//	for (int i = 0; i < context->macros.size(); ++i)
//	{
//		if (lstrcmpW(name, context->macros[i].name) == 0)
//		{
//			return &context->macros[i];
//		}
//	}
//
//	return NULL;
//}
//
//inline int CB__AddArg(CB_Context * context, CB_CommandArgument * arg, wchar_t * argStart, size_t argLength)
//{
//	if (argStart[0] == L'$')
//	{
//		CB_Macro * macro = CB_FindMacro(context, argStart + 1, argLength - 1);
//		if (macro == NULL)
//		{
//			static wchar_t damn[64];
//			wsprintfW(damn, L"Unknown macro \"%.*s\".", argLength - 1, argStart + 1);
//			goto whatever; // @Temporary
//			return 0;
//		}
//
//		arg->data = macro->name;
//		arg->length = macro->nameLength;
//	}
//	else
//	{
//		whatever:
//		arg->data = argStart;
//		arg->length = argLength;
//	}
//
//	return 1;
//}
//
//int  CB_EvaluateExpression(CB_Context * context, wchar_t * expression, size_t expressionLength)
//{
//	CB_CommandArgument args[16];
//
//	int numArgs = 0;
//	wchar_t c;
//	wchar_t* argStart = NULL;
//	size_t   argLength = 0;
//	for (size_t i = 0; i < expressionLength; ++i)
//	{
//		c = expression[i];
//		if (c != ' ' && c != '\t')
//		{
//			if (argStart == NULL)
//			{
//				argStart = expression + i;
//				argLength = 1;
//				continue;
//			}
//			else
//			{
//				++argLength;
//				continue;
//			}
//		}
//		else
//		{
//			if (argStart == NULL)
//			{
//				continue;
//			}
//			else
//			{
//				CB__AddArg(context, &args[numArgs++], argStart, argLength);
//				argStart = NULL;
//				argLength = 0;
//				continue;
//			}
//		}
//	}
//
//	if (argStart != NULL)
//	{
//		CB__AddArg(context, &args[numArgs++], argStart, argLength);
//		argStart = NULL;
//		argLength = 0;
//	}
//	
//	if (numArgs == 0)
//	{
//		return 0;
//	}
//
//	for (int i = 0; i < context->commands.size(); ++i)
//	{
//		CB_CommandDef* command = &context->commands[i];
//		if (args[0].length != command->nameLength) continue;
//		int cmp = wcsncmp(command->name, args[0].data, args[0].length);
//		if (cmp == 0)
//		{
//			command->callback(context, &args[1], numArgs - 1, command->userdata);
//			return 1;
//		}
//	}
//
//	return 0;
//}
//
//inline void CB_StringToWide(wchar_t* dest, const char* src, size_t length)
//{
//	while (length-- > 0) {
//		*dest++ = *src++;
//	}
//}
//
//wchar_t* CB_AllocateWideString(char* src, size_t length)
//{
//	wchar_t* name = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));
//	CB_StringToWide(name, src, length);
//	name[length] = '\0';
//	return name;
//}
//
//void __cdecl CB_GenericExecCallback(CB_Context * context, CB_CommandArgument * args, int numArgs, void* userdata)
//{
//	wchar_t* fileName = (wchar_t*)userdata;
//
//	STARTUPINFO i;
//	memset(&i, 0, sizeof(i));
//	i.cb = sizeof(i);
//	PROCESS_INFORMATION info;
//
//	CreateProcessW(fileName, L"", NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &i, &info);
//	CloseHandle(info.hProcess); // We don't care.
//	CloseHandle(info.hThread);
//}
//
//void CB_LoadCommands(CB_Context* context)
//{
//	uint32_t size;
//	char* text = (char*)ReadFileContents(L"D:/vlad/cb/cmds.txt", &size);
//
//	if (!text || size == 0) {
//		return;
//	}
//
//	ConfigState s;
//	InitConfigState(&s, text, size);
//
//	while (NextConfigEvent(&s)) {
//		switch (s.curr.type)
//		{
//			case ConfigEventType_KeyValue:
//			{
//				wchar_t* name = CB_AllocateWideString(s.curr.key, s.curr.keyLength);
//				wchar_t* path = CB_AllocateWideString(s.curr.value, s.curr.valueLength);
//				wchar_t* help = L"Execute this program";
//			
//				if (name[0] == L'$')
//				{
//					CB_AddMacro(context, name, path);
//					OutputDebugStringW(L"added macro ");
//					OutputDebugStringW(name);
//					OutputDebugStringW(L"\n");
//				}
//				else
//				{
//					CB_AddCommand(context, name, help, (CommandCallback)CB_GenericExecCallback, (void*)path);
//					OutputDebugStringW(L"added command ");
//					OutputDebugStringW(name);
//					OutputDebugStringW(L"\n");
//				}
//				break;
//			}
//			case ConfigEventType_Error:
//			{
//				__debugbreak();
//				break;
//			}
//		}
//	}
//
//	free(text);
//}
//
//void CB_InitCoreFunctionality()
//{
//}
