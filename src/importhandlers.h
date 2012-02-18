/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: ImportHandlers.hpp 679377 2008-07-24 11:56:42Z borisk $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/sax/HandlerBase.hpp>
#include <QVector>

XERCES_CPP_NAMESPACE_USE

XERCES_CPP_NAMESPACE_BEGIN
class AttributeList;
XERCES_CPP_NAMESPACE_END

struct triangle {
    std::string vs1;
    std::string vs2;
    std::string vs3;
};

struct vertex {
    std::string x;
    std::string y;
    std::string z;
};

class ImportHandlers : public HandlerBase
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ImportHandlers();
    ~ImportHandlers();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    QVector<vertex> getVertices() const
    {
        return vertices;
    }

    QVector<triangle> getTriangles() const
    {
        return triangles;
    }
    /*
    XMLSize_t getElementCount() const
    {
        return fElementCount;
    }

    XMLSize_t getAttrCount() const
    {
        return fAttrCount;
    }

    XMLSize_t getCharacterCount() const
    {
        return fCharacterCount;
    }

    bool getSawErrors() const
    {
        return fSawErrors;
    }

    XMLSize_t getSpaceCount() const
    {
        return fSpaceCount;
    }
*/

    // -----------------------------------------------------------------------
    //  Handlers for the SAX DocumentHandler interface
    // -----------------------------------------------------------------------
    void startElement(const XMLCh* const name, AttributeList& );
    void characters(const XMLCh* const chars, const XMLSize_t length);
    void endElement(const XMLCh* const name);
    void ignorableWhitespace(const XMLCh* const chars, const XMLSize_t length);
    void resetDocument();


    // -----------------------------------------------------------------------
    //  Handlers for the SAX ErrorHandler interface
    // -----------------------------------------------------------------------
    void warning(const SAXParseException& exc);
    void error(const SAXParseException& exc);
    void fatalError(const SAXParseException& exc);
    void resetErrors();


private:
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fAttrCount
    //  fCharacterCount
    //  fElementCount
    //  fSpaceCount
    //      These are just counters that are run upwards based on the input
    //      from the document handlers.
    //
    //  fSawErrors
    //      This is set by the error handlers, and is queryable later to
    //      see if any errors occured.
    // -----------------------------------------------------------------------
    QVector<vertex> vertices;
    QVector<triangle> triangles;

    XMLSize_t       el_amf;
    XMLSize_t       el_object;
    XMLSize_t       el_mesh;
    XMLSize_t       el_vertices;
    XMLSize_t       el_vertex;
    XMLSize_t       el_coordinates;
    XMLSize_t       el_volume;
    XMLSize_t       el_triangle;
    XMLCh*          el_state;
    XMLCh*          el_x;
    XMLCh*          el_y;
    XMLCh*          el_z;
    XMLCh*          el_v1;
    XMLCh*          el_v2;
    XMLCh*          el_v3;
    bool            fSawErrors;
};
