#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup() {
    
    ////////////
    // set global of environment settings
    ////////////
    ofSetLogLevel(OF_LOG_NOTICE);
    ofSetVerticalSync(TRUE);
	ofSetFrameRate(60);
	ofBackground(255, 128, 0);    // initial background color
    ofResetElapsedTimeCounter();
    
    
    ////////////
    // setup partisynth manager
    ////////////
    handPositions.clear();
    //ofPoint handPosition;
    //handPosition.x = 320;
    //handPosition.y = 240;
    //handPositions.push_back(handPosition);
    partisynthmngr.setup();
    
    
    ////////////
    // setup soundstream
    ////////////
	// 2 output channels,
	// 0 input channels
	// 22050 samples per second
	// 512 samples per buffer
	// 4 num buffers (latency)
    int bufferSize              = 512;
	int sampleRate              = 44100;
 	soundStream.setup(this, 2, 0, sampleRate, bufferSize, 4);
    
    
    ////////////
    // setup screenID, blendMode, and other display variables
    ////////////
    // 0 > debug
    // 1 > settings, instructions
    // 2 > activity
    // 3 > workout stats, credits
    screenID = 2; // 0; // 0 = debug, 1 = settings/instructions, 2 = activity, 3 = credits
    instability = 440.0f; // used in screenShake and screenFlicker--is based off legacy frequency calculations and should eventually normalized to more understandable numbers 
    heightPct = 0.5f;
    blendMode = OF_BLENDMODE_ADD;    
    /* 	
     enum ofBlendMode {
         OF_BLENDMODE_DISABLED  = 0,
         OF_BLENDMODE_ALPHA     = 1,
         OF_BLENDMODE_ADD       = 2,
         OF_BLENDMODE_SUBTRACT  = 3,
         OF_BLENDMODE_MULTIPLY  = 4,
         OF_BLENDMODE_SCREEN    = 5
     };
     */
    depthColoring = COLORING_BLUES_INV; //TODO: test the inversion of the light and darks in this mode to see if it worked
    /* 
     enum DepthColoring {
         COLORING_PSYCHEDELIC_SHADES    = 0,
         COLORING_PSYCHEDELIC           = 1,
         COLORING_RAINBOW               = 2,
         COLORING_CYCLIC_RAINBOW        = 3, // default value in ofxOpenNI
         COLORING_BLUES                 = 4,
         COLORING_BLUES_INV             = 5,
         COLORING_GREY                  = 6,
         COLORING_STATUS                = 7,       
         COLORING_COUNT                 = 8
     };
     */

    
    ////////////
    // setup HUD elements like fonts, and labels
    ////////////    
	font.loadFont("franklinGothic.otf", 72);
    fontSMALL.loadFont("franklinGothic.otf", 18);
    verdana.loadFont(ofToDataPath("verdana.ttf"), 24);
    cals = ofToString(0);
    bpm = ofToString(ofRandom(125, 185));
    bpm = bpm.substr(0, 5);
    labelCAL = "CALORIES BURNED";
    labelBPM = "HEARTBEATS PER MINUTE";

    
    ////////////
    // set up kinects
    ////////////
    cloudRes = -1;
    stopped = false;
	angle = 0;    // zero the tilt on startup
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
        openNIDevices[deviceID].setUseDepthRawPixels(true);
        openNIDevices[deviceID].addImageGenerator();
        openNIDevices[deviceID].setRegister(true);
        openNIDevices[deviceID].setMirror(true);
        
        // set up user generator
        openNIDevices[deviceID].addUserGenerator();

        // setup the hand generator
        openNIDevices[deviceID].addHandsGenerator();
        
        // add all focus gestures (ie., wave, click, raise arm)
        openNIDevices[deviceID].addAllHandFocusGestures();
        // or you can add them one at a time
        //vector<string> gestureNames = openNIDevice.getAvailableGestures(); // you can use this to get a list of gestures
        // prints to console and/or you can use the returned vector
        //openNIDevice.addHandFocusGesture("Wave");

		openNIDevices[deviceID].start();
    }
    
    // NB: Only one device can have a user generator at a time - this is a known bug in NITE due to a singleton issue
    // so it's safe to assume that the fist device to ask (ie., deviceID == 0) will have the user generator...
    
    // TODO: Is there a reason this was being limited to 1 before I commented it out?
    openNIDevices[0].setMaxNumUsers(4); // default is 4
    openNIDevices[0].setMaxNumHands(8); // what is default?

    ofAddListener(openNIDevices[0].userEvent, this, &testApp::userEvent);
    // TODO: do I also need a handEvent listener?
    
    ofxOpenNIUser user;
    user.setUseMaskTexture(true);
    // user.setUsePointCloud(true); // TODO/NOTE: I had to disable cleanout point cloud stuff because it gives bad access errors when switching between "debug" and "activity" screen
    // user.setPointCloudDrawSize(2); // this is the size of the glPoint that will be drawn for the point cloud
    // user.setPointCloudResolution(2); // this is the step size between points for the cloud -> eg., this sets it to every second point
    openNIDevices[0].setBaseUserClass(user); // this becomes the base class on which tracked users are created
                                             // allows you to set all tracked user properties to the same type easily
                                             // and allows you to create your own user class that inherits from ofxOpenNIUser
    
    /*
    // if you want to get fine grain control over each possible tracked user for some reason you can iterate
    // through users like I'm doing below. Please not the use of nID = 1 AND nID <= openNIDevices[0].getMaxNumUsers()
    // as what you're doing here is retrieving a user that is being stored in a std::map using it's XnUserID as the key
    // that means it's not a 0 based vector, but instead starts at 1 and goes up to, and includes maxNumUsers...
    for (XnUserID nID = 1; nID <= openNIDevices[0].getMaxNumUsers(); nID++){
        ofxOpenNIUser & user = openNIDevices[0].getUser(nID);
        user.setUseMaskTexture(true);
        user.setUsePointCloud(true);
        //user.setUseAutoCalibration(false); // defualts to true; set to false to force pose detection
        //user.setLimbDetectionConfidence(0.9f); // defaults 0.3f
        user.setPointCloudDrawSize(2);
        user.setPointCloudResolution(1);
    }
    //*/
    
}

//--------------------------------------------------------------
void testApp::update(){

    // iterate through devices and hands to store positions
    handPositions.clear();
    if (numDevices) {
        for (int deviceID = 0; deviceID < numDevices; deviceID++){
            openNIDevices[deviceID].update();

            int numHands = openNIDevices[deviceID].getNumTrackedHands(); // get number of current hands
            // TODO: store a list of hand locations to both print in debug and hand to partisynthMngr during .update()
            if (numHands) {
                for (int i = 0; i < numHands; i++){
                    
                    ofxOpenNIHand & hand = openNIDevices[deviceID].getTrackedHand(i); // get a reference to this hand
                    ofPoint & handPosition = hand.getPosition(); // get hand position
                    handPositions.push_back(handPosition); // store the positions
                }
            }
            else {
                /*
                ofPoint handPosition;
                handPosition.x = 320; // x positions range from 0 (left) - 640 (right)
                handPosition.y = 240; // y positions range from 0 (top) - 480 (bottom)
                handPosition.z = 1850;// z positions range from about 600 (near) - 3000 (far)
                handPositions.push_back(handPosition);
                //*/
            }
        }
    }
 
    partisynthmngr.update(handPositions);
    updateProperties();

}

void testApp::updateProperties(){
    
    // TODO: Agregate Y + Z information from hands array to determine level of instability
    float instabilityPct = 0.0f;
    if (handPositions.size()) {
        for (int i = 0; i < handPositions.size(); i++) {
            // x positions range from 0 (left) - 640 (right)
            float pctY =    1   - handPositions[i].y / 480.0f; // y positions range from 0 (top) - 480 (bottom)
            float pctZ =    1; // 1   - handPositions[i].z / 2500.0f; // z positions range from about 500 (near) - 3000 (far), but this should be done relative to player's torso since it requires a great deal of movement from the player to get this tuned appealingly
            instabilityPct += pctY * pctZ / handPositions.size();
            cout << "handPositions["<<i<<"] = ("<<handPositions[i].x<<", "<<handPositions[i].y<<", "<<handPositions[i].z<<")"<<endl;
        }
    }

    // exponential relationship between instabilityPct and instability
 	instability = 100.0f * pow(1.059463094359f, instabilityPct*75.0f);
}



void testApp::updateProperties(int x, int y){
    ///*
    //int width = ofGetWidth();
	//float height = ofGetHeight();
    int width = 640; // use dimensions of Kinect output so they map well with it when drawn at this scale
	float height = 480;
    
    // set default values so we don't get ridiculous pitches at start of program
    if (!x && !y) {
        x = width / 2.0f;
        y = height / 2.0f;
    }
    
    // pan = (float)x / (float)width;
	heightPct = ((height-y) / height);
    
    // exponential relationship between instability and mouse Y -- could do this when in a "mouseFollow" mode
 	instability = 100.0f * pow(1.059463094359f, heightPct*75.0f);

}


//--------------------------------------------------------------
void testApp::draw(){
    
    // create reportStream for messages
    stringstream reportStream;

    // define booleans for drawing elements
    bool    displayActivity = false;
    bool    displayHUD      = false;
    bool    displayDebug    = false;
    bool    displayReport   = false;
    bool    screenShake     = false;
    bool    screenFlicker   = false;
    
    // get width and height of window for positioning of calories burned labels
    float windowW = ofGetWidth();
    float windowH = ofGetHeight();
    float windowMargin = 15; // how far from the top of the screen labels will draw
    
    // test to see if there are any recognized users so we can display vitals or not depending on this
    int numUsers = openNIDevices[0].getNumTrackedUsers();
          
    ////////////
    // draw screen based on display mode
    ////////////
    switch (screenID) {
            
        case 0: 
            // 0 > debug
            // 
            ofBackground(0, 255, 255);
            ofSetColor(255, 255, 255);

            displayDebug = true;
            //displayHUD = true;
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
            ofBackground(255, 128, 0);
            ofSetColor(255, 255, 255);

            displayActivity = true;
            screenShake     = true;
            screenFlicker   = true;
            //displayHUD      = true;
            displayReport   = true;

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
    
    if (displayReport) { // header for report stream -- ofDrawBitmap String occurs at end of draw so other modes can input their report data
        reportStream 
        << "ScreenID = " << screenID << " (press 'd' for debug[0], 'a' for activity[2]" <<endl
        << "Stopped = " << stopped <<" (press 'x' to stop all devices, 's' to start)" << endl
        << "Cloud Resolution = " << cloudRes <<" (press '1'–'5' to modify)" << endl
        << "Image On = " << openNIDevices[0].isImageOn() << "; Infrared On = " << openNIDevices[0].isInfraOn() << " (press 'i' to toggle between image and infra)" << endl
        << "Backbuffer = " << openNIDevices[0].getUseBackBuffer() <<" (press 'b' to toggle)" << endl
        << "Millis: " << ofGetElapsedTimeMillis() << endl
        << "FPS: " << ofGetFrameRate() << endl
        ;
        // ofDrawBitmapString(reportStream.str(), windowMargin, windowH - 150); // moved this to end of draw() so other elements could add to report stream
    }
    
    if (displayDebug) {
        
        ofPushMatrix();
        
        ofEnableBlendMode(blendMode);
        for (int deviceID = 0; deviceID < numDevices; deviceID++){
            ofTranslate(0, deviceID * 480);
            openNIDevices[deviceID].drawDebug(); // debug draw does the equicalent of the commented methods below
            //        openNIDevices[deviceID].drawDepth(0, 0, 640, 480);
            //        openNIDevices[deviceID].drawImage(640, 0, 640, 480);
            //        openNIDevices[deviceID].drawSkeletons(640, 0, 640, 480);
            
            reportStream
            << "Device[" << deviceID <<"] FPS: " + ofToString(openNIDevices[deviceID].getFrameRate()) << endl;
        }
        
        // iterate through handsPositions and draw small rects at their locations
        int numHands = handPositions.size();
        // print hand locations
        for (int i = 0; i < numHands; i++){
            // draw a rect at the position (don't confuse this with the debug draw which shows circles!!)
            ofSetColor(255,0,0);
            ofRect(handPositions[i].x, handPositions[i].y, 10, 10);
        }

        // draw particle emitters
        partisynthmngr.draw();
        
        // do some drawing of user clouds and masks
        ofPushMatrix();
        int numUsers = openNIDevices[0].getNumTrackedUsers();
        for (int nID = 0; nID < numUsers; nID++){
            ofxOpenNIUser & user = openNIDevices[0].getTrackedUser(nID);
            // TODO: Would be nice to apply user masks in a way that plays better with multiple users
            // user.drawMask();
            ofPushMatrix();
            ofRotate(180, 0, 1, 0);
            // ofTranslate(320, 240, -1000);
            ofTranslate(320, 240, 0);
            //user.drawPointCloud();
            ofPopMatrix();
        }
        ofDisableBlendMode();
        ofPopMatrix();

        ofPopMatrix();
    }
    
    ///*
    if (screenShake) {
        int mult = 5;
        ofTranslate(ofRandomf() * pow(instability,mult)    / pow(2000.0f, mult  ), 
                    ofRandomf() * pow(instability,mult)    / pow(2000.0f, mult  ), 
                    ofRandomf() * pow(instability,mult/2)  / pow(2000.0f, mult/2) );        
    }
    //*/
    
    ofPushStyle();
    //ofEnableBlendMode(OF_BLENDMODE_ADD);
    /* 	
     OF_BLENDMODE_DISABLED  = 0
     OF_BLENDMODE_ALPHA     = 1
     OF_BLENDMODE_ADD       = 2
     OF_BLENDMODE_SUBTRACT  = 3
     OF_BLENDMODE_MULTIPLY  = 4
     OF_BLENDMODE_SCREEN    = 5
     */
    ///*
    if (screenFlicker) {
        float mult = 5;
        ofBackground(16     + ofRandom( pow(instability, mult)      / pow(2048.0f, mult ) ), 
                     64     + ofRandom( pow(instability, mult)      / pow(2048.0f, mult ) ),
                     128    + ofRandom( pow(instability, mult)      / pow(2048.0f, mult ) ) );
        //        ofSetColor(     16     + ofRandom( pow(instability, mult)      / pow(2048.0f, mult ) ), 
        //                   64     + ofRandom( pow(instability, mult)      / pow(2048.0f, mult ) ),
        //                   128    + ofRandom( pow(instability, mult)      / pow(2048.0f, mult ) ) );
        //        ofRectangle(0, 0, 640, 480);
    }
    else {
        // ofBackground(16, 64, 128);
        ofSetColor(16, 64, 128);
        ofRectangle(0, 0, 640, 480);
    } //*/
    // ofDisableBlendMode();
    ofPopStyle();
        
    if (displayActivity) {
        
        // displayActivity matrix = (-30, -30, 1340, 860);
        
        ofPushMatrix();
        // ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofEnableBlendMode(blendMode);
        /* 	
         OF_BLENDMODE_DISABLED  = 0
         OF_BLENDMODE_ALPHA     = 1
         OF_BLENDMODE_ADD       = 2
         OF_BLENDMODE_SUBTRACT  = 3
         OF_BLENDMODE_MULTIPLY  = 4
         OF_BLENDMODE_SCREEN    = 5
         */
        
        for (int deviceID = 0; deviceID < numDevices; deviceID++){
            // ofTranslate(0, deviceID * 480);
            // openNIDevices[deviceID].drawDebug(); // debug draw does the equivalent of the commented methods below
            //        openNIDevices[deviceID].drawDepth(0, 0, 640, 480);
            //        openNIDevices[deviceID].drawImage(640, 0, 640, 480);
            //        openNIDevices[deviceID].drawSkeletons(640, 0, 640, 480);
            
            float pctW  = windowW  * 1.15f     / 640.0f;
            float pctH  = windowH  * 1.15f     / 480.0f;
            float pct   = MAX(pctW, pctH);
            float offsetX   =   50.0f;
            float offsetY   =   30.0f;//45.0f;
            
            float x = (windowW - (640.0f * pct)) / 2.0f;
            float y = (windowH - (480.0f * pct)) / 2.0f;

            ofScale(pct, pct, 0.0f);
            ofTranslate(x + offsetX, y + offsetY, 0.0f);

            openNIDevices[deviceID].setDepthColoring(depthColoring); // COLORING_CYCLIC_RAINBOW by default
            /*
            enum DepthColoring {
                COLORING_PSYCHEDELIC_SHADES = 0,
                COLORING_PSYCHEDELIC,
                COLORING_RAINBOW,
                COLORING_CYCLIC_RAINBOW,
                COLORING_BLUES,
                COLORING_GREY,
                COLORING_STATUS,
                COLORING_COUNT
            };
            //*/
            
            openNIDevices[deviceID].drawDepth(0, 0, 640, 480);
            // openNIDevices[deviceID].drawSkeletons(0, 0, 640, 480);

            /* // TODO: Would be nice to apply user masks in a more creative way
            // do some drawing of user clouds and masks
            ofPushMatrix();
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            int numUsers = openNIDevices[0].getNumTrackedUsers();
            for (int nID = 0; nID < numUsers; nID++){
                ofxOpenNIUser & user = openNIDevices[0].getTrackedUser(nID);
                user.drawMask();
                ofPushMatrix();
                ofRotate(180, 0, 1, 0);
                // ofTranslate(320, 240, -1000);
                ofTranslate(320, 240, 0);
                user.drawPointCloud();
                ofPopMatrix();
            }
            ofDisableBlendMode();
            ofPopMatrix();
            // */
                        
            reportStream
            << "Device[" << deviceID <<"] FPS: " + ofToString(openNIDevices[deviceID].getFrameRate()) << endl;

        }
        // iterate through handsPositions and draw small rects at their locations
        int numHands = handPositions.size();
        // print hand locations
        for (int i = 0; i < numHands; i++){
            // draw a rect at the position (don't confuse this with the debug draw which shows circles!!)
            ofSetColor(255,0,0);
            ofRect(handPositions[i].x, handPositions[i].y, 10, 10);
        } 
        
        // draw particle emitters
        partisynthmngr.draw();
       
        ofDisableBlendMode();
        ofPopMatrix();
    }
        
    if (displayHUD) {
        // Update calories burned and bpm
        if (ofGetElapsedTimeMillis() > 1000 && numUsers) {
            //cals = ofToString(ofRandom(1.284,  2.471));// (min, max)
            //cals = cals.substr(0, 5);
            int calIncrement = ofRandom(1.284,  2.471);// (min, max)
            cals = ofToString(ofToInt(cals) + calIncrement);
            bpm = ofToString(ofRandom(125, 185));
            bpm = bpm.substr(0, 3);
            ofResetElapsedTimeCounter();
        } else {
            if (!numUsers) {
                cals = "0";
                bpm = "0";
            }
        }
        
        // set label background dimensions
        int labelBGw = 350;
        int labelBGh = 145;
        int labelMargin = 5; // how far from eachother labels will draw

        ofEnableAlphaBlending();
        
        // draw HUD relative to right of screen
        ofPushMatrix();
        ofTranslate(windowW-labelBGw, (-labelBGh/2));
        
        int iMax;
        if (numUsers) {
            //iMax = numUsers; //TODO: fix this as it seems to be buggy at the moment
            iMax = 1;
        } else {
            iMax = 1;
        }
        
        for (int i = 0; i < iMax; i++) {
            // draw next below previous lable
            ofPushMatrix();
            ofTranslate(0, (labelBGh + labelMargin)); 

            // first lable
            ofSetColor(255, 128, 0, 255); // bg
            ofRect(0, 0, labelBGw, labelBGh);
            ofSetColor(255, 255, 255); // text
            fontSMALL.drawString(labelCAL, 15, 30);
            font.drawString(cals, 15, 117);
            //font.drawString(ofToString(ofToInt(cals) + ofRandom(-5, 5)), 15, 117);
            
            // draw next below previous lable
            ofPushMatrix();
            ofTranslate(0, (labelBGh + labelMargin)); 
            
            // second label
            ofSetColor(255, 0, 255, 255); // bg
            ofRect(0, 0, labelBGw, labelBGh);
            ofSetColor(255, 255, 255); // text
            fontSMALL.drawString(labelBPM,  15, 30);
            font.drawString(bpm, 15, 117);
            //font.drawString(ofToString(ofToInt(bpm) + ofRandom(-5, 5)), 15, 117);
        }
        
        // reset matrixes for labels
        for (int i = 0; i < iMax; i++) {
            ofPopMatrix();
            ofPopMatrix();
        }
        
        // reset matrix for HUD
        ofPopMatrix();

        ofDisableAlphaBlending();
    }
    
    if (displayReport) { // this ofDrawBitmap String occurs at end of draw so other modes can input their report data after generic header
        ofDrawBitmapString(reportStream.str(), windowMargin, windowH - 150);
    }

}

//--------------------------------------------------------------
void testApp::userEvent(ofxOpenNIUserEvent & event){
    ofLogNotice() << getUserStatusAsString(event.userStatus) << "for user" << event.id << "from device" << event.deviceID;
}


//--------------------------------------------------------------
void testApp::handEvent(ofxOpenNIHandEvent & event){
    // show hand event messages in the console
    ofLogNotice() << getHandStatusAsString(event.handStatus) << "for hand" << event.id << "from device" << event.deviceID;
}

//--------------------------------------------------------------
void testApp::exit(){
    // this often does not work -> it's a known bug -> but calling it on a key press or anywhere that isnt std::exit() works
    // press 'x' to shutdown cleanly...
    for (int deviceID = 0; deviceID < numDevices; deviceID++){
        openNIDevices[deviceID].stop();
    }
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    cloudRes = -1;
    switch (key) {
        
        
        ////////////
        // handle cloudRes (TODO: clean since I don't use it anymore?)
        // and BlendMode to be used used in .draw()
        ////////////
        /* 	
             enum ofBlendMode {
             OF_BLENDMODE_DISABLED  = 0,
             OF_BLENDMODE_ALPHA     = 1,
             OF_BLENDMODE_ADD       = 2,
             OF_BLENDMODE_SUBTRACT  = 3,
             OF_BLENDMODE_MULTIPLY  = 4,
             OF_BLENDMODE_SCREEN    = 5
         };
         */
        case '0':
            cloudRes = 1;
            blendMode = OF_BLENDMODE_DISABLED;
            break;
        case '1':
            cloudRes = 1;
            blendMode = OF_BLENDMODE_ALPHA;
            break;
        case '2':
            blendMode = OF_BLENDMODE_ADD;
            cloudRes = 2;
            break;
        case '3':
            blendMode = OF_BLENDMODE_SUBTRACT;
            cloudRes = 3;
            break;
        case '4':
            blendMode = OF_BLENDMODE_MULTIPLY;
            cloudRes = 4;
            break;
        case '5':
            blendMode = OF_BLENDMODE_SCREEN;
            cloudRes = 5;
            break;

        
        ////////////
        // handle color scheme for depth buffer to be used used in .draw()
        ////////////
        /* 
         enum DepthColoring {
             COLORING_PSYCHEDELIC_SHADES    = 0,
             COLORING_PSYCHEDELIC           = 1,
             COLORING_RAINBOW               = 2,
             COLORING_CYCLIC_RAINBOW        = 3, // default value in ofxOpenNI
             COLORING_BLUES                 = 4,
             COLORING_BLUES_INV             = 5,
             COLORING_GREY                  = 6,
             COLORING_STATUS                = 7,       
             COLORING_COUNT                 = 8
         };
         */
        case ')': // SHIFT + 0
            depthColoring = COLORING_PSYCHEDELIC_SHADES; 
            break;
        case '!': // SHIFT + 1
            depthColoring = COLORING_PSYCHEDELIC; 
            break;
        case '@': // SHIFT + 2
            depthColoring = COLORING_RAINBOW; // default value for depthColoring enum in ofxOpenNI
            break;
        case '#': // SHIFT + 3
            depthColoring = COLORING_CYCLIC_RAINBOW;
           break;
        case '$': // SHIFT + 4
            depthColoring = COLORING_BLUES;
            break;
        case '%': // SHIFT + 5
            depthColoring = COLORING_BLUES_INV;
            break;
        case '^': // SHIFT + 6
            depthColoring = COLORING_GREY;
            break;
        case '&': // SHIFT + 7
            depthColoring = COLORING_STATUS;
            break;
            
            
        ////////////
        // handle some hardware settings
        ////////////
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
            
        case 'r': 
            screenID = 1; // display settings, instructions screen
            break;
            
        case 'e': 
        case 'a': 
            screenID = 2; // display activity screen
            break;
            
        case 'f': 
            screenID = 3; // display workout stats, credits screen
            break;
        
        default:
            break;
    }
    for (int deviceID = 0; deviceID < numDevices; deviceID++){
		//openNIDevices[deviceID].setPointCloudResolutionAllUsers(cloudRes);
	}
    
    // Hand off key presses to other objects
    partisynthmngr.keyPressed(key);
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

    // Hand off mouseMoved events to other objects
    partisynthmngr.mouseMoved(x, y);
    
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

    mouseMoved(x, y);
    
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

    partisynthmngr.mousePressed(x, y, button);
    
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

    partisynthmngr.mouseReleased(x, y, button);
    
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::audioOut(float * output, int bufferSize, int nChannels){
    
    partisynthmngr.audioOut(output, bufferSize, nChannels); 

}