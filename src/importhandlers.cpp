// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include "importhandlers.h"
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/SAXException.hpp>

// ---------------------------------------------------------------------------
//  ImportHandlers: Constructors and Destructor
// ---------------------------------------------------------------------------
ImportHandlers::ImportHandlers() :

    el_amf(false)
  , el_object(0)
  , el_mesh(0)
  , el_vertices(0)
  , el_vertex(0)
  , el_coordinates(0)
  , el_volume(0)
  , el_triangle(0)
  , fSawErrors(false)
{
}

ImportHandlers::~ImportHandlers()
{
}


// ---------------------------------------------------------------------------
//  ImportHandlers: Implementation of the SAX DocumentHandler interface
// ---------------------------------------------------------------------------
void ImportHandlers::startElement(const   XMLCh* const   name
                                    ,       AttributeList&  /* attributes */)
{
    char* el = XMLString::transcode(name);
    if(0 == strcmp(el, "amf"))
        el_amf = true;
    else if(el_amf) {
        if(0 == strcmp(el, "object"))
            el_object++;
        else if(el_object == 1) {
            if(0 == strcmp(el, "mesh"))
                el_mesh++;
            else if(el_mesh == 1) {
                // through intro!!
                // now caching vertices
                if(0 == strcmp(el, "vertices"))
                    el_vertices++;
                else if(el_vertices == 1) {
                    if(0 == strcmp(el, "vertex"))
                        el_vertex++;
                    else if(el_vertex == 1) {
                        if(0 == strcmp(el, "coordinates"))
                            el_coordinates++;
                        else if(el_coordinates > 0) {
                            if(0 == strcmp(el, "x")) {
                                el_state = (XMLCh*)name;
                                el_coordinates++;
                            }
                            else if(0 == strcmp(el, "y")) {
                                el_state = (XMLCh*)name;
                                el_coordinates++;
                            }
                            else if(0 == strcmp(el, "z")) {
                                el_state = (XMLCh*)name;
                                el_coordinates++;
                            }
                        }
                    }
                }
                // now caching triangles
                if(0 == strcmp(el, "volume"))
                    el_volume++;
                else if(el_volume == 1) {
                    if(0 == strcmp(el, "triangle"))
                        el_triangle++;
                    else if(el_triangle > 0) {
                        if(0 == strcmp(el, "v1")) {
                            el_state = (XMLCh*)name;
                            el_triangle++;
                        }
                        else if(0 == strcmp(el, "v2")) {
                            el_state = (XMLCh*)name;
                            el_triangle++;
                        }
                        else if(0 == strcmp(el, "v3")) {
                            el_state = (XMLCh*)name;
                            el_triangle++;
                        }
                    }
                }
            }
        }
    }
    //fElementCount++;
    //fAttrCount += attributes.getLength();
}

void ImportHandlers::characters(  const   XMLCh* const    chars
                                    , const XMLSize_t      /* length */)
{
    char* state = XMLString::transcode(el_state);
    if(0 == strcmp("x", state)) {
        el_x = new XMLCh[XMLString::stringLen(chars)];
        XMLString::copyString(el_x, chars);
        //el_x = (XMLCh*)chars;
    }
    else if(0 == strcmp("y", state)) {
        el_y = new XMLCh[XMLString::stringLen(chars)];
        XMLString::copyString(el_y, chars);
    }
    else if(0 == strcmp("z", state)) {
        el_z = new XMLCh[XMLString::stringLen(chars)];
        XMLString::copyString(el_z, chars);
    }
    else if(0 == strcmp("v1", state)) {
        el_v1 = new XMLCh[XMLString::stringLen(chars)];
        XMLString::copyString(el_v1, chars);
    }
    else if(0 == strcmp("v2", state)) {
        el_v2 = new XMLCh[XMLString::stringLen(chars)];
        XMLString::copyString(el_v2, chars);
    }
    else if(0 == strcmp("v3", state)) {
        el_v3 = new XMLCh[XMLString::stringLen(chars)];
        XMLString::copyString(el_v3, chars);
    }
    //fCharacterCount += length;
}

void ImportHandlers::endElement(const XMLCh* const name)
{
    char* el = XMLString::transcode(name);
    if(0 == strcmp(el, "amf"))
        el_amf = false;
    else if(el_amf) {
        if(0 == strcmp(el, "object"))
            el_object--;
        else if(el_object > 0) {
            if(0 == strcmp(el, "mesh"))
                el_mesh--;
            else if(el_mesh > 0) {
                // through intro!!
                // now caching vertices
                if(0 == strcmp(el, "vertices"))
                    el_vertices--;
                else if(el_vertices > 0) {
                    if(0 == strcmp(el, "vertex")) {
                        vertex v;
                        v.x = XMLString::transcode(el_x);
                        v.y = XMLString::transcode(el_y);
                        v.z = XMLString::transcode(el_z);
                        vertices.push_back(v);
                        el_vertex--;
                    }
                    else if(el_vertex > 0) {
                        if(0 == strcmp(el, "coordinates"))
                            el_coordinates--;
                        else if(el_coordinates > 0) {
                            if(0 == strcmp(el, "x") && name == el_state) {
                                el_state = (XMLCh*)"";
                                el_coordinates--;
                            }
                            else if(0 == strcmp(el, "y") && name == el_state) {
                                el_state = (XMLCh*)"";
                                el_coordinates--;
                            }
                            else if(0 == strcmp(el, "z") && name == el_state) {
                                el_state = (XMLCh*)"";
                                el_coordinates--;
                            }
                        }
                    }
                }
                // now caching triangles
                if(0 == strcmp(el, "volume"))
                    el_volume--;
                else if(el_volume > 0) {
                    if(0 == strcmp(el, "triangle")) {
                        triangle t;
                        t.vs1 = XMLString::transcode(el_v1);
                        t.vs2 = XMLString::transcode(el_v2);
                        t.vs3 = XMLString::transcode(el_v3);
                        triangles.push_back(t);
                        el_triangle--;
                    }
                    else if(el_triangle > 0) {
                        if(0 == strcmp(el, "v1") && name == el_state) {
                            el_state = (XMLCh*)"";
                            el_triangle--;
                        }
                        else if(0 == strcmp(el, "v2") && name == el_state) {
                            el_state = (XMLCh*)"";
                            el_triangle--;
                        }
                        else if(0 == strcmp(el, "v3") && name == el_state) {
                            el_state = (XMLCh*)"";
                            el_triangle--;
                        }
                    }
                }
            }
        }
    }
    // No escapes are legal here
    //fFormatter << XMLFormatter::NoEscapes << gEndElement << name << chCloseAngle;
}

void ImportHandlers::ignorableWhitespace( const   XMLCh* const /* chars */
                                            , const XMLSize_t    /* length */)
{
    //fSpaceCount += length;
}

void ImportHandlers::resetDocument()
{
  el_amf = false;
  el_object = 0;
  el_mesh = 0;
  el_vertices = 0;
  el_vertex = 0;
  el_coordinates = 0;
  el_volume = 0;
  el_triangle = 0;
  el_state = (XMLCh*)"";
  el_x = (XMLCh*)"";
  el_y = (XMLCh*)"";
  el_z = (XMLCh*)"";
  el_v1 = (XMLCh*)"";
  el_v2 = (XMLCh*)"";
  el_v3 = (XMLCh*)"";
  vertices.clear();
  triangles.clear();
}


// ---------------------------------------------------------------------------
//  ImportHandlers: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------
void ImportHandlers::error(const SAXParseException& /*e*/)
{
    fSawErrors = true;
    // display error
    /*
    XERCES_STD_QUALIFIER cerr << "\nError at file " << StrX(e.getSystemId())
         << ", line " << e.getLineNumber()
         << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << XERCES_STD_QUALIFIER endl;
    */
}

void ImportHandlers::fatalError(const SAXParseException& /*e*/)
{
    fSawErrors = true;
    // display error
}

void ImportHandlers::warning(const SAXParseException& /*e*/)
{
    // display warning
}

void ImportHandlers::resetErrors()
{
    fSawErrors = false;
}
