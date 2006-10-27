/*******************************************************************************
 * Copyright (C) 2004-2006 Intel Corp. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  - Neither the name of Intel Corp. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Intel Corp. OR THE CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/

/**
 * @author Anas Nashif
 * @author Eugene Yarmosh
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <assert.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "u/libu.h"

#include "wsman-xml-api.h"
#include "wsman-soap.h"
#include "wsman-xml.h"
#include "wsman-xml-binding.h"


static void destroy_attr_private_data(void* data)
{
    if ( data )
        xmlFree(data);
}


static void destroy_node_private_data(void* _data)
{
    iWsNode* data = (iWsNode*)_data;
    if ( data )
    {
        // ??? TBD data->nsQNameList;
        if ( data->valText )
            xmlFree(data->valText);
        u_free(data);
    }
}

static void destroy_tree_private_data(xmlNode* node)
{
    while(node)
    {
        xmlAttrPtr attr = node->properties;

        if ( node->_private )
        {
            destroy_node_private_data(node->_private);
            node->_private = NULL;
        }

        while(attr)
        {
            if ( attr->_private )
            {
                destroy_attr_private_data(attr->_private);
                attr->_private = NULL;
            }
            attr = attr->next;
        }
        destroy_tree_private_data(node->children);
        node = node->next;
    }
}



void xml_parser_initialize(SoapH soap)
{
}

void xml_parser_destroy(SoapH soap)
{
}

int xml_parser_utf8_strlen (char *buf) {
    return xmlUTF8Strlen(BAD_CAST buf);
}

void xml_parser_doc_to_memory( WsXmlDocH doc, char** buf, int* ptrSize, char* encoding)
{
    if ( doc && buf && ptrSize )
        xmlDocDumpMemoryEnc(((iWsDoc*)doc)->parserDoc, 
                ( xmlChar **) buf, ptrSize, (!encoding) ? "UTF-8" : encoding);
}

void xml_parser_free_memory(void* ptr)
{
    if (ptr )
        xmlFree(ptr);
}

int xml_parser_create_doc(iWsDoc* wsDoc, char* rootName)
{
    int retVal = -1;
    xmlDocPtr doc;
    xmlNodePtr rootNode;

    if ( (doc = xmlNewDoc(BAD_CAST "1.0")) == NULL ||
            (rootNode = xmlNewNode(NULL, BAD_CAST rootName)) == NULL )
    {
        if ( doc )
            xmlFreeDoc(doc);
    } else {
        doc->_private = wsDoc;
        wsDoc->parserDoc = doc;
        xmlDocSetRootElement(doc, rootNode);
        retVal = 0;
    }

    return retVal;
}



void xml_parser_destroy_doc(iWsDoc* wsDoc)
{
    xmlDocPtr xmlDoc = (xmlDocPtr)wsDoc->parserDoc;
    if ( xmlDoc != NULL )
    {
        destroy_tree_private_data(xmlDocGetRootElement(xmlDoc));
        xmlFreeDoc(xmlDoc);
    }
}


WsXmlDocH xml_parser_get_doc(WsXmlNodeH node)
{
    xmlDocPtr xmlDoc = ((xmlDocPtr)node)->doc;
    return (WsXmlDocH)(!xmlDoc ? NULL : xmlDoc->_private);
}


WsXmlNodeH
xml_parser_get_root(WsXmlDocH doc)
{
    if ( ((iWsDoc*)doc)->parserDoc != NULL )
        return (WsXmlNodeH)xmlDocGetRootElement((xmlDocPtr)((iWsDoc*)doc)->parserDoc);
    return NULL;
}

WsXmlDocH
xml_parser_file_to_doc(SoapH soap, char* filename, char* encoding, unsigned long options)
{
    env_t* fw = (env_t*)soap;
    WsXmlDocH soapDoc = NULL;
    if (fw)
    {
        xmlDocPtr xmlDoc = xmlReadFile(filename, 
                encoding,
                XML_PARSE_NONET | XML_PARSE_NSCLEAN);
        if ( xmlDoc != NULL )
        {
            iWsDoc* iDoc;
            if ( (iDoc = (iWsDoc*)u_zalloc(sizeof(iWsDoc))) == NULL )
            {
                xmlFreeDoc(xmlDoc);
            }
            else
            {
                xmlDoc->_private = iDoc;
                iDoc->fw = fw;			
                iDoc->parserDoc = xmlDoc;
                soapDoc = (WsXmlDocH)iDoc;
            }
        }
    }
    return soapDoc;

}

WsXmlDocH 
xml_parser_memory_to_doc(SoapH soap,char* buf, int size, char* encoding, unsigned long options)
{	
    env_t* fw = (env_t*)soap;
    WsXmlDocH soapDoc = NULL;

    if ( buf && size && fw)
    {
        xmlDocPtr xmlDoc = xmlReadMemory(buf,
                size,
                NULL,
                encoding,
                XML_PARSE_NONET | XML_PARSE_NSCLEAN);
        if ( xmlDoc != NULL )
        {
            iWsDoc* iDoc;
            if ( (iDoc = (iWsDoc*)u_zalloc(sizeof(iWsDoc))) == NULL )
            {
                xmlFreeDoc(xmlDoc);
            }
            else
            {
                xmlDoc->_private = iDoc;
                iDoc->fw = fw;			
                iDoc->parserDoc = xmlDoc;
                soapDoc = (WsXmlDocH)iDoc;
            }
        }
    }
    return soapDoc;
}


char* xml_parser_node_query(WsXmlNodeH node, int what)
{
    char* ptr = NULL;
    xmlNodePtr xmlNode = (xmlNodePtr)node;
    iWsNode* wsNode = (iWsNode*)xmlNode->_private;

    switch(what)
    {
    case XML_TEXT_VALUE:
        if ( wsNode == NULL )
            xmlNode->_private = wsNode = u_zalloc(sizeof(iWsNode));

        if ( wsNode != NULL )
        {
            if ( wsNode->valText == NULL )
            {
                wsNode->valText =  (char *)xmlNodeGetContent(xmlNode);
            }
            ptr = wsNode->valText;
        }
        break;
    case XML_LOCAL_NAME:
        ptr = (char*)xmlNode->name;
        break;
    case XML_NS_URI:
        if ( xmlNode->ns != NULL )
            ptr = (char*)xmlNode->ns->href;
        break;
    case XML_NS_PREFIX:
        if ( xmlNode->ns != NULL )
            ptr = (char*)xmlNode->ns->prefix;
        break;
    default:
        break;
    }

    return ptr;
}


int xml_parser_node_set(WsXmlNodeH node, int what, char* str)
{
    int retVal = -1;
    xmlNodePtr xmlNode = (xmlNodePtr)node;
    iWsNode* wsNode = (iWsNode*)xmlNode->_private;
    xmlNsPtr xmlNs;

    switch(what)
    {
    case XML_TEXT_VALUE:
        if ( wsNode == NULL )
            xmlNode->_private = wsNode = u_zalloc(sizeof(iWsNode));

        if ( wsNode != NULL )
        {
            if ( wsNode->valText != NULL )
            {
                xmlFree(wsNode->valText);
                wsNode->valText = NULL;
            }
            xmlNodeSetContent(xmlNode, BAD_CAST str);
            retVal = 0;
        }
        break;

    case XML_LOCAL_NAME:
        xmlNodeSetName(xmlNode, BAD_CAST str);
        retVal = 0;
        break;

    case XML_NS_URI:
        if ( (xmlNs = (xmlNsPtr)xml_parser_ns_find(node, str, NULL, 1, 1)) != NULL )
        {
            xmlNode->ns = xmlNs;
            retVal = 0;
        }
        else
            retVal = 1;
        break;
    default:
        retVal = 1;
        break;
    }

    return retVal;
}



WsXmlNodeH xml_parser_node_get(WsXmlNodeH node, int which)
{
    xmlNodePtr xmlNode = NULL;
    xmlNodePtr base = (xmlNodePtr)node;

    switch (which)
    {
    case XML_ELEMENT_PARENT:
        xmlNode = base->parent;
        break;
    case XML_ELEMENT_NEXT:
        if ( (xmlNode = base->next) != NULL )
        {
            do
            {
                if ( xmlNode->type == XML_ELEMENT_NODE )
                    break;
            }
            while( (xmlNode = xmlNode->next) != NULL);
        }
        break;
    case XML_ELEMENT_PREV:
        if ( (xmlNode = base->prev) != NULL )
        {
            do
            {
                if ( xmlNode->type == XML_ELEMENT_NODE )
                    break;
            }
            while( (xmlNode = xmlNode->prev) != NULL);
        }
        break;
    case XML_LAST_CHILD:
    default:
        if ( which >= 0 || which == XML_LAST_CHILD )
        {
            int count = 0;
            xmlNode = base->children;

            while(xmlNode)
            {
                if ( xmlNode->type == XML_ELEMENT_NODE )
                {
                    if ( which == XML_LAST_CHILD && xmlNode->next == NULL )
                        break;

                    if ( count == which )
                        break;

                    count++;
                }
                xmlNode = xmlNode->next;
            }
        }
        else
        {
            assert(which >= 0);
        }
        break;
    }
    return (WsXmlNodeH)xmlNode;
}


WsXmlNsH 
xml_parser_ns_find(WsXmlNodeH node, 
                   char* uri, 
                   char* prefix, 
                   int bWalkUpTree, 
                   int bAddAtRootIfNotFound)
{
    xmlNodePtr xmlNode = (xmlNodePtr)node;
    xmlNsPtr xmlNs = NULL;

    while( xmlNode != NULL )
    {
        xmlNs = xmlNode->nsDef;
        while( xmlNs != NULL )
        {
            if ( uri )
            {
                if ( !strcmp((char *)xmlNs->href, uri) )
                    break;
            }
            else
                if ( prefix == NULL )
                {
                    if ( xmlNs->prefix == NULL )
                        break;
                }
                else
                    if ( xmlNs->prefix && !strcmp((char *)xmlNs->prefix, prefix) )
                    {
                        break;	
                    }
            xmlNs = xmlNs->next;
        }
        if ( xmlNs != NULL || !bWalkUpTree )
            break;	
        xmlNode = xmlNode->parent;
    }

    if ( xmlNs == NULL && bAddAtRootIfNotFound )
    {
        xmlNodePtr xmlRoot = xmlDocGetRootElement(((xmlDocPtr)node)->doc);
        char buf[12];

        if ( prefix == NULL )
        {
            ws_xml_make_default_prefix((WsXmlNodeH)xmlRoot, uri, buf, sizeof(buf));
            prefix = buf;
        }
        xmlNs = (xmlNsPtr)xml_parser_ns_add((WsXmlNodeH)xmlRoot, uri, prefix);
    }
    return (WsXmlNsH)xmlNs;
}

char*
xml_parser_ns_query(WsXmlNsH ns, int what)
{
    xmlNsPtr xmlNs = (xmlNsPtr)ns;
    char* ptr = NULL;

    switch(what)
    {
    case XML_NS_URI:
        ptr = (char*)xmlNs->href;
        break;
    case XML_NS_PREFIX:
        ptr = (char*)xmlNs->prefix;
        break;
    default:
        assert(what == XML_NS_URI);
        break;
    }
    return ptr;
}


WsXmlNsH
xml_parser_ns_add( WsXmlNodeH node, 
                   char* uri, 
                   char* prefix)
{
    xmlNsPtr xmlNs = NULL;
    if ( node && uri )
    {
        if ( (xmlNs = (xmlNsPtr)xml_parser_ns_find(node, uri, NULL, 0, 0)) != NULL )
        {
            if ( xmlNs->prefix != NULL )
            {
                xmlFree((char*)xmlNs->prefix);
                xmlNs->prefix = NULL;
            }

            if ( prefix != NULL )
            {
                xmlNs->prefix = xmlStrdup(BAD_CAST prefix);
            }
            // TBD: walk down the tree to update QName node and attr values 
        }
        else
        {
            xmlNs = xmlNewNs((xmlNodePtr)node, BAD_CAST uri, BAD_CAST prefix);
        }
    }
    return (WsXmlNsH)xmlNs;
}


int xml_parser_ns_remove(WsXmlNodeH node, char* nsUri)
{
    int retVal = -1;

    if ( node && nsUri )
    {
        xmlNodePtr xmlNode = (xmlNodePtr)node;
        xmlNsPtr xmlNs = xmlNode->nsDef;
        xmlNsPtr prevNs = NULL;

        while (xmlNs != NULL) 
        {
            if ( (xmlStrEqual(xmlNs->href, BAD_CAST nsUri)) )
            {
                break;
            }
            prevNs = xmlNs;
            xmlNs = xmlNs->next;
        }

        if ( xmlNs != NULL )
        {
            retVal = 0;
            if ( prevNs == NULL )
                xmlNode->nsDef = xmlNs->next;
            else
                prevNs->next = xmlNs->next;
            xmlFreeNs(xmlNs);
        }
        else
            retVal = 1;
    }
    return retVal;	
}



WsXmlNsH xml_parser_ns_get(WsXmlNodeH node, int which)
{
    xmlNodePtr xmlNode = (xmlNodePtr)node;
    xmlNsPtr xmlNs = NULL;

    if ( which >= 0 )
    {
        int count = 0;
        xmlNs = xmlNode->nsDef;
        while( xmlNs != NULL )
        {
            if ( which == count )
                break;	
            count++;
            xmlNs = xmlNs->next;
        }
    }
    else
    {
        assert(which >= 0); 
    }
    return (WsXmlNsH)xmlNs;
}


int get_ns_count_at_node(xmlNodePtr xmlNode)
{
    int count = 0;
    xmlNsPtr xmlNs = xmlNode->nsDef;

    while(xmlNs != NULL)
    {
        count++;
        xmlNs = xmlNs->next;
    }
    return count;
}


int xml_parser_get_count(WsXmlNodeH node, int what, int bWalkUpTree)
{
    int count = 0;
    xmlNodePtr xmlNode;
    xmlAttrPtr xmlAttr;

    switch (what)
    {
    case XML_COUNT_NODE:
        xmlNode = ((xmlNodePtr)node)->children;
        while(xmlNode)
        {
            if ( xmlNode->type == XML_ELEMENT_NODE )
                count++;
            xmlNode = xmlNode->next;
        }
        break;
    case XML_COUNT_ATTR:
        xmlAttr = ((xmlNodePtr)node)->properties;
        while(xmlAttr)
        {
            count++;
            xmlAttr = xmlAttr->next;
        }
        break;
    case XML_COUNT_NS:
        xmlNode = (xmlNodePtr)node;
        while( xmlNode != NULL )
        {
            count += get_ns_count_at_node(xmlNode);
            if ( !bWalkUpTree )
                break;
            xmlNode = xmlNode->parent;
        }
        break;
    default:
        assert(what == XML_COUNT_NODE || what == XML_COUNT_ATTR || what == XML_COUNT_NS);
        break;
    }

    return count;
}

static xmlNodePtr
make_new_xml_node( xmlNodePtr base, 
                   char* uri, 
                   char* name, 
                   char* value) 
{
    xmlNodePtr newNode = NULL;
    xmlNsPtr ns = NULL;

    if ( uri == NULL || 
            (ns = (xmlNsPtr)xml_parser_ns_find((WsXmlNodeH)base, uri, NULL, 1, 1)) != NULL )
    {
        if ( (newNode = xmlNewNode(ns, BAD_CAST name)) != NULL )
        {
            if ( value != NULL )
                xmlNodeSetContent(newNode, BAD_CAST  value);
            newNode->_private = u_zalloc(sizeof(iWsNode)); 
        }
    }
    return newNode;
}


WsXmlNodeH
xml_parser_node_add(WsXmlNodeH base, 
                    int where, 
                    char* nsUri, 
                    char* localName, 
                    char* value)
{
    xmlNodePtr xmlBase = (xmlNodePtr)base;
    xmlNodePtr newNode = 
        make_new_xml_node((where != XML_ELEMENT_NEXT && where != XML_ELEMENT_PREV) 
                ? xmlBase : xmlBase->parent, nsUri, localName, value); 
    if ( newNode )
    {
        switch(where)
        {
        case XML_ELEMENT_NEXT:
            xmlAddNextSibling((xmlNodePtr)base, newNode);
            break;
        case XML_ELEMENT_PREV:
            xmlAddPrevSibling((xmlNodePtr)base, newNode);
            break;
        case XML_LAST_CHILD:
        default:
            xmlAddChild((xmlNodePtr)base, newNode);
            break;
        }
    }
    return (WsXmlNodeH)newNode;

}

int xml_parser_node_remove(WsXmlNodeH node)
{
    destroy_node_private_data(((xmlNodePtr)node)->_private);
    xmlUnlinkNode((xmlNodePtr)node);
    xmlFreeNode((xmlNodePtr)node);
    return 0;
}


WsXmlAttrH 
xml_parser_attr_add( WsXmlNodeH node, 
                     char* uri, 
                     char* name, 
                     char* value)
{
    xmlNodePtr xmlNode = (xmlNodePtr)node;
    xmlNsPtr xmlNs = (xmlNsPtr)xml_parser_ns_find(node, uri, NULL, 1, 1);
    xmlAttrPtr xmlAttr	= (xmlAttrPtr)ws_xml_find_node_attr(node, uri, name);

    if ( xmlAttr != NULL )
        ws_xml_remove_node_attr((WsXmlAttrH)xmlAttr);

    if ( xmlNs == NULL ) 
        xmlAttr = xmlNewProp(xmlNode, BAD_CAST name, BAD_CAST value);
    else
        xmlAttr = xmlNewNsProp(xmlNode, xmlNs, BAD_CAST name, BAD_CAST value);

    if ( xmlAttr != NULL )
    {
        if ( xmlNs == NULL ) 
            xmlAttr->_private = xmlGetProp(xmlNode, BAD_CAST name); 
        else
            xmlAttr->_private = xmlGetNsProp(xmlNode, BAD_CAST name, xmlNs->href); 
    }

    return (WsXmlAttrH)xmlAttr;
}



int xml_parser_attr_remove(WsXmlAttrH attr)
{
    xmlAttrPtr xmlAttr = (xmlAttrPtr)attr;
    xmlNodePtr xmlNode = (xmlNodePtr)xmlAttr->parent;
    xmlAttrPtr xmlAttrPrev = (xmlNode->properties == xmlAttr) ? NULL : xmlNode->properties;

    while(xmlAttrPrev != NULL && xmlAttrPrev->next != xmlAttr)
    {
        xmlAttrPrev = xmlAttrPrev->next;
    }
    if ( xmlAttrPrev != NULL )
        xmlAttrPrev->next = xmlAttr->next;
    else
        xmlNode->properties = xmlAttr->next;

    xmlNode->parent = NULL;
    xmlNode->next = NULL;

    destroy_attr_private_data((xmlAttrPtr)attr);
    xmlFreeProp((xmlAttrPtr)attr);

    return 0;
}

char* xml_parser_attr_query(WsXmlAttrH attr, int what)
{
    char* ptr = NULL;
    xmlAttrPtr xmlAttr = (xmlAttrPtr)attr;
    switch(what)
    {
    case XML_LOCAL_NAME:
        ptr = (char*)xmlAttr->name;
        break;
    case XML_NS_URI:
        if ( xmlAttr->ns != NULL )
            ptr = (char*)xmlAttr->ns->href;
        break;
    case XML_NS_PREFIX:
        if ( xmlAttr->ns != NULL )
            ptr = (char*)xmlAttr->ns->prefix;
        break;
    case XML_TEXT_VALUE:
        if ( xmlAttr->_private == NULL )
        {
            if ( xmlAttr->ns == NULL ) 
                xmlAttr->_private = xmlGetProp(xmlAttr->parent, 
                        xmlAttr->name); 
            else
                xmlAttr->_private = xmlGetNsProp(xmlAttr->parent, 
                        xmlAttr->name, 
                        xmlAttr->ns->href); 
        }
        ptr = (char*)xmlAttr->_private;
        break;
    default:
        assert(what == XML_LOCAL_NAME);
        break;
    }
    return ptr;
}



WsXmlAttrH xml_parser_attr_get(WsXmlNodeH node, int which)
{
    xmlNodePtr xmlNode = (xmlNodePtr)node;
    xmlAttrPtr xmlAttr = NULL;

    switch(which)
    {
    case XML_LAST_CHILD:
    default:
        if ( which >= 0 || which == XML_LAST_CHILD )
        {
            int count = 0;
            xmlAttr = xmlNode->properties;

            while(xmlAttr)
            {
                if ( which == XML_LAST_CHILD && xmlAttr->next == NULL )
                    break;

                if ( which == count )
                    break;

                count++;

                xmlAttr = xmlAttr->next;
            }
        }
        else
        {
            assert(which >= 0 || which == XML_LAST_CHILD); 
        }
        break;
    }

    return (WsXmlAttrH)xmlAttr;
}



void 
xml_parser_element_dump(FILE* f, WsXmlDocH doc, WsXmlNodeH node) {

    xmlNodePtr n = (xmlNodePtr) node;
    xmlDocPtr d = (xmlDocPtr) doc;
    xmlElemDump(f, d, n );
}

void xml_parser_doc_dump(FILE* f, WsXmlDocH doc) {

    xmlDocPtr d = (xmlDocPtr)((iWsDoc*)doc)->parserDoc;
    xmlDocFormatDump(f, d, 1);
    return;
}


char *
xml_parser_get_xpath_value(WsXmlDocH doc, const char *expression)
{
    //int i;
    char *result = NULL;    
    xmlXPathObject *obj;
    xmlNodeSetPtr nodeset;
    xmlXPathContextPtr ctxt;
    xmlNsPtr *nsList, *cur;
    
    xmlDocPtr d = (xmlDocPtr)((iWsDoc*)doc)->parserDoc; 
                    
    ctxt = xmlXPathNewContext(d);
    if (ctxt == NULL) {
        error("failed while creating xpath context");
        return NULL;
    }
    nsList = xmlGetNsList(d, (xmlNodePtr )xml_parser_get_root(doc));
    if (nsList == NULL)
        return NULL;
  
    for (cur = nsList; *cur != NULL; cur++) {
        if(xmlXPathRegisterNs(ctxt, (*cur)->prefix, (*cur)->href) != 0) {
            //error("Error: unable to register NS with prefix=\"%s\" and href=\"%s\"", cur->prefix, cur->href);        
            return NULL;
        }       
    }
    xmlFree(nsList);
    
#if 0    
    for(i = 0; g_wsNsData[i].uri != NULL; i++)
    {
        WsXmlNsData* nsd = &g_wsNsData[i];
        if(xmlXPathRegisterNs(ctxt, BAD_CAST nsd->prefix, BAD_CAST nsd->uri) != 0) {
            error("Error: unable to register NS with prefix=\"%s\" and href=\"%s\"", nsd->prefix, nsd->uri);        
            return NULL;
        }                   
    }
#endif
    
    obj = xmlXPathEvalExpression(BAD_CAST expression, ctxt);
    if (obj) {
        nodeset     = obj->nodesetval;
        if (nodeset)
            result = (char *) xmlNodeListGetString(d, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    } else {
        return NULL;
    }    
    xmlXPathFreeObject (obj);
    xmlXPathFreeContext(ctxt);
    return result;  
}


int xml_parser_check_xpath(WsXmlNodeH node, char * xpath_expr) {

    xmlDocPtr doc;
    xmlNodePtr rootNode = NULL;
 
    if ( (doc = xmlNewDoc(BAD_CAST "1.0")) == NULL ||
            (rootNode = xmlNewNode(NULL, BAD_CAST "resource")) == NULL )
    {
        if ( doc )
            xmlFreeDoc(doc);
    } else {
        xmlDocSetRootElement(doc, rootNode);
    }    
    ws_xml_duplicate_tree((WsXmlNodeH)xmlDocGetRootElement(doc), node);
    if (rootNode) {
        xmlDocFormatDump(stdout, doc, 1);
        xmlFreeDoc(doc);
    }

    return 0;
}

