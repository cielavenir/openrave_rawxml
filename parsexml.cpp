/// partly copied libopenrave/libopenrave.cpp because it is not declared as OPENRAVE_API... ///

#define BOOST_SYSTEM_NO_DEPRECATED
#include <openrave/openrave.h>
#include <openrave/xmlreaders.h>
#include <cstdarg>

#define LIBXML_SAX1_ENABLED
#include <libxml/globals.h>
#include <libxml/xmlerror.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h> // only for xmlNewInputFromFile()
#include <libxml/tree.h>

#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>

namespace OpenRAVE {
namespace LocalXML {

void RaveXMLErrorFunc(void *ctx, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	RAVELOG_ERROR("XML Parse error: ");
	vprintf(msg,args);
	va_end(args);
}

struct XMLREADERDATA
{
	XMLREADERDATA(BaseXMLReader& reader, xmlParserCtxtPtr ctxt) : _reader(reader), _ctxt(ctxt) {
	}
	BaseXMLReader& _reader;
	BaseXMLReaderPtr _pdummy;
	xmlParserCtxtPtr _ctxt;
};

void DefaultStartElementSAXFunc(void *ctx, const xmlChar *name, const xmlChar **atts)
{
	AttributesList listatts;
	if( atts != NULL ) {
		for (int i = 0; (atts[i] != NULL); i+=2) {
			listatts.push_back(std::make_pair(std::string((const char*)atts[i]),std::string((const char*)atts[i+1])));
			std::transform(listatts.back().first.begin(), listatts.back().first.end(), listatts.back().first.begin(), ::tolower);
		}
	}

	XMLREADERDATA* pdata = (XMLREADERDATA*)ctx;
	std::string s = (const char*)name;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	if( !!pdata->_pdummy ) {
		RAVELOG_VERBOSE(str(boost::format("unknown field %s\n")%s));
		pdata->_pdummy->startElement(s,listatts);
	}
	else {
		BaseXMLReader::ProcessElement pestatus = pdata->_reader.startElement(s, listatts);
		if( pestatus != BaseXMLReader::PE_Support ) {
			// not handling, so create a temporary class to handle it
			pdata->_pdummy.reset(new DummyXMLReader(s,"(libxml)"));
		}
	}
}

void DefaultEndElementSAXFunc(void *ctx, const xmlChar *name)
{
	XMLREADERDATA* pdata = (XMLREADERDATA*)ctx;
	std::string s = (const char*)name;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	if( !!pdata->_pdummy ) {
		if( pdata->_pdummy->endElement(s) ) {
			pdata->_pdummy.reset();
		}
	}
	else {
		if( pdata->_reader.endElement(s) ) {
			//RAVEPRINT(L"%s size read %d\n", name, data->_ctxt->input->consumed);
			xmlStopParser(pdata->_ctxt);
		}
	}
}

void DefaultCharactersSAXFunc(void *ctx, const xmlChar *ch, int len)
{
	XMLREADERDATA* pdata = (XMLREADERDATA*)ctx;
	if( !!pdata->_pdummy ) {
		pdata->_pdummy->characters(std::string((const char*)ch, len));
	}
	else {
		pdata->_reader.characters(std::string((const char*)ch, len));
	}
}

bool xmlDetectSAX2(xmlParserCtxtPtr ctxt)
{
	if (ctxt == NULL) {
		return false;
	}
#ifdef LIBXML_SAX1_ENABLED
	if (ctxt->sax &&  ctxt->sax->initialized == XML_SAX2_MAGIC && (ctxt->sax->startElementNs != NULL || ctxt->sax->endElementNs != NULL)) {
		ctxt->sax2 = 1;
	}
#else
	ctxt->sax2 = 1;
#endif

	ctxt->str_xml = xmlDictLookup(ctxt->dict, BAD_CAST "xml", 3);
	ctxt->str_xmlns = xmlDictLookup(ctxt->dict, BAD_CAST "xmlns", 5);
	ctxt->str_xml_ns = xmlDictLookup(ctxt->dict, XML_XML_NAMESPACE, 36);
	if ( ctxt->str_xml==NULL || ctxt->str_xmlns==NULL || ctxt->str_xml_ns == NULL) {
		return false;
	}
	return true;
}

bool ParseXMLData(BaseXMLReader& reader, const char* buffer, int size)
{
	static xmlSAXHandler s_DefaultSAXHandler = { 0};
	if( size <= 0 ) {
		size = strlen(buffer);
	}
	if( !s_DefaultSAXHandler.initialized ) {
		// first time, so init
		s_DefaultSAXHandler.startElement = DefaultStartElementSAXFunc;
		s_DefaultSAXHandler.endElement = DefaultEndElementSAXFunc;
		s_DefaultSAXHandler.characters = DefaultCharactersSAXFunc;
		s_DefaultSAXHandler.error = RaveXMLErrorFunc;
		s_DefaultSAXHandler.initialized = 1;
	}

	xmlSAXHandlerPtr sax = &s_DefaultSAXHandler;
	int ret = 0;
	xmlParserCtxtPtr ctxt;

	ctxt = xmlCreateMemoryParserCtxt(buffer, size);
	if (ctxt == NULL) {
		return false;
	}
	if (ctxt->sax != (xmlSAXHandlerPtr) &xmlDefaultSAXHandler) {
		xmlFree(ctxt->sax);
	}
	ctxt->sax = sax;
	xmlDetectSAX2(ctxt);

	XMLREADERDATA readerdata(reader, ctxt);
	ctxt->userData = &readerdata;

	xmlParseDocument(ctxt);

	if (ctxt->wellFormed) {
		ret = 0;
	}
	else {
		if (ctxt->errNo != 0) {
			ret = ctxt->errNo;
		}
		else {
			ret = -1;
		}
	}
	if (sax != NULL) {
		ctxt->sax = NULL;
	}
	if (ctxt->myDoc != NULL) {
		xmlFreeDoc(ctxt->myDoc);
		ctxt->myDoc = NULL;
	}
	xmlFreeParserCtxt(ctxt);

	return ret==0;
}
}}
