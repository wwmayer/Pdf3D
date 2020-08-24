
/*
 The application mathgl (see http://archive.ubuntu.com/ubuntu/pool/universe/m/mathgl/mathgl_2.3.4.orig.tar.gz)
 has the option to export to a .prc or diectly to a .pdf file (see prc.cpp)

 Links:
 https://sourceforge.net/p/libharu/patches/8/
 https://sourceforge.net/p/libharu/discussion/283687/thread/3c61d402/
 http://git.libharu.org
 https://github.com/ningfei/u3d
 https://github.com/ningfei/u3d/blob/master/Samples/TestScenes/box_base.idtf
 http://www.ecma-international.org/publications/standards/Ecma-363.htm

 https://github.com/vectorgraphics/asymptote
 https://github.com/XenonofArcticus/libPRC/tree/master/src/asymptote

 https://groups.google.com/forum/?_escaped_fragment_=topic/mathgl/wj3-E19XS5o#!topic/mathgl/wj3-E19XS5o
 http://www.okino.com/conv/imp_u3d.htm

 IDTFConverter.exe  -en 1 -rzf 0 -pq 500 -input test.idtf -output test.u3d
*/

#ifdef WIN32
# pragma warning(disable : 4244)
# define NOMINMAX
# include <Windows.h>
# define USE_WIDE_CHAR
#endif

#include <oPRCFile.h>
#include "Stream.h"
#include "Swap.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

#include <setjmp.h>
#include "hpdf.h"
#include "hpdf_u3d.h"


#ifdef USE_WIDE_CHAR
typedef std::wstring String;
String PATHSEP = L"/\\";
#else
typedef std::string String;
String PATHSEP = "/\\";
#endif


// forward declarations
struct BoundingBox;
int convertPdf(const std::string& prcData, const String& pdfFile, BoundingBox bbox);

struct BoundingBox {
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
};

struct Mesh {
    std::vector<float> pointArray;
    std::vector<unsigned long> facetArray;
    BoundingBox bbox;
};

std::string narrow(const std::wstring& str)
{
    std::ostringstream stm;
    const std::ctype<char>& ctfacet = std::use_facet< std::ctype<char> >(stm.getloc());
    for (size_t i=0; i<str.size(); ++i)
        stm << ctfacet.narrow(str[i], 0);
    return stm.str();
}

void loadMesh(String inputName, Mesh& mesh)
{
    std::ifstream istr(inputName.c_str(),
        std::ios_base::in | std::ios_base::binary);
    if (!istr || istr.bad())
        return;

    // get header
    Base::InputStream str(istr);

    // Read the header with a "magic number" and a version
    uint32_t magic, version, swap_magic, swap_version;
    str >> magic >> version;
    swap_magic = magic; Base::SwapEndian(swap_magic);
    swap_version = version; Base::SwapEndian(swap_version);
    uint32_t open_edge = 0xffffffff; // value to mark an open edge

    // is it the new or old format?
    bool new_format = false;
    if (magic == 0xA0B0C0D0 && version == 0x010000) {
        new_format = true;
    }
    else if (swap_magic == 0xA0B0C0D0 && swap_version == 0x010000) {
        new_format = true;
        str.setByteOrder(Base::Stream::BigEndian);
    }

    if (new_format) {
        char szInfo[256];
        istr.read(szInfo, 256);

        // read the number of points and facets
        uint32_t uCtPts=0, uCtFts=0;
        str >> uCtPts >> uCtFts;

        // read the data
        std::vector<float> pointArray;
        pointArray.reserve(3 * uCtPts);
        for (uint32_t i = 0; i < uCtPts; ++i) {
            float x,y,z;
            str >> x >> y >> z;
            pointArray.push_back(x);
            pointArray.push_back(y);
            pointArray.push_back(z);
        }

        std::vector<unsigned long> facetArray;
        facetArray.reserve(3 * uCtFts);
        for (uint32_t i = 0; i < uCtFts; ++i) {
            uint32_t v1, v2, v3;
            str >> v1 >> v2 >> v3;
            facetArray.push_back(v1);
            facetArray.push_back(v2);
            facetArray.push_back(v3);

            // The neighour indices
            str >> v1 >> v2 >> v3;
        }

        BoundingBox box;
        str >> box.minX >> box.maxX;
        str >> box.minY >> box.maxY;
        str >> box.minZ >> box.maxZ;

        mesh.bbox = box;
        mesh.pointArray.swap(pointArray);
        mesh.facetArray.swap(facetArray);
    }
}

BoundingBox addMeshToPrc(String input, oPRCFile* prcFile, float alpha)
{
    Mesh mesh;
    loadMesh(input, mesh);

#if 1
    const PRCmaterial materialMathGL(
        RGBAColour(0.1,0.1,0.1,1), // ambient
        RGBAColour(0.8,0.8,0.8,1), // diffuse
        RGBAColour(0.1,0.1,0.1,1), // emissive
        RGBAColour(0.0,0.0,0.0,1), // spectral
        alpha,0.1 // alpha, shininess
        );
    uint32_t materialMathGLid = prcFile->addMaterial(materialMathGL);

    PRC3DTess *tess = new PRC3DTess();
    tess->crease_angle = 0.0;

    // Copy point coordinates
    tess->coordinates.insert(tess->coordinates.begin(), mesh.pointArray.begin(), mesh.pointArray.end());

    PRCTessFace *tessFace = new PRCTessFace();
    tessFace->number_of_texture_coordinate_indexes = 0;
    tessFace->start_triangulated = 0;
    tessFace->used_entities_flag = PRC_FACETESSDATA_Triangle;
    tessFace->is_rgba = false;

    tess->has_faces = true;

    // Copy and adjust face indices to correctly reference in a flat list
    tess->triangulated_index.reserve(mesh.facetArray.size());
    for (std::size_t index = 0; index < mesh.facetArray.size(); ++index) {
        tess->triangulated_index.push_back(3 * mesh.facetArray[index]);
    }

    tessFace->sizes_triangulated.push_back(static_cast<uint32_t>(mesh.facetArray.size()/3));
    tess->addTessFace(tessFace);

    uint32_t tess_index = prcFile->add3DTess(tess);
    prcFile->useMesh(tess_index, materialMathGLid);

    // set name
    std::size_t found = input.find_last_of(PATHSEP);
    String name = input.substr(found+1);
    PRCgroup &group = prcFile->findGroup();
#ifdef USE_WIDE_CHAR
    group.polymodels.back()->name = narrow(name);
#else
    group.polymodels.back()->name = name;
#endif

#else
    const uint32_t nP = (uint32_t)mesh.pointArray.size()/3;
    double (*P)[3] = new double[nP][3];
    for (uint32_t p_index = 0; p_index < nP; ++p_index) {
        P[p_index][0] = mesh.pointArray[p_index*3 + 0];
        P[p_index][1] = mesh.pointArray[p_index*3 + 1];
        P[p_index][2] = mesh.pointArray[p_index*3 + 2];
    }

    const uint32_t nI = (uint32_t)mesh.facetArray.size()/3;
    uint32_t (*PI)[3] = new uint32_t[nI][3];
    for(uint32_t f_index = 0; f_index < nI; ++f_index)
    {
        PI[f_index][0] = mesh.facetArray[f_index*3 + 0];
        PI[f_index][1] = mesh.facetArray[f_index*3 + 1];
        PI[f_index][2] = mesh.facetArray[f_index*3 + 2];
    }

    const uint32_t tess_index = prcFile->createTriangleMesh(nP, P, nI, PI, m1, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
    const PRCmaterial materialMathGL(
        RGBAColour(0.1,0.1,0.1,1), // ambient
        RGBAColour(1.0,1.0,1.0,1), // diffuse
        RGBAColour(0.1,0.1,0.1,1), // emissive
        RGBAColour(0.0,0.0,0.0,1), // spectral
        1.0,0.1 // alpha, shininess
        );

    prcFile->useMesh(tess_index, prcFile->addMaterial(materialMathGL));
    delete [] PI;
    delete [] P;
#endif

    return mesh.bbox;
}

int main(int argc, char** argv)
{
    /* check parameters */
    if (argc < 4) {
        printf ("mshtoprc infile(s) -o outfile.\n");
        return 1;
    }

#ifdef USE_WIDE_CHAR
    typedef std::wstring String;
    std::vector<String> args;
    std::wstring option = L"-o";

    LPWSTR *szArgList;
    int argCount;

    szArgList = CommandLineToArgvW(GetCommandLineW(), &argCount);
    if (szArgList == NULL) {
        printf ("Unable to parse command line.\n");
        return 1;
    }

    for(int i = 1; i < argCount; i++)
        args.push_back(std::wstring(szArgList[i]));

    LocalFree(szArgList);
#else
    typedef std::string String;
    std::vector<String> args;
    std::string option = "-o";

    for(int i = 1; i < argc; i++)
        args.push_back(std::string(argv[i]));
#endif

    std::stringstream ostr;//(std::ios_base::out | std::ios_base::binary);
    if (ostr.bad())
        return -1;

    String pdfName;
    oPRCFile* prcFile(new oPRCFile(ostr));
    if (prcFile == NULL)
        return -1;

    prcFile->groups.top().product_occurrence->name = "Mesh files";
    BoundingBox globalbox;
    globalbox.maxX = -FLT_MAX;
    globalbox.minX =  FLT_MAX;
    globalbox.maxY = -FLT_MAX;
    globalbox.minY =  FLT_MAX;
    globalbox.maxZ = -FLT_MAX;
    globalbox.minZ =  FLT_MAX;
    float alpha = argc > 4 ? 0.8f : 1.0f;
    for (std::size_t i=0; i<args.size(); i++) {
        String arg(args[i]);
        if (arg != option) {
            BoundingBox bbox = addMeshToPrc(arg, prcFile, alpha);
            globalbox.maxX = std::max<float>(globalbox.maxX, bbox.maxX);
            globalbox.maxY = std::max<float>(globalbox.maxY, bbox.maxY);
            globalbox.maxZ = std::max<float>(globalbox.maxZ, bbox.maxZ);
            globalbox.minX = std::min<float>(globalbox.minX, bbox.minX);
            globalbox.minY = std::min<float>(globalbox.minY, bbox.minY);
            globalbox.minZ = std::min<float>(globalbox.minZ, bbox.minZ);
        }
        else if (i+1 < args.size()) {
            pdfName = args[i+1];
            break;
        }
    }

    if (!(prcFile->finish()))
        return -1;

    std::string prcData = ostr.str();
    delete prcFile;

    if (!pdfName.empty())
        return convertPdf(prcData, pdfName, globalbox);

    return 0;
}

jmp_buf env;

#ifdef HPDF_DLL
void  __stdcall
#else
void
#endif
error_handler  (HPDF_STATUS   error_no,
                HPDF_STATUS   detail_no,
                void         *user_data)
{
    printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,
                (HPDF_UINT)detail_no);
    longjmp(env, 1);
}

int convertPdf(const std::string& prcData, const String& pdfFile, BoundingBox bbox)
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Rect rect = {50, 50, 800, 550};
    HPDF_U3D u3d;
    HPDF_Annotation annot;
    HPDF_Dict view;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        printf ("ERROR: cannot create pdf object.\n");
        return 1;
    }

    if (setjmp(env)) {
        HPDF_Free (pdf);
        return 1;
    }

    HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, "mshtoprc");

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE);

//    u3d = HPDF_LoadU3DFromFile (pdf, prcFile.c_str());
    u3d = HPDF_LoadU3DFromMem(pdf, reinterpret_cast<const HPDF_BYTE *>(prcData.c_str()), static_cast<HPDF_UINT>(prcData.size()));
    annot = HPDF_Page_Create3DAnnot (page, rect, HPDF_TRUE, HPDF_FALSE, u3d, NULL);
    view = HPDF_Page_Create3DView(page, u3d, annot, "View");
    HPDF_3DView_SetLighting(view, "CAD");
    HPDF_3DView_SetBackgroundColor(view, 0.5, 0.5, 0.5);

    float coox = 0.5f * (bbox.maxX + bbox.minX);
    float cooy = 0.5f * (bbox.maxY + bbox.minY);
    float cooz = 0.5f * (bbox.maxZ + bbox.minZ);
    float lenx = bbox.maxX - bbox.minX;
    float leny = bbox.maxY - bbox.minY;
    float lenz = bbox.maxZ - bbox.minZ;
    float radius = sqrt(lenx*lenx + leny*leny + lenz* lenz);

    HPDF_3DView_SetCamera(view, coox, cooy, cooz, 0, 0, 0, radius, 0);
    HPDF_U3D_SetDefault3DView(u3d, "View");

//    HPDF_SaveToFile (pdf, pdfFile.c_str());
    HPDF_SaveToStream(pdf);
    HPDF_UINT32 len = HPDF_GetStreamSize(pdf);
    std::vector<HPDF_BYTE> buf(len);
    HPDF_ReadFromStream(pdf, &(buf[0]), &len);

    /* clean up */
    HPDF_Free (pdf);

    std::ofstream ostr(pdfFile.c_str(), std::ios::out | std::ios::binary);
    ostr.write(reinterpret_cast<char*>(&(buf[0])), buf.size());
    ostr.close();

    return 0;
}
