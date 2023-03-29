/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: Kyle Gan
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"
#include <vector>
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#if defined(WIN32) || defined(_WIN32)
    #ifdef _DEBUG
        #pragma comment(lib, "glew32d.lib")
    #else
    #pragma comment(lib, "glew32.lib")
    #endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework 2";

// THE HANDLES
GLuint vertexPositionAndColorVBO;
GLuint splineVAO;
int numVerticesSpline;

OpenGLMatrix matrix;
BasicPipelineProgram* pipelineProgram;

// represents one control point along the spline 
struct Point
{
    double x;
    double y;
    double z;
};

struct Normal
{
    float x;
    float y;
    float z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
    int numControlPoints;
    Point* points;
};

// the spline array 
Spline* splines;
// total number of splines 
int numSplines;

// RIDE IS DONE
bool done = false;
float s = 0.5;

glm::mat4 basis(-s, 2 * s, -s, 0,
    2 - s, s - 3, 0, 1,
    s - 2, 3 - 2 * s, s, 0,
    s, -s, 0, 0);

vector<float> vertices;
float camera_u = 0;
float step = 0.01;

vector<glm::vec3> POS;
vector<glm::vec3> T;
vector<glm::vec3> N;
vector<glm::vec3> B;
int trackIndex = 0;
int threed = 0;

int cameraIndex = 0;
// IGNORE THIS IS JUST TO CREATE SCREENSHOTS
int imageNum = 1;
void saveScreenshot(const char* filename)
{
    unsigned char* screenshotData = new unsigned char[windowWidth * windowHeight * 3];
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

    ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

    if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
        cout << "File " << filename << " saved successfully." << endl;
    else cout << "Failed to save file " << filename << '.' << endl;

    delete[] screenshotData;
}

void displayFunc()
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();

    //for (int i = 0 ; i < )
    matrix.LookAt(
        POS[cameraIndex].x + 0.5f * B[cameraIndex].x, POS[cameraIndex].y + 0.5f * B[cameraIndex].y, POS[cameraIndex].z + 0.5f * B[cameraIndex].z,
        POS[cameraIndex].x + T[cameraIndex].x + 0.5f * B[cameraIndex].x, POS[cameraIndex].y + T[cameraIndex].y + 0.5f * B[cameraIndex].y, POS[cameraIndex].z + T[cameraIndex].z + 0.5f * B[cameraIndex].z,
        B[cameraIndex].x, B[cameraIndex].y, B[cameraIndex].z
    );

    // ORDER OF TRANSFORMATIONS MATTER

    float modelViewMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetMatrix(modelViewMatrix);

    float projectionMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(projectionMatrix);


    pipelineProgram->Bind(); 
    pipelineProgram->SetModelViewMatrix(modelViewMatrix);
    pipelineProgram->SetProjectionMatrix(projectionMatrix);

    glBindVertexArray(splineVAO);
    glDrawArrays(GL_LINE_STRIP, 0, numVerticesSpline);
    glBindVertexArray(0);

    glutSwapBuffers();
}

void idleFunc()
{

    if (cameraIndex < T.size() - 1 && threed == 10)
    {
        cameraIndex++;
        threed = 0;
        cout << "setting " << cameraIndex;
    }
    else if (cameraIndex < T.size() - 1)
    {
        threed++;
    }
    glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
    glViewport(0, 0, w, h);
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.LoadIdentity();
    const float zNear = 0.1f;
    const float zFar = 10000.0f;
    const float humanFieldOfView = 60.0f;
    matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void keyboardFunc(unsigned char key, int x, int y)
{
    // ALL THE MODES ARE IN HERE
    switch (key)
    {
    case 27: // ESC key
        exit(0); // exit the program
        break;

    case ' ':
        cout << "You pressed the spacebar." << endl;
        break;

    case 'x':
    {
        string s = to_string(imageNum) + ".jpg";
        saveScreenshot(s.c_str());
        imageNum++;
        break;
    }
    }
}

int loadSplines(char* argv)
{
    char* cName = (char*)malloc(128 * sizeof(char));
    FILE* fileList;
    FILE* fileSpline;
    int iType, i = 0, j, iLength;

    // load the track file 
    fileList = fopen(argv, "r");
    if (fileList == NULL)
    {
        printf("can't open file\n");
        exit(1);
    }

    // stores the number of splines in a global variable 
    fscanf(fileList, "%d", &numSplines);

    splines = (Spline*)malloc(numSplines * sizeof(Spline));

    // reads through the spline files 
    for (j = 0; j < numSplines; j++)
    {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");

        if (fileSpline == NULL)
        {
            printf("can't open file\n");
            exit(1);
        }

        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);

        // allocate memory for all the points
        splines[j].points = (Point*)malloc(iLength * sizeof(Point));
        splines[j].numControlPoints = iLength;

        // saves the data to the struct
        while (fscanf(fileSpline, "%lf %lf %lf",
            &splines[j].points[i].x,
            &splines[j].points[i].y,
            &splines[j].points[i].z) != EOF)
        {
            i++;
        }
    }

    free(cName);

    return 0;
}


// RECURSIVE SUBDIVISION EXTRA CREDIT
//void subdivide(float u0, float u1, float maxlinelength, vector<float>& vertices, vector<float>& colors, glm::mat3x4& product)
//{
//
//    float umid = (u0 + u1) / 2;
//    glm::vec4 u0_vector(powf(u0, 3), powf(u0, 2), powf(u0, 1), 1);
//    glm::vec4 u1_vector(powf(u1, 3), powf(u1, 2), powf(u1, 1), 1);
//
//    glm::vec3 vert0 = u0_vector * product;
//    glm::vec3 vert1 = u1_vector * product;
//
//    if (glm::distance(vert0, vert1) > maxlinelength)
//    {
//        subdivide(u0, umid, maxlinelength, vertices, colors, product);
//        subdivide(umid, u1, maxlinelength, vertices, colors, product);
//    }
//    else
//    {
//        vertices.push_back(vert0.x);
//        vertices.push_back(vert0.y);
//        vertices.push_back(vert0.z);
//        for (int i = 0; i < 4; ++i)
//        {
//            colors.push_back(1.0);
//        }
//        glm::vec4 u1_vector(3 * powf(u1, 3), powf(u1, 2), powf(u1, 1), 1);
//
//
//        vertices.push_back(vert1.x);
//        vertices.push_back(vert1.y);
//        vertices.push_back(vert1.z);
//        for (int i = 0; i < 4; ++i)
//        {
//            colors.push_back(1.0);
//        }
//
//    }
//}

//void level1Recursive()
//{
//    vector<float> colors;
//    for (int i = 0; i < numSplines; ++i)
//    {
//        Spline spl = splines[i];
//        for (int j = 1; j < spl.numControlPoints - 2; ++j)
//        {
//            glm::mat3x4 control(spl.points[j - 1].x, spl.points[j].x, spl.points[j + 1].x, spl.points[j + 2].x,
//                                spl.points[j - 1].y, spl.points[j].y, spl.points[j + 1].y, spl.points[j + 2].y,
//                                spl.points[j - 1].z, spl.points[j].z, spl.points[j + 1].z, spl.points[j + 2].z);
//
//
//            subdivide(0, 1, 0.01, vertices, colors, basis*control);
//        }
//    }
//    for (int i = 0; i < 100; i++)
//    {
//        cout << vertices[i] << " ";
//    }
//
//    glGenBuffers(1, &vertexPositionAndColorVBO);
//    glBindBuffer(GL_ARRAY_BUFFER, vertexPositionAndColorVBO);
//
//    numVerticesSpline = vertices.size()/3;
//    const int numBytesInPositions = vertices.size() * sizeof(float);
//    const int numBytesInColors = colors.size() * sizeof(float);
//    glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
//    glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, vertices.data());
//    glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, colors.data());
//
//
//    glGenVertexArrays(1, &splineVAO);
//    glBindVertexArray(splineVAO);
//    glBindBuffer(GL_ARRAY_BUFFER, vertexPositionAndColorVBO);
//
//    const GLuint locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
//    glEnableVertexAttribArray(locationOfPosition);
//    const int stride = 0;
//    const GLboolean normalized = GL_FALSE;
//    glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0);
//
//    const GLuint locationOfColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
//    glEnableVertexAttribArray(locationOfColor);
//    glVertexAttribPointer(locationOfColor, 4, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesInPositions);
//}


void level1()
{
    vector<float> vertices;
    vector<float> colors;
        for (int i = 0; i < numSplines; ++i)
        {
            Spline spl = splines[i];
            for (int j = 1; j < spl.numControlPoints - 2; ++j)
            {
                glm::mat3x4 control(spl.points[j - 1].x, spl.points[j].x, spl.points[j + 1].x, spl.points[j + 2].x,
                                    spl.points[j - 1].y, spl.points[j].y, spl.points[j + 1].y, spl.points[j + 2].y,
                                    spl.points[j - 1].z, spl.points[j].z, spl.points[j + 1].z, spl.points[j + 2].z);
                
                for (float u = 0; u < 1.0; u += 0.01f)
                {
                    glm::vec4 u0_vector(powf(u, 3), powf(u, 2), powf(u, 1), 1);
                    glm::vec4 t0_vector(powf(u, 2) * 3, 2 * u, 1, 0);

                    glm::mat3x4 product = (basis * control);
                    glm::vec3 vert0 = u0_vector * (product);
                    glm::vec3 tang0 = t0_vector * product;

                    vertices.push_back(vert0.x);
                    vertices.push_back(vert0.y);
                    vertices.push_back(vert0.z);
                    POS.push_back(vert0);
                    for (int i = 0; i < 4; ++i)
                    {
                        colors.push_back(1.0);
                    }
                    if (!T.size())
                    {
                        glm::vec3 arbitrary = glm::normalize(glm::vec3( .5, 2, 0.8 ));
                        T.push_back(glm::normalize(tang0));
                        glm::vec3 norm = glm::normalize(glm::cross(tang0, arbitrary));
                        N.push_back(norm);
                        B.push_back(glm::normalize(glm::cross(tang0, norm)));
                        trackIndex++;
                    }
                    else
                    {
                        T.push_back(glm::normalize(tang0));
                        glm::vec3 norm = glm::normalize(glm::cross(B[trackIndex - 1], tang0));
                        N.push_back(norm);
                        B.push_back(glm::normalize(glm::cross(tang0, norm)));
                    }
                }

                glGenBuffers(1, &vertexPositionAndColorVBO);
                glBindBuffer(GL_ARRAY_BUFFER, vertexPositionAndColorVBO);
                
                numVerticesSpline = vertices.size()/3;
                const int numBytesInPositions = vertices.size() * sizeof(float);
                const int numBytesInColors = colors.size() * sizeof(float);
                glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, vertices.data());
                glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, colors.data());
                
                
                glGenVertexArrays(1, &splineVAO);
                glBindVertexArray(splineVAO);
                glBindBuffer(GL_ARRAY_BUFFER, vertexPositionAndColorVBO);
                
                const GLuint locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
                glEnableVertexAttribArray(locationOfPosition);
                const int stride = 0;
                const GLboolean normalized = GL_FALSE;
                glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0);
                
                const GLuint locationOfColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
                glEnableVertexAttribArray(locationOfColor);
                glVertexAttribPointer(locationOfColor, 4, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesInPositions);
            }
        }
        cout << vertices.size() << " " << T.size() << " " << B.size() << endl;
}

glm::vec3 getVertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec3& bino)
{
    return pos + 0.02f * norm + 0.02f * bino;
}
void level3()
{
    glm::vec3 v0, v1, v2, v3, v4, v5, v6, v7;
    
    for (int i = 0; i < POS.size(); ++i)
    {
        v0 = getVertex(POS[i], -1.0f * N[i], B[i]);
        v1 = getVertex(POS[i], N[i], B[i]);
        v2 = getVertex(POS[i], N[i], -1.0f*B[i]);
        v3 = getVertex(POS[i], -1.0f * N[i], -1.0f*B[i]);

        i++;
        v4 = getVertex(POS[i], -1.0f * N[i], B[i]);
        v5 = getVertex(POS[i], N[i], B[i]);
        v6 = getVertex(POS[i], N[i], -1.0f * B[i]);
        v7 = getVertex(POS[i], -1.0f * N[i], -1.0f * B[i]);
        i--;

    }
}

void initScene(int argc, char* argv[])
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.
    glEnable(GL_DEPTH_TEST);
    pipelineProgram = new BasicPipelineProgram;
    int ret = pipelineProgram->Init(shaderBasePath);
    if (ret != 0)
    {
        abort();
    }
    pipelineProgram->Bind();

    level1();
    std::cout << "GL error: " << glGetError() << std::endl;
}

int initTexture(const char* imageFilename, GLuint textureHandle)
{
    // read the texture image
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(imageFilename, &imgFormat);

    if (err != ImageIO::OK)
    {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }

    // check that the number of bytes is a multiple of 4
    if (img.getWidth() * img.getBytesPerPixel() % 4)
    {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }

    // allocate space for an array of pixels
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

    // fill the pixelsRGBA array with the image pixels
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++)
        {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }

    // bind the texture
    glBindTexture(GL_TEXTURE_2D, textureHandle);

    // initialize the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

    // generate the mipmaps for this texture
    glGenerateMipmap(GL_TEXTURE_2D);

    // set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // query support for anisotropic texture filtering
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // set anisotropic texture filtering
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

    // query for any errors
    GLenum errCode = glGetError();
    if (errCode != 0)
    {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }

    // de-allocate the pixel array -- it is no longer needed
    delete[] pixelsRGBA;

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }

    cout << "Initializing GLUT..." << endl;
    glutInit(&argc, argv);

    cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(windowTitle);

    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif
    glutDisplayFunc(displayFunc);
    glutIdleFunc(idleFunc);
    glutReshapeFunc(reshapeFunc);
    glutKeyboardFunc(keyboardFunc);

    loadSplines(argv[1]);

    printf("Loaded %d spline(s).\n", numSplines);
    for (int i = 0; i < numSplines; i++)
        printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);


#ifdef __APPLE__
#else
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
        cout << "error: " << glewGetErrorString(result) << endl;
        exit(EXIT_FAILURE);
    }
#endif
    initScene(argc, argv);
    glutMainLoop();

    return 0;
}