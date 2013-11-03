
#include "testApp.h"
#include "ofMain.h"
#include "ofAppGlutWindow.h"

//========================================================================
int main( ){

    ofAppGlutWindow window;
	ofSetupOpenGL(&window, 640*2,500*2, OF_FULLSCREEN);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
    
    ofHideCursor();

    
	ofRunApp( new testApp());

}
