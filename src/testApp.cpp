#include "testApp.h"


//--------------------------------------------------------------
void testApp::setup() {
    ofResetElapsedTimeCounter();
    cals = ofToString(ofRandom(1.284,  2.471));// (min, max)
    bpm = cals.substr(0, 5);
    bpm = ofToString(ofRandom(125, 185));
    
	font.loadFont("franklinGothic.otf", 72);
    fontSMALL.loadFont("franklinGothic.otf", 18);
	
    ofSetLogLevel(OF_LOG_VERBOSE);
	
	// enable depth->video image calibration
	kinect.setRegistration(true);
    
	kinect.init();
	//kinect.init(true); // shows infrared instead of RGB video image
	//kinect.init(false, false); // disable video image (faster fps)
	
	kinect.open();		// opens first available kinect
	//kinect.open(1);	// open a kinect by id, starting with 0 (sorted by serial # lexicographically))
	//kinect.open("A00362A08602047A");	// open a kinect using it's unique serial #
	
	// print the intrinsic IR sensor values
	if(kinect.isConnected()) {
		ofLogNotice() << "sensor-emitter dist: " << kinect.getSensorEmitterDistance() << "cm";
		ofLogNotice() << "sensor-camera dist:  " << kinect.getSensorCameraDistance() << "cm";
		ofLogNotice() << "zero plane pixel size: " << kinect.getZeroPlanePixelSize() << "mm";
		ofLogNotice() << "zero plane dist: " << kinect.getZeroPlaneDistance() << "mm";
	}
	
#ifdef USE_TWO_KINECTS
	kinect2.init();
	kinect2.open();
#endif
	
    colorImgMultiply.allocate(kinect.width, kinect.height);
	colorImg.allocate(kinect.width, kinect.height);
	grayImage.allocate(kinect.width, kinect.height);
	grayThreshNear.allocate(kinect.width, kinect.height);
	grayThreshFar.allocate(kinect.width, kinect.height);
	
	nearThreshold = 230;
	farThreshold = 70;
	bThreshWithOpenCV = true;
	
	ofSetFrameRate(60);
	
	// zero the tilt on startup
	angle = 0;
	kinect.setCameraTiltAngle(angle);
	
	// start from the front
	bDrawPointCloud = false;
    
    // set initial background color
	ofBackground(255, 128, 0);

    ////////////
    // set screen to display
    ////////////
    // 0 > debug
    // 1 > settings, instructions
    // 2 > activity
    // 3 > workout stats, credits
    screenID = 0;
    
}

//--------------------------------------------------------------
void testApp::update() {
	
	kinect.update();
	
	// there is a new frame and we are connected
	if(kinect.isFrameNew()) {
		
		// load grayscale depth image from the kinect source
		grayImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);

        int numPixels = grayImage.getWidth() * grayImage.getHeight();
        
        ofxCvGrayscaleImage imageR = grayImage;
        // work with red channel pixels to make them brighter
        unsigned char * pixRED = imageR.getPixels();
        for(int i = 0; i < numPixels; i++) {
            int val = pixRED[i] * 1.5;
            if (val > 255) {
                val = 255;
            }
            pixRED[i] = val;
        }
        ofxCvGrayscaleImage imageB = imageR;
        // work with blue channel pixels to make them all 255
        unsigned char * pixBLUE = imageB.getPixels();
        for(int i = 0; i < numPixels; i++) {
            pixBLUE[i] = 255;
        }
        ofxCvGrayscaleImage imageG = imageR;
        // work with green channel pixels to make them all \
        // twice as bright as red channel
        unsigned char * pixGREEN = imageG.getPixels();
        for(int i = 0; i < numPixels; i++) {
            int val = 128 + pixGREEN[i]/2;
            if (val > 255) { //clamp top at 255
                val = 255;
            }
            pixGREEN[i] = val;
        }
		//colorImgMultiply.setFromGrayscalePlanarImages(imageG, imageG, imageG);
		colorImgMultiply.setFromGrayscalePlanarImages(imageR, imageG, imageB);
        colorImgMultiply.mirror(false, true);
        
		// we do two thresholds - one for the far plane and one for the near plane
		// we then do a cvAnd to get the pixels which are a union of the two thresholds
		if(bThreshWithOpenCV) {
			grayThreshNear = grayImage;
			grayThreshFar = grayImage;
			grayThreshNear.threshold(nearThreshold, true);
			grayThreshFar.threshold(farThreshold);
			cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), grayImage.getCvImage(), NULL);
		} else {
			
			// or we do it ourselves - show people how they can work with the pixels
			unsigned char * pix = grayImage.getPixels();
			
			int numPixels = grayImage.getWidth() * grayImage.getHeight();
			for(int i = 0; i < numPixels; i++) {
				if(pix[i] < nearThreshold && pix[i] > farThreshold) {
					pix[i] = 255;
				} else {
					pix[i] = 0;
				}
			}
		}
		
		// update the cv images
		grayImage.flagImageChanged();
		
		// find contours which are between the size of 20 pixels and 1/3 the w*h pixels.
		// also, find holes is set to true so we will get interior contours as well....
		contourFinder.findContours(grayImage, 10, (kinect.width*kinect.height)/2, 20, false);
	}
	
#ifdef USE_TWO_KINECTS
	kinect2.update();
#endif
}

//--------------------------------------------------------------
void testApp::draw() {

    // clear reportStream
    stringstream reportStream;
    reportStream << "screenID = " << screenID << endl;

    
    ////////////
    // draw screen based on display mode
    ////////////
    switch (screenID) {
            
        case 0: // 0 > debug
            //
            ofBackground(255, 128, 0);
            ofSetColor(255, 255, 255);
            
            if(bDrawPointCloud) {
                easyCam.begin();
                drawPointCloud();
                easyCam.end();
            } else {
                // draw from the live kinect
                kinect.drawDepth(10, 10, 400, 300);
                kinect.draw(420, 10, 400, 300);
                
                grayImage.draw(10, 320, 400, 300);
                contourFinder.draw(10, 320, 400, 300);
/*                
#ifdef USE_TWO_KINECTS
                kinect2.draw(420, 320, 400, 300);
#endif
*/
            }
            
            // draw on-screen instructions
            ofSetColor(255, 255, 255);
            
            if(kinect.hasAccelControl()) {
                reportStream << "accel is: " << ofToString(kinect.getMksAccel().x, 2) << " / "
                << ofToString(kinect.getMksAccel().y, 2) << " / "
                << ofToString(kinect.getMksAccel().z, 2) << endl;
            } else {
                reportStream << "Note: this is a newer Xbox Kinect or Kinect For Windows device," << endl
                << "motor / led / accel controls are not currently supported" << endl << endl;
            }
            
            reportStream << "press p to switch between images and point cloud, rotate the point cloud with the mouse" << endl
            << "using opencv threshold = " << bThreshWithOpenCV <<" (press spacebar)" << endl
            << "set near threshold " << nearThreshold << " (press: + -)" << endl
            << "set far threshold " << farThreshold << " (press: < >) num blobs found " << contourFinder.nBlobs
            << ", fps: " << ofGetFrameRate() << endl
            << "press c to close the connection and o to open it again, connection is: " << kinect.isConnected() << endl;
            
            if(kinect.hasCamTiltControl()) {
                reportStream << "press UP and DOWN to change the tilt angle: " << angle << " degrees" << endl
                << "press 1-5 & 0 to change the led mode" << endl;
            }
            
            //ofDrawBitmapString(reportStream.str(), 20, 652);
           
            break;
            
        case 1:
            // 1 > settings, instructions
            // 
            ofBackground(255, 0, 128);
            ofSetColor(255, 255, 255);

            if(kinect.hasAccelControl()) {
                reportStream << "accel is: " << ofToString(kinect.getMksAccel().x, 2) << " / "
                << ofToString(kinect.getMksAccel().y, 2) << " / "
                << ofToString(kinect.getMksAccel().z, 2) << endl;
            } else {
                reportStream << "Note: this is a newer Xbox Kinect or Kinect For Windows device," << endl
                << "motor / led / accel controls are not currently supported" << endl << endl;
            }
            
            reportStream // << "press p to switch between images and point cloud, rotate the point cloud with the mouse" << endl
            << "using opencv threshold = " << bThreshWithOpenCV <<" (press spacebar)" << endl
            << "set near threshold " << nearThreshold << " (press: + -)" << endl
            << "set far threshold " << farThreshold << " (press: < >) num blobs found " << contourFinder.nBlobs
            << ", fps: " << ofGetFrameRate() << endl
            << "press c to close the connection and o to open it again, connection is: " << kinect.isConnected() << endl;
            
            if(kinect.hasCamTiltControl()) {
                reportStream << "press UP and DOWN to change the tilt angle: " << angle << " degrees" << endl
                << "press 1-5 & 0 to change the led mode" << endl;
            }
            
            //ofDrawBitmapString(reportStream.str(), 20, 652);

            break;
            
        case 2: 
            // 2 > activity
            // 
            ofBackground(0, 255, 255);
            ofSetColor(255, 255, 255);
            
            //ofPushMatrix();
            // the projected points are 'backwards' 
            //ofScale(1, -1, 1);
            if(bDrawPointCloud) {
                easyCam.begin();
                drawPointCloud();
                easyCam.end();
            } else {
                
                // show people how they can work with the pixels
                unsigned char * pix = colorImgMultiply.getPixels();
                
                                
                // draw from the live kinect
                //colorImgMultiply.draw(0, 0, 1280, 800);
                // kinect.drawDepth(0, 0, 1280, 800); //drawDepth draws an ofTexture
                colorImgMultiply.draw(-30, -30, 1340, 860);
                // kinect.drawDepth(10, 10, 400, 300);
                // kinect.draw(420, 10, 400, 300);
                
                // grayImage.draw(10, 320, 400, 300);
                // contourFinder.draw(10, 320, 400, 300);
                /*                
                 #ifdef USE_TWO_KINECTS
                 kinect2.draw(420, 320, 400, 300);
                 #endif
                 */
            }
            //ofPopMatrix();
            
            // draw on-screen instructions
            ofSetColor(255, 255, 255);
            // stringstream reportStream;
            
            if(kinect.hasAccelControl()) {
                reportStream << "accel is: " << ofToString(kinect.getMksAccel().x, 2) << " / "
                << ofToString(kinect.getMksAccel().y, 2) << " / "
                << ofToString(kinect.getMksAccel().z, 2) << endl;
            } else {
                reportStream << "Note: this is a newer Xbox Kinect or Kinect For Windows device," << endl
                << "motor / led / accel controls are not currently supported" << endl << endl;
            }
            
            reportStream // << "press p to switch between images and point cloud, rotate the point cloud with the mouse" << endl
            << "using opencv threshold = " << bThreshWithOpenCV <<" (press spacebar)" << endl
            << "set near threshold " << nearThreshold << " (press: + -)" << endl
            << "set far threshold " << farThreshold << " (press: < >) num blobs found " << contourFinder.nBlobs
            << ", fps: " << ofGetFrameRate() << endl
            << "press c to close the connection and o to open it again, connection is: " << kinect.isConnected() << endl;
            
            if(kinect.hasCamTiltControl()) {
                reportStream << "press UP and DOWN to change the tilt angle: " << angle << " degrees" << endl
                << "press 1-5 & 0 to change the led mode" << endl;
            }
            
            //ofDrawBitmapString(reportStream.str(), 20, 652);
            
            
            break;
            
        case 3: 
            // 3 > workout stats, credits
            // 
            ofBackground(128, 0, 255);
            ofSetColor(255, 255, 255);
            
            if(kinect.hasAccelControl()) {
                reportStream << "accel is: " << ofToString(kinect.getMksAccel().x, 2) << " / "
                << ofToString(kinect.getMksAccel().y, 2) << " / "
                << ofToString(kinect.getMksAccel().z, 2) << endl;
            } else {
                reportStream << "Note: this is a newer Xbox Kinect or Kinect For Windows device," << endl
                << "motor / led / accel controls are not currently supported" << endl << endl;
            }
            
            reportStream // << "press p to switch between images and point cloud, rotate the point cloud with the mouse" << endl
            << "using opencv threshold = " << bThreshWithOpenCV <<" (press spacebar)" << endl
            << "set near threshold " << nearThreshold << " (press: + -)" << endl
            << "set far threshold " << farThreshold << " (press: < >) num blobs found " << contourFinder.nBlobs
            << ", fps: " << ofGetFrameRate() << endl
            << "press c to close the connection and o to open it again, connection is: " << kinect.isConnected() << endl;
            
            if(kinect.hasCamTiltControl()) {
                reportStream << "press UP and DOWN to change the tilt angle: " << angle << " degrees" << endl
                << "press 1-5 & 0 to change the led mode" << endl;
            }
            
            //ofDrawBitmapString(reportStream.str(), 20, 652);
            
            break;
        
        default:
            break;
    }
    
    ofDrawBitmapString(reportStream.str(), 20, 652);
    
    
    // display calories burned and bpm
    if (ofGetElapsedTimeMillis() > 1000 ) {
        cals = ofToString(ofRandom(1.284,  2.471));// (min, max)
        cals = cals.substr(0, 5);
        bpm = ofToString(ofRandom(125, 185));
        bpm = bpm.substr(0, 3);
        labelCAL = "CALORIES PER MINUTE";
        labelBPM = "HEARTBEATS PER MINUTE";
        
        ofResetElapsedTimeCounter();
    }
    ofEnableAlphaBlending();
    
    ofSetColor(255, 128, 0, 255);
    ofRect(980, 15, 1200, 145);
    
    ofSetColor(128, 255, 0, 255);
    ofRect(980, 165, 1200, 145);

    ofDisableAlphaBlending();

    ofSetColor(255, 255, 255);
    fontSMALL.drawString(labelCAL, 1000, 50);
    font.drawString(cals, 1000, 132);
    fontSMALL.drawString(labelBPM, 1000, 200);
    font.drawString(bpm, 1000, 282);
    
}

void testApp::drawPointCloud() {
	int w = 640;
	int h = 480;
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_POINTS);
	int step = 2;
	for(int y = 0; y < h; y += step) {
		for(int x = 0; x < w; x += step) {
			if(kinect.getDistanceAt(x, y) > 0) {
				mesh.addColor(kinect.getColorAt(x,y));
				mesh.addVertex(kinect.getWorldCoordinateAt(x, y));
			}
		}
	}
	glPointSize(3);
	ofPushMatrix();
	// the projected points are 'upside down' and 'backwards' 
	ofScale(1, -1, -1);
	ofTranslate(0, 0, -1000); // center the points a bit
	glEnable(GL_DEPTH_TEST);
	mesh.drawVertices();
	glDisable(GL_DEPTH_TEST);
	ofPopMatrix();
}

//--------------------------------------------------------------
void testApp::exit() {
	kinect.setCameraTiltAngle(0); // zero the tilt on exit
	kinect.close();
	
#ifdef USE_TWO_KINECTS
	kinect2.close();
#endif
}

//--------------------------------------------------------------
void testApp::keyPressed (int key) {
	switch (key) {
		case ' ':
			bThreshWithOpenCV = !bThreshWithOpenCV;
			break;
			
		case'p':
			bDrawPointCloud = !bDrawPointCloud;
			break;
			
		case '>':
		case '.':
			farThreshold ++;
			if (farThreshold > 255) farThreshold = 255;
			break;
			
		case '<':
		case ',':
			farThreshold --;
			if (farThreshold < 0) farThreshold = 0;
			break;
			
		case '+':
		case '=':
			nearThreshold ++;
			if (nearThreshold > 255) nearThreshold = 255;
			break;
			
		case '-':
			nearThreshold --;
			if (nearThreshold < 0) nearThreshold = 0;
			break;
			
		case 'w':
			kinect.enableDepthNearValueWhite(!kinect.isDepthNearValueWhite());
			break;
			
		case 'o':
			kinect.setCameraTiltAngle(angle); // go back to prev tilt
			kinect.open();
			break;
			
		case 'c':
			kinect.setCameraTiltAngle(0); // zero the tilt
			kinect.close();
			break;
			
		case '1':
			kinect.setLed(ofxKinect::LED_GREEN);
			break;
			
		case '2':
			kinect.setLed(ofxKinect::LED_YELLOW);
			break;
			
		case '3':
			kinect.setLed(ofxKinect::LED_RED);
			break;
			
		case '4':
			kinect.setLed(ofxKinect::LED_BLINK_GREEN);
			break;
			
		case '5':
			kinect.setLed(ofxKinect::LED_BLINK_YELLOW_RED);
			break;
			
		case '0':
			kinect.setLed(ofxKinect::LED_OFF);
			break;
			
		case OF_KEY_UP:
			angle++;
			if(angle>30) angle=30;
			kinect.setCameraTiltAngle(angle);
			break;
			
		case OF_KEY_DOWN:
			angle--;
			if(angle<-30) angle=-30;
			kinect.setCameraTiltAngle(angle);
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

        case 's': 
            screenID = 1; // display settings, instructions screen
            break;
            
        case 'a': 
            screenID = 2; // display activity screen
            break;
            
        case 'x': 
            screenID = 3; // display workout stats, credits screen
            break;
            
        default:
            break;
	}
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h)
{}
