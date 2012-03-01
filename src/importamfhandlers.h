// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/sax/HandlerBase.hpp>
#include <string>
#include <vector>

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

class ImportAmfHandlers : public HandlerBase
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ImportAmfHandlers();
    ~ImportAmfHandlers();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    std::vector<vertex> getVertices() const
    {
        return vertices;
    }

    std::vector<triangle> getTriangles() const
    {
        return triangles;
    }

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
    // -----------------------------------------------------------------------
    std::vector<vertex> vertices;
    std::vector<triangle> triangles;

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
