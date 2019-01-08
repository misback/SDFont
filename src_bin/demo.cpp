#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <png.h>

#include "metrics_parser.hpp"
#include "texture_loader.hpp"
#include "runtime_helper.hpp"
#include "vanilla_shader_manager.hpp"

using namespace std;

/** @file demo.cpp
 *
 *  @brief visual demo for signed distant fonts.
 *
 *  @dependencies
 *    OpenGL 3.3 or later
 *    GLFW3
 *    GLEW
 *    GLM
 */


class GlfwManager  {

  public:

    GlfwManager( int requestedWindowWidth, int requestedWindowHeight );

    virtual ~GlfwManager();

    void configGLFW();

    int windowWidth()         const { return mWindowWidth; }

    int windowHeight()        const { return mWindowHeight; }

    int windowWidthInPixel()  const { return mWindowWidthInPixel; }

    int windowHeightInPixel() const { return mWindowHeightInPixel; }

    void update();


    /** @param: keyCode (in): GLFW_KEY_* defined in glfw3.h
     *
     *  @return true if the key is pressed now.
     *
     *  @reference /usr/local/include/GLFW/glfw3.h
     */
    bool isPressed( int keyCode ) const {

        if ( glfwGetKey( mWindow, GLFW_KEY_SPACE ) == GLFW_PRESS ) {

            return true;
        }
        else {
            return false;
        }
    }

    virtual bool shouldExit();

    virtual void callbackWindowSize(
        GLFWwindow* w,
        int         width,
        int         height
    );

    virtual  void callbackMouseButton(
        GLFWwindow* window,
        int         button,
        int         action,
        int         mods
    );

    virtual void callbackMousePos(
        GLFWwindow* window,
        double      xpos,
        double      ypos
    );

    virtual void callbackMouseWheel(
        GLFWwindow* window,
        double      xoffset,
        double      yoffset
    );

    virtual void callbackKey(
        GLFWwindow* window,
        int         key,
        int         scancode,
        int         action,
        int         mods
    );

    virtual void callbackChar(
        GLFWwindow* window,
        int         codepoint
    );


  private:

    GLFWwindow*   mWindow;
    int           mWindowWidth;
    int           mWindowHeight;
    int           mWindowWidthInPixel;
    int           mWindowHeightInPixel;

    static GlfwManager* mThisSingleton;

    static void dispatchWindowSize  ( GLFWwindow* w, int x, int y );
    static void dispatchMouseButton ( GLFWwindow* w, int b, int a, int m );
    static void dispatchMousePos    ( GLFWwindow* w, double x, double y );
    static void dispatchMouseWheel  ( GLFWwindow* w, double x, double y );
    static void dispatchKey         ( GLFWwindow* w,int k,int s,int a,int m );
    static void dispatchChar        ( GLFWwindow* w, int c );
};


GlfwManager* GlfwManager::mThisSingleton = nullptr;


GlfwManager::GlfwManager(

    int requestedWindowWidth,
    int requestedWindowHeight

)
{
    // Check for singleton.
    if ( mThisSingleton != nullptr )  {

        cerr << "Duplicated GlfwManager.\n";
    }

    // Initialise GLFW
    if ( !glfwInit() ) {

        cerr << "glfwInit() failed.\n";
    }

    glfwWindowHint( GLFW_SAMPLES,               4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE );

    mWindow = glfwCreateWindow(

                  requestedWindowWidth,
                  requestedWindowHeight,
                  "Test Visualiser Convex Hull",
                  NULL,
                  NULL

              );

    if ( mWindow == NULL ) {

        cerr << "glfwCreateWindow failed\n";
        glfwTerminate();
    }

    glfwMakeContextCurrent( mWindow );
    
    glfwGetFramebufferSize(

        mWindow,
        &mWindowWidthInPixel,
        &mWindowHeightInPixel

    );


    glfwGetWindowSize( mWindow, &mWindowWidth, &mWindowHeight );

    glewExperimental = true;

    if ( glewInit() != GLEW_OK ) {

        cerr << "Failed to initialize GLEW\n";;
        terminate();

    }

    glViewport(
        0,
        0,
        (GLsizei)mWindowWidthInPixel, 
        (GLsizei)mWindowHeightInPixel
    );

    GlfwManager::mThisSingleton = this;
}


void GlfwManager::configGLFW()
{

    glfwSetWindowSizeCallback
                         ( mWindow, (GLFWwindowsizefun)  dispatchWindowSize );

    glfwSetMouseButtonCallback 
                         ( mWindow, (GLFWmousebuttonfun) dispatchMouseButton );

    glfwSetCursorPosCallback   
                         ( mWindow, (GLFWcursorposfun)   dispatchMousePos );
                                        
    glfwSetScrollCallback( mWindow, (GLFWscrollfun)      dispatchMouseWheel );
                                        
    glfwSetKeyCallback   ( mWindow, (GLFWkeyfun)         dispatchKey );
                                        
    glfwSetCharCallback  ( mWindow, (GLFWcharfun)        dispatchChar );

    glfwSetInputMode     ( mWindow, GLFW_STICKY_KEYS, GL_TRUE );

    glfwSetInputMode     ( mWindow, GLFW_CURSOR,      GLFW_CURSOR_NORMAL );

    glfwPollEvents();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glDepthFunc(GL_LESS); 

}


void GlfwManager::update()
{

    glfwSwapBuffers( mWindow );
    glfwPollEvents();

}


GlfwManager::~GlfwManager()
{
    glfwTerminate();
}


bool GlfwManager::shouldExit()
{
    return    glfwGetKey( mWindow, GLFW_KEY_ESCAPE ) != GLFW_PRESS
           && glfwWindowShouldClose( mWindow )       == 0          ;
}


void GlfwManager::dispatchWindowSize( GLFWwindow* w, int x, int y )
{
    if ( mThisSingleton != nullptr ) {

        glfwGetFramebufferSize(
            mThisSingleton->mWindow,
            &(mThisSingleton->mWindowWidthInPixel),
            &(mThisSingleton->mWindowHeightInPixel)
        );
    
        glfwGetWindowSize(
            mThisSingleton->mWindow,
            &(mThisSingleton->mWindowWidth),
            &(mThisSingleton->mWindowHeight)
        );

        glViewport(
            0,
            0,
            (GLsizei)(mThisSingleton->mWindowWidthInPixel), 
            (GLsizei)(mThisSingleton->mWindowHeightInPixel)
        );

        mThisSingleton->callbackWindowSize( w, x, y );
    }
}


void GlfwManager::dispatchMouseButton( GLFWwindow* w, int b, int a, int m )
{
    if ( mThisSingleton != nullptr ) {

        mThisSingleton->callbackMouseButton( w, b, a, m );
    }
}


void GlfwManager::dispatchMousePos( GLFWwindow* w, double x, double y )
{
    if ( mThisSingleton != nullptr ) {

        mThisSingleton->callbackMousePos( w, x, y );
    }
}


void GlfwManager::dispatchMouseWheel( GLFWwindow* w, double x, double y )
{
    if ( mThisSingleton != nullptr ) {

        mThisSingleton->callbackMouseWheel( w, x, y );
    }
}


void GlfwManager::dispatchKey( GLFWwindow* w,int k,int s,int a,int m )
{
    if ( mThisSingleton != nullptr ) {

        mThisSingleton->callbackKey( w, k, s, a, m );
    }
}


void GlfwManager::dispatchChar( GLFWwindow* w, int c )
{
    if ( mThisSingleton != nullptr ) {

        mThisSingleton->callbackChar( w, c );
    }
}


void GlfwManager::callbackWindowSize(
    GLFWwindow* window, 
    int         width,
    int         height
) {
   ;
}

void GlfwManager::callbackMouseButton(
    GLFWwindow* window,
    int         button,
    int         action,
    int         mods
) {
    ;
}


void GlfwManager::callbackMousePos(
    GLFWwindow* window,
    double      xpos,
    double      ypos
) {
    ;
}


void GlfwManager::callbackMouseWheel(
    GLFWwindow* window,
    double      xoffset,
    double      yoffset
) {
    ;
}


void GlfwManager::callbackKey(
    GLFWwindow* window,
    int         key,
    int         scancode,
    int         action,
    int         mods
) {
    ;
}


void GlfwManager::callbackChar(
    GLFWwindow* window,
    int         codepoint
) {
    ;
}


class DeltaTime {

  public:

    DeltaTime(): mMicroSecondsCur(0.0)
    {
        struct timeval  mTimeVal;

        gettimeofday( &mTimeVal, NULL );

        mMicroSecondsCur = mTimeVal.tv_sec*1000000.0 + 
                           mTimeVal.tv_usec ;
    }

    ~DeltaTime(){;}

    double delta()
    {
        struct timeval  mTimeVal;
        double          microSecondsPrev;

        gettimeofday( &mTimeVal, NULL );

        microSecondsPrev = mMicroSecondsCur;

        mMicroSecondsCur  = mTimeVal.tv_sec * 1000000.0 + mTimeVal.tv_usec ;

        double delta = ( mMicroSecondsCur - microSecondsPrev ) / 1000000.0 ;
        
        return delta;
    }

  private:

    double mMicroSecondsCur;

};






string prologue = "A long time ago, in a gallaxy far\nfar away....";
string episode = "Episode IV";
string title   = "A NEW HOPE";
string para1 = "It is a period of civil war.\n"
"Rebel spaceships, striking\n"
"from a hidden base, have won\n"
"their first victory against\n"
"the evil Galactic Empire.";
string para2 = "During the battle, Rebel\n"
"spies managed to steal secret\n"
"plans to the Empire's\n"
"ultimate weapon, the DEATH\n"
"STAR, an armored space\n"
"station with enough power\n"
"to destroy an entire planet.";
string para3 = "Pursued by the Empire's\n"
"sinister agents, Princess\n"
"Leia races home aboard her\n"
"starship, custodian of the\n"
"stolen plans that can save her\n"
"people and restore\n"
"freedom to the galaxy....";


class Word {

};



class Line {

};



int main( int argc, char* argv[] )
{

    if ( argc != 1 ) {

        cerr << "Usage: test_visualizer_joints\n";
        exit(1);
    }

    GlfwManager        glfw  ( 1024, 768 );

    SDFont::RuntimeHelper helper;
    SDFont::TextureLoader loader ( "signed_dist_font.png" );
    SDFont::MetricsParser parser ( helper.glyphs(), helper.margin() );
    parser.parseSpec( "signed_dist_font.txt" );

    SDFont::VanillaShaderManager  shader ( loader.GLtexture(), 0 );

    glfw.configGLFW();


    float            width[2];
    float            bearingY[2];
    float            belowBaseLineY[2];
    float            advanceY[2];
    vector< SDFont::Glyph* > glyphs[2];
    vector< float >  textureXs[2];

    helper.getMetrics(
        "STAR",
        width[0],
        textureXs[0],
        bearingY[0],
        belowBaseLineY[0],
        advanceY[0],
        glyphs[0]         );

    helper.getMetrics(
        "WARS",
        width[1],
        textureXs[1],
        bearingY[1],
        belowBaseLineY[1],
        advanceY[1],
        glyphs[1]         );

    float*  GLPPPNNNTT;
    GLuint* GLindices;

    GLPPPNNNTT = (float*)malloc(   sizeof(float)
                              * SDFont::RuntimeHelper::NUM_FLOATS_PER_GLYPH
                              * ( glyphs[0].size() + glyphs[1].size() ) );


    GLindices  = (GLuint*) malloc(   sizeof(GLuint)
                                * SDFont::RuntimeHelper::NUM_INDICES_PER_GLYPH
                                * ( glyphs[0].size() + glyphs[1].size() ) );

    helper.generateOpenGLDrawElements (

        glyphs[0],
        textureXs[0],
        -0.5 * width[0] * 3.0,
        -0.5 * advanceY[0] * 3.0,
        3.0,
        1.0,
        0.0,
        GLPPPNNNTT,
        0,
        GLindices
    );

    helper.generateOpenGLDrawElements (
        glyphs[1],
        textureXs[1],
        -0.5 * width[1] * 3.0,
        -1.5 * advanceY[1] * 3.0,
        3.0,
        1.0,
        0.0,

        &( GLPPPNNNTT[SDFont::RuntimeHelper::NUM_FLOATS_PER_GLYPH
                       * ( glyphs[0].size() )                  ] ) ,

        SDFont::RuntimeHelper::NUM_POINTS_PER_GLYPH * glyphs[0].size() , 

        & ( GLindices[   SDFont::RuntimeHelper::NUM_INDICES_PER_GLYPH
                       * ( glyphs[0].size() )                  ] )
    );

    DeltaTime dt;
    float val = 10.0;
    bool  dir = true;
    int   e   = 0;
    do {
        auto deltaT = dt.delta();
//        cerr << "delta: [" << deltaT << "]\n";

        glfw.update();

        shader.load();

        int       effect         = e;
        float     lowThreshold   = 0.45;
        float     highThreshold  = 0.55;
        float     smoothing      = 4.0/16.0;
        glm::vec3 baseColor      (1.0, 1.0, 0.0);
        glm::vec3 borderColor    (1.0, 1.0, 0.0);

        glm::mat4 M = glm::translate(
                          glm::mat4(), 
                          glm::vec3( 0.0, 0.0, -1.0 * val )
                      );

        glm::mat4 V = glm::lookAt(
                          glm::vec3(  0.0f,  0.0f,  1.0f ), // Cam pos
                          glm::vec3(  0.0f,  0.0f,  0.0f ), // and looks here
                          glm::vec3(  0.0f,  1.0f,  0.0f )  // Head is up
                      );          
        glm::mat4 P = glm::perspective(
                          glm::radians(45.0f), 4.0f / 4.0f, 0.1f, 100.0f);

        glm::vec3 lightWCS( 0.0, 0.0, 2.0 );

        glClearColor ( 0.0, 0.0, 0.0, 0.0 );

        glClear      ( GL_COLOR_BUFFER_BIT );

        shader.draw(
            GLPPPNNNTT ,
              SDFont::RuntimeHelper::NUM_FLOATS_PER_GLYPH
            * ( glyphs[0].size() + glyphs[1].size() ) ,
            GLindices ,
              SDFont::RuntimeHelper::NUM_INDICES_PER_GLYPH
            * ( glyphs[0].size() + glyphs[1].size() ) ,
            effect,
            false,
            lowThreshold,
            highThreshold,
            smoothing,
            baseColor,
            borderColor,
            P, M, V, lightWCS
        );

        shader.unload();

        if (dir) {
            val += 0.001;
            if (val > 5.0) {
                dir = false;
                e = (e + 1) % 7 ;
            }
        }
        else {
            val -= 0.001;
            if (val < 0.0) dir = true;
        }

    } while ( glfw.shouldExit() );


    return 0;
}

