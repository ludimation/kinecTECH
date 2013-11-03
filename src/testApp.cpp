#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup() {
    
    ofSetLogLevel(OF_LOG_NOTICE);
    
	/*
     ofxOpenNI       openNIDevices[MAX_DEVICES];
     int             numDevices;
     ofTrueTypeFont  verdana;
     
     int screenID;
     
     ofTrueTypeFont		font;
     ofTrueTypeFont      fontSMALL;
     
     string cals;
     string labelCAL;
     string bpm;
     string labelBPM;
     string messageSMALL;
     */
    
    // set initial background color
	ofBackground(255, 128, 0);
    ofResetElapsedTimeCounter();
    
    ////////////
    // set screen to display
    ////////////
    // 0 > debug
    // 1 > settings, instructions
    // 2 > activity
    // 3 > workout stats, credits
    screenID = 2; // 0 = debug, 1 = settings/instructions, 2 = activity, 3 = credits

    cals = ofToString(ofRandom(1.284,  2.471));// (min, max)
    cals = cals.substr(0, 5);
    bpm = ofToString(ofRandom(125, 185));
    bpm = bpm.substr(0, 5);
    labelCAL = "CALORIES PER MINUTE";
    labelBPM = "HEARTBEATS PER MINUTE";
    
	font.loadFont("franklinGothic.otf", 72);
    fontSMALL.loadFont("franklinGothic.otf", 18);
    verdana.loadFont(ofToDataPath("verdana.ttf"), 24);
    
    cloudRes = -1;
    stopped = false;
    
    // zero the tilt on startup
	angle = 0;
    
    // setup Kinects and users
    numDevices = openNIDevices[0].getNumDevices();
    
    for (int deviceID = 0; deviceID < numDevices; deviceID++){
        //openNIDevices[deviceID].setLogLevel(OF_LOG_VERBOSE); // ofxOpenNI defaults to ofLogLevel, but you can force to any level
        openNIDevices[deviceID].setup();
        // TODO: reimplement angle control
        // openNIDevices[deviceID].setCameraTiltAngle(angle); // is there an openNI equivalent for this? or should I use ofxKinect? Can it be used in combination with ofxOpenNI?
        // candidates: setPosition(); 
        // resetUserTracking();
        // openNIDevices[deviceID].setUserSmoothing(float smooth); // what values is this expecting?
        // setSkeletonProfile(XnSkeletonProfile profile); // what does this do?
        // addGesture(); // could be the gesture control I'm looking for
        // there are a slew of hand tracking functions too
        openNIDevices[deviceID].addDepthGenerator();
        openNIDevices[deviceID].addImageGenerator();
        openNIDevices[deviceID].addUserGenerator();
        openNIDevices[deviceID].setRegister(true);
        openNIDevices[deviceID].setMirror(true);
		openNIDevices[deviceID].start();
    }
    
    // NB: Only one device can have a user generator at a time - this is a known bug in NITE due to a singleton issue
    // so it's safe to assume that the fist device to ask (ie., deviceID == 0) will have the user generator...
    
    // TODO: Is there a reason this was being limited to 1 before I commented it out?
    // openNIDevices[0].setMaxNumUsers(1); // defualt is 4
    ofAddListener(openNIDevices[0].userEvent, this, &testApp::userEvent);
    
    ofxOpenNIUser user;
    user.setUseMaskTexture(true);
    user.setUsePointCloud(true);
    user.setPointCloudDrawSize(2); // this is the size of the glPoint that will be drawn for the point cloud
    user.setPointCloudResolution(2); // this is the step size between points for the cloud -> eg., this sets it to every second point
    openNIDevices[0].setBaseUserClass(user); // this becomes the base class on which tracked users are created
                                             // allows you to set all tracked user properties to the same type easily
                                             // and allows you to create your own user class that inherits from ofxOpenNIUser
    
    // if you want to get fine grain control over each possible tracked user for some reason you can iterate
    // through users like I'm doing below. Please not the use of nID = 1 AND nID <= openNIDevices[0].getMaxNumUsers()
    // as what you're doing here is retrieving a user that is being stored in a std::map using it's XnUserID as the key
    // that means it's not a 0 based vector, but instead starts at 1 and goes up to, and includes maxNumUsers...
//    for (XnUserID nID = 1; nID <= openNIDevices[0].getMaxNumUsers(); nID++){
//        ofxOpenNIUser & user = openNIDevices[0].getUser(nID);
//        user.setUseMaskTexture(true);
//        user.setUsePointCloud(true);
//        //user.setUseAutoCalibration(false); // defualts to true; set to false to force pose detection
//        //user.setLimbDetectionConfidence(0.9f); // defaults 0.3f
//        user.setPointCloudDrawSize(2);
//        user.setPointCloudResolution(1);
//    }
}

//--------------------------------------------------------------
void testApp::update(){
    ofBackground(0, 0, 0);
    for (int deviceID = 0; deviceID < numDevices; deviceID++){
        openNIDevices[deviceID].update();
    }
}

//--------------------------------------------------------------
void testApp::draw(){
    
    // create reportStream for messages
    stringstream reportStream;

    // define booleans for drawing elements
    bool    displayActivity = false;
    bool    displayHUD = false;
    bool    displayDebug = false;
    bool    displayReport = false;
    
    // get width and height of window for positioning of calories burned labels
    int windowW = ofGetWidth();
    int windowH = ofGetHeight();
    int windowMargin = 15; // how far from the top of the screen labels will draw
        
    ////////////
    // draw screen based on display mode
    ////////////
    switch (screenID) {
            
        case 0: 
            // 0 > debug
            //
            ofBackground(255, 128, 0);
            ofSetColor(255, 255, 255);
            
            displayDebug = true;
            displayHUD = true;
            displayReport = true;

            break;
            
        case 1:
            // 1 > settings, instructions
            // 
            ofBackground(255, 0, 128);
            ofSetColor(255, 255, 255);
            
            displayReport = true;

            break;
            
        case 2: 
            // 2 > activity
            // 
            ofBackground(0, 255, 255);
            ofSetColor(255, 255, 255);

            displayActivity = true;
            displayHUD = true;

            break;
            
        case 3: 
            // 3 > workout stats, credits
            // 
            ofBackground(128, 0, 255);
            ofSetColor(255, 255, 255);
            
            displayReport = true;
            
            break;
            
        default:
            break;
    }
    
    if (displayReport) {
        reportStream 
        << "ScreenID = " << screenID << " (press 'd' for debug[0], 'a' for activity[2]" <<endl
        << "Stopped = " << stopped <<" (press 'x' to stop all devices, 's' to start)" << endl
        << "Cloud Resolution = " << cloudRes <<" (press '1'–'5' to modify)" << endl
        << "Image On = " << openNIDevices[0].isImageOn() << "; Infrared On = " << openNIDevices[0].isInfraOn() << " (press 'i' to toggle between image and infra)" << endl
        << "Backbuffer = " << openNIDevices[0].getUseBackBuffer() <<" (press 'b' to toggle)" << endl
        << "Millis: " << ofGetElapsedTimeMillis() << "; FPS: " << ofGetFrameRate() << endl
        ;
        ofDrawBitmapString(reportStream.str(), windowMargin, windowH - 150);
    }
    
    if (displayDebug) {
        
        ofPushMatrix();
        
        for (int deviceID = 0; deviceID < numDevices; deviceID++){
            ofTranslate(0, deviceID * 480);
            openNIDevices[deviceID].drawDebug(); // debug draw does the equicalent of the commented methods below
            //        openNIDevices[deviceID].drawDepth(0, 0, 640, 480);
            //        openNIDevices[deviceID].drawImage(640, 0, 640, 480);
            //        openNIDevices[deviceID].drawSkeletons(640, 0, 640, 480);
            
        }
        
        // do some drawing of user clouds and masks
        ofPushMatrix();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        int numUsers = openNIDevices[0].getNumTrackedUsers();
        for (int nID = 0; nID < numUsers; nID++){
            ofxOpenNIUser & user = openNIDevices[0].getTrackedUser(nID);
            user.drawMask();
            ofPushMatrix();
            ofTranslate(320, 240, -1000);
            user.drawPointCloud();
            ofPopMatrix();
        }
        ofDisableBlendMode();
        ofPopMatrix();
    }
    
    if (displayActivity) {
        
        // displayActivity matrix = (-30, -30, 1340, 860);
        
        ofPushMatrix();
        
        for (int deviceID = 0; deviceID < numDevices; deviceID++){
            // ofTranslate(0, deviceID * 480);
            // openNIDevices[deviceID].drawDebug(); // debug draw does the equivalent of the commented methods below
            //        openNIDevices[deviceID].drawDepth(0, 0, 640, 480);
            //        openNIDevices[deviceID].drawImage(640, 0, 640, 480);
            //        openNIDevices[deviceID].drawSkeletons(640, 0, 640, 480);
            
            int pctW = windowW / 640 + 0.05f;
            int pctH = windowH / 480 + 0.05f;
            int pct = MAX(pctW, pctH);
            int x = (windowW / 2) - (640 * pct / 2);
            int y = (windowH / 2) - (480 * pct / 2);
            
            ofScale(pct, pct);
            ofTranslate(x, y);
            openNIDevices[deviceID].drawDepth(0, 0, 640, 480);
            openNIDevices[deviceID].drawSkeletons(0, 0, 640, 480);

        }
        
        ofPopMatrix();
    }
        
    
    if (displayHUD) {
        // Update calories burned and bpm
        if (ofGetElapsedTimeMillis() > 1000 ) {
            cals = ofToString(ofRandom(1.284,  2.471));// (min, max)
            cals = cals.substr(0, 5);
            bpm = ofToString(ofRandom(125, 185));
            bpm = bpm.substr(0, 3);
            
            ofResetElapsedTimeCounter();
        }
        
        ofEnableAlphaBlending();
        
        // set label background dimensions
        int labelBGw = 350;
        int labelBGh = 145;
        int labelMargin = 5; // how far from eachother labels will draw
        
        // draw relative to right of screen
        ofPushMatrix();
        ofTranslate(windowW-labelBGw, windowMargin);
        
        // first lable
        ofSetColor(255, 128, 0, 255); // bg
        ofRect(0, 0, labelBGw, labelBGh);
        ofSetColor(255, 255, 255); // text
        fontSMALL.drawString(labelCAL, 15, 30);
        font.drawString(cals, 15, 117);
        
        // draw next below previous lable
        ofPushMatrix();
        ofTranslate(0, (labelBGh + labelMargin)); 
        
        // second label
        ofSetColor(255, 0, 255, 255); // bg
        ofRect(0, 0, labelBGw, labelBGh);
        ofSetColor(255, 255, 255); // text
        fontSMALL.drawString(labelBPM,  15, 30);
        font.drawString(bpm, 15, 117);
        
        // reset matrix
        ofPopMatrix();
        ofPopMatrix();
        
        ofDisableAlphaBlending();
        
    }
}

//--------------------------------------------------------------
void testApp::userEvent(ofxOpenNIUserEvent & event){
    ofLogNotice() << getUserStatusAsString(event.userStatus) << "for user" << event.id << "from device" << event.deviceID;
}

//--------------------------------------------------------------
void testApp::exit(){
    // this often does not work -> it's a known bug -> but calling it on a key press or anywhere that isnt std::aexit() works
    // press 'x' to shutdown cleanly...
    for (int deviceID = 0; deviceID < numDevices; deviceID++){
        openNIDevices[deviceID].stop();
    }
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    cloudRes = -1;
    switch (key) {
        case '1':
            cloudRes = 1;
            break;
        case '2':
            cloudRes = 2;
            break;
        case '3':
            cloudRes = 3;
            break;
        case '4':
            cloudRes = 4;
            break;
        case '5':
            cloudRes = 5;
            break;
        case 'x': // stop all devices
            // TODO: trouble-shoot this
            for (int deviceID = 0; deviceID < numDevices; deviceID++){
                openNIDevices[deviceID].stop();
            }
            stopped = true;
            break;
        case 's': // start all devces
            // TODO: trouble-shoot this, does not seem to be restarting the devices as expected
            // might need to separate out setup calls into a function that can be called anytime
            for (int deviceID = 0; deviceID < numDevices; deviceID++){
                openNIDevices[deviceID].start();
            }
            stopped = false;
            break;
        case 'i': // toggle between infrared and image generators
            for (int deviceID = 0; deviceID < numDevices; deviceID++){
                if (openNIDevices[deviceID].isImageOn()){
                    openNIDevices[deviceID].removeImageGenerator();
                    openNIDevices[deviceID].addInfraGenerator();
                    continue;
                }
                if (openNIDevices[deviceID].isInfraOn()){
                    openNIDevices[deviceID].removeInfraGenerator();
                    openNIDevices[deviceID].addImageGenerator();
                    continue;
                }
            }
            break;
        case 'b':  // toggle using backBuffer
            for (int deviceID = 0; deviceID < numDevices; deviceID++){
                openNIDevices[deviceID].setUseBackBuffer(!openNIDevices[deviceID].getUseBackBuffer());
            }
            break;

        ////////////
        // set screen to display
        ////////////
        // 0 > debug
        // 1 > settings, instructions
        // 2 > activity
        // 3 > workout stats, credits
        
        case 'd': 
            screenID = 0; // display debug screen
            break;
            
        case 'e': 
            screenID = 1; // display settings, instructions screen
            break;
            
        case 'a': 
            screenID = 2; // display activity screen
            break;
            
        case 'w': 
            screenID = 3; // display workout stats, credits screen
            break;
        
        default:
            break;
    }
    for (int deviceID = 0; deviceID < numDevices; deviceID++){
		openNIDevices[deviceID].setPointCloudResolutionAllUsers(cloudRes);
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

