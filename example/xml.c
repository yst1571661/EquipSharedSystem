/*
 * gjobread.c : a small test program for gnome jobs XML format
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@w3.org
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h> 

int XmlCreat(xmlChar *filename)
{
	      xmlDocPtr doc = NULL;       /* document pointer */ 
          xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;/* node pointers */ 
   // Creates a new document, a node and set it as a root node 
          doc = xmlNewDoc(BAD_CAST "1.0"); 
          root_node = xmlNewNode(NULL, BAD_CAST "root"); 
          xmlDocSetRootElement(doc, root_node); 
          //creates a new node, which is "attached" as child node of root_node node. 
      
          xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1); 
          /*free the document */ 
          xmlFreeDoc(doc); 
          xmlCleanupParser(); 
          xmlMemoryDump();//debug memory for regression tests 
          return(0); 
}

xmlChar *XmlRead(xmlChar *filename, int ChildrenNode1, int next1, int ChildrenNode2, int next2, xmlChar *record) {

    // COMPAT: Do not genrate nodes for formatting spaces 
    LIBXML_TEST_VERSION
    xmlKeepBlanksDefault(0);
	xmlDocPtr doc = NULL;       // document pointer  
	xmlNodePtr root_node = NULL;
   // xmlDocPtr doc;
    xmlNsPtr ns;
    xmlNodePtr cur;
	xmlChar *ret = NULL;
	int i;

#ifdef LIBXML_SAX1_ENABLED
    /*
     * build an XML tree from a the file;
     */
    doc = xmlParseFile(filename);
    if (doc == NULL) 
	{	
		XmlCreat(filename);
		return NULL;
	}
#else
    /*
     * the library has been compiled without some of the old interfaces
     */
    return;
#endif /* LIBXML_SAX1_ENABLED */

    /*
     * Check the document is of the right kind
     */
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr,"empty document\n");
	xmlFreeDoc(doc);
	return NULL;
    }
    /*ns = xmlSearchNsByHref(doc, cur,
	    (const xmlChar *) "http://www.gnome.org/some-location");
    if (ns == NULL) {
        fprintf(stderr,
	        "document of the wrong type, GJob Namespace not found\n");
	xmlFreeDoc(doc);
	return;
    }*/
	
	for(i = 0; i < ChildrenNode1; i++)
	{//printf("\n-----ChildrenNode1 = %d -----\n", i);
		if (cur == NULL) 
		{
			fprintf(stderr,"\n----- cur == NULL -----\n");
			xmlFreeDoc(doc);
			return NULL;
		}
		cur = cur->xmlChildrenNode;
	}
	for(i = 0; i < next1; i++)
	{//printf("\n-----next1 = %d -----\n", i);
		if (cur == NULL) 
		{
			fprintf(stderr,"\n----- cur == NULL -----\n");
			xmlFreeDoc(doc);
			return NULL;
		}
		cur = cur->next;
	}
	for(i = 0; i < ChildrenNode2; i++)
	{
		if (cur == NULL) 
		{
			fprintf(stderr,"\n----- cur == NULL -----\n");
			xmlFreeDoc(doc);
			return NULL;
		}
		cur = cur->xmlChildrenNode;
	}
	for(i = 0; i < next2; i++)
	{
		if (cur == NULL) 
		{
			fprintf(stderr,"\n----- cur == NULL -----\n");
			xmlFreeDoc(doc);
			return NULL;
		}
		cur = cur->next;
	}

	
	if (cur != NULL)
	{
		 if ((!xmlStrcmp(cur->name, (xmlChar *) record))) //&& (cur->ns == ns))
		 {
			 ret = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			 //printf("%s: %s\n", record, ret);
			 //return ret;
		 }		
	}
	xmlFreeDoc(doc);
    xmlCleanupParser();
	//printf("Email: %s\n", ret);
    return ret;
}

int XmlWrite(xmlChar *filename, xmlChar *record) 
{ 
	xmlDocPtr doc = NULL;       // document pointer  
	xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;// node pointers  
	// Creates a new document, a node and set it as a root node 
	doc = xmlParseFile(filename);
	if (doc == NULL) 
	{
		doc = xmlNewDoc(BAD_CAST "1.0"); 
		root_node = xmlNewNode(NULL, BAD_CAST "root"); 
		xmlDocSetRootElement(doc, root_node); 
	}	
	else
	{
		root_node = xmlDocGetRootElement(doc);
		if (root_node == NULL) 
		{
			fprintf(stderr,"empty document\n");
			xmlFreeDoc(doc);
			return -1;
		}
	}
	//creates a new node, which is "attached" as child node of root_node node. 
	xmlNewChild(root_node, NULL, BAD_CAST "record",BAD_CAST record); 

	//Dumping document to stdio or file 
	xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1); 
	//free the document 
	xmlFreeDoc(doc); 
	xmlCleanupParser(); 
	xmlMemoryDump();//debug memory for regression tests 
	return(0); 
}

/*int XmlChange(xmlChar *filename, xmlChar *record) 
{ 
	xmlDocPtr doc = NULL;       // document pointer  
	xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;// node pointers  
	// Creates a new document, a node and set it as a root node 
	doc = xmlParseFile(filename);
	if (doc == NULL) 
	{
		doc = xmlNewDoc(BAD_CAST "1.0"); 
		root_node = xmlNewNode(NULL, BAD_CAST "root"); 
		xmlDocSetRootElement(doc, root_node); 
		xmlNewChild(root_node, NULL, BAD_CAST "record",BAD_CAST record);
	}	
	else
	{
		root_node = xmlDocGetRootElement(doc);
		if (root_node == NULL) 
		{
			fprintf(stderr,"empty document\n");
			xmlFreeDoc(doc);
			return -1;
		}
		node = root_node->xmlChildrenNode; 
		if (node == NULL) 
		{
			//fprintf(stderr,"node == NULL\n");
			//xmlFreeDoc(doc);
			//return -1;
			xmlNewChild(root_node, NULL, BAD_CAST "record",BAD_CAST record);
		}
		else
		{
			xmlNodeSetContent(node, (const xmlChar *)record);
		}
	}
	//creates a new node, which is "attached" as child node of root_node node.
	
	//xmlNewChild(root_node, NULL, BAD_CAST "record",BAD_CAST record); 
    
	//Dumping document to stdio or file 
	xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1); 
	//free the document 
	xmlFreeDoc(doc); 
	xmlCleanupParser(); 
	xmlMemoryDump();//debug memory for regression tests 
	return(0); 
}*/

int XmlChange(xmlChar *filename, int ChildrenNode1, int next1, int ChildrenNode2, int next2, xmlChar *recordtag, xmlChar *record) 
{
// COMPAT: Do not genrate nodes for formatting spaces 
    LIBXML_TEST_VERSION
    xmlKeepBlanksDefault(0);
	xmlDocPtr doc = NULL;       // document pointer  
	xmlNodePtr root_node = NULL;
   // xmlDocPtr doc;
    xmlNsPtr ns;
    xmlNodePtr cur;
	xmlChar *ret = NULL;
	int i;

#ifdef LIBXML_SAX1_ENABLED
    /*
     * build an XML tree from a the file;
     */
    doc = xmlParseFile(filename);
    if (doc == NULL) 
	{	
		XmlCreat(filename);
		return -1;
	}
#else
    /*
     * the library has been compiled without some of the old interfaces
     */
    return;
#endif /* LIBXML_SAX1_ENABLED */

    /*
     * Check the document is of the right kind
     */
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr,"empty document\n");
	xmlFreeDoc(doc);
	return -1;
    }
    /*ns = xmlSearchNsByHref(doc, cur,
	    (const xmlChar *) "http://www.gnome.org/some-location");
    if (ns == NULL) {
        fprintf(stderr,
	        "document of the wrong type, GJob Namespace not found\n");
	xmlFreeDoc(doc);
	return;
    }*/
	
	for(i = 0; i < ChildrenNode1; i++)
	{//printf("\n-----ChildrenNode1 = %d -----\n", i);
		if (cur == NULL) 
		{
			fprintf(stderr,"\n----- cur == NULL -----\n");
			xmlFreeDoc(doc);
			return -1;
		}
		cur = cur->xmlChildrenNode;
	}
	for(i = 0; i < next1; i++)
	{//printf("\n-----next1 = %d -----\n", i);
		if (cur == NULL) 
		{
			fprintf(stderr,"\n----- cur == NULL -----\n");
			xmlFreeDoc(doc);
			return -1;
		}
		cur = cur->next;
	}
	for(i = 0; i < ChildrenNode2; i++)
	{
		if (cur == NULL) 
		{
			fprintf(stderr,"\n----- cur == NULL -----\n");
			xmlFreeDoc(doc);
			return -1;
		}
		cur = cur->xmlChildrenNode;
	}
	for(i = 0; i < next2; i++)
	{
		if (cur == NULL) 
		{
			fprintf(stderr,"\n----- cur == NULL -----\n");
			xmlFreeDoc(doc);
			return -1;
		}
		cur = cur->next;
	}

	
	if (cur != NULL)
	{
		 if ((!xmlStrcmp(cur->name, (xmlChar *) recordtag))) //&& (cur->ns == ns))
		 {
			 //ret = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);		
			 xmlNodeSetContent(cur->xmlChildrenNode, (const xmlChar *)record);
			 //printf("%s: %s\n", record, ret);
			 //return ret;
		 }		
	}
	xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
	xmlFreeDoc(doc);
    xmlCleanupParser();
	xmlMemoryDump();//debug memory for regression tests 
	//printf("Email: %s\n", ret);
    return 0;
}
