//#include "common.h"
//#include "strutils.h"
//#include "config.h"
//#include <string.h>
//
//using namespace strutils;
//
//void InitConfigState(ConfigState* s, const AString& source)
//{
//	s->lineReader.setSource(source);
//	s->line = 0;
//}
//
//bool NextConfigEvent(ConfigState* s) {
//	AString line;
//
//	skipEmptyLine:
//	if (!s->lineReader.nextLine(&line))
//	{
//		return 0;
//	}
//	++s->line;
//
//	strutils::trimChars(&line.data, &line.count);
//	if (line.count == 0)
//	{
//		goto skipEmptyLine;
//	}
//
//	if (line[0] == ':')
//	{
//		if (length == 1)
//		{
//			s->curr.type = ConfigEventType_Error;
//			s->curr.error = "Group is empty";
//			s->curr.errorLength = strlen(s->curr.error);
//			return 1;
//		}
//		else
//		{
//			s->curr.type = ConfigEventType_Group;
//			s->curr.group = &line[1];
//			s->curr.groupLength = length - 1;
//			return 1;
//		}
//	}
//	else if (line[0] == '#')
//	{
//		s->curr.type = ConfigEventType_Comment;
//		return 1; // @TODO;
//	}
//	else
//	{
//		int keyLength = strutils::numCharsBeforeSpace(line, length);
//		int space = strutils::numSpacesBeforeChar(line + keyLength, length - keyLength);
//		int valueLength = length - (keyLength + space);
//		s->curr.type = ConfigEventType_KeyValue;
//		s->curr.key = line;
//		s->curr.keyLength = keyLength;
//		s->curr.value = line + keyLength + space;
//		s->curr.valueLength = valueLength;
//		return 1;
//	}
//
//	return 0;
//}
