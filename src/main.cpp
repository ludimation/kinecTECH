#include "testApp.h"
#include "ofAppGlutWindow.h"

int main() {
	// ofAppGlutWindow window;          // <-------- setup the GL context
	// ofSetupOpenGL(&window, 1024, 768, OF_WINDOW);
    
    ofSetupOpenGL(1024,768, OF_FULLSCREEN);			// <-------- setup the GL context
	ofHideCursor();

	ofRunApp(new testApp());
}