//#pragma once
//#include "common.h"
//#include "strutils.h"
//
//enum ConfigEventType
//{
//	ConfigEventType_Group = 1,
//	ConfigEventType_Comment = 2,
//	ConfigEventType_KeyValue = 4,
//	ConfigEventType_Error = 8,
//	ConfigEventType_Eof = 16,
//};
//
//// config.cpp
//typedef struct
//{
//	enum ConfigEventType type;
//	union {
//		struct {
//			char* key;
//			uint32_t keyLength;
//			char* value;
//			uint32_t valueLength;
//		};
//		struct {
//			char* group;
//			uint32_t groupLength;
//		};
//		struct {
//			char* comment;
//			uint32_t commentLength;
//		};
//		struct {
//			char* error;
//			uint32_t errorLength;
//		};
//	};
//} ConfigEvent;
//
//typedef struct
//{
//	strutils::LineReader<AString> lineReader;
//	ConfigEvent curr;
//
//	int line;
//} ConfigState;
//
//void InitConfigState(ConfigState* s, char* source, size_t sourceLength);
//bool NextConfigEvent(ConfigState* s);
//
