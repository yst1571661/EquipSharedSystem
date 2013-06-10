#ifndef _XML_H
#define _XML_H

#include <string.h>
#include <stdlib.h>


#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h> 

xmlChar *XmlRead(xmlChar *filename, int ChildrenNode1, int next1, int ChildrenNode2, int next2, xmlChar *record);


#endif
