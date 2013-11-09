#ifndef _TEST_APP
#define _TEST_APP

#include "ofxOpenNI.h"
#include "ofMain.h"
#include "partisynthmngr.h"

#define MAX_DEVICES 2

class testApp : public ofBaseApp{

public:
    
	void setup();
	void update();
	void draw();
    void exit();
    
	void keyPressed  (int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);

    void audioOut(float * input, int bufferSize, int nChannels);
    void updateProperties(int x, int y);
    void updateProperties();

    void userEvent(ofxOpenNIUserEvent & event);
    void handEvent(ofxOpenNIHandEvent & event);

    ofTrueTypeFont      verdana;
	ofxOpenNI           openNIDevices[MAX_DEVICES];
    int                 numDevices;
    enum ofBlendMode    blendMode;
    enum DepthColoring  depthColoring;
    vector<ofPoint>     handPositions;
    float               instability;
    float               heightPct;
    
private:
    ////////////
    // screenID value convention
    ////////////
    // 0 > debug
    // 1 > settings, instructions
    // 2 > activity
    // 3 > workout stats, credits
    int cloudRes;
    int stopped;
	int angle;
    int screenID;
    
	ofTrueTypeFont		font;
    ofTrueTypeFont      fontSMALL;
    
    string cals;
    string labelCAL;
    string bpm;
    string labelBPM;
    // string messageSMALL; // TODO: cleanup? Not sure this is used anywhere
    
    ofSoundStream       soundStream;
    PartisynthMngr      partisynthmngr;
    
};

#endif
