#include "testApp.h"

// TODO: develop credits, including 
// - audioOutputExample source code
// - James M's contributions: 1) working through math for linear tone/mouse relationship, and 2) square wave

// TODO:
// - debug elaphantitis of 8th particle system (being huge when it shouldn't be)
// - clean remnants of Partisynth code out from testApp since it no longer synthesizes anything or has any particles
// - kinput > use kinetic data to drive partisynth pan (x), pitch (y), and volume (z)
// - aggregate screenshake, and screenflicker into main app so that any user can affect it
// - init() > has soundstream issues > should make this into a global "setup" function that can be called from testApp instead of having to be inside testApp so it can be packaged with "synth" when I make that into an addon

//--------------------------------------------------------------
void partisynthmngr::setup(){
    
    ofSetVerticalSync(TRUE);
	ofSetFrameRate(60);
    updatePartisynths = 0;
    
    numPartisynths = 8;
    // numPartisynths = 0;
    
    // Partisynth creation
    partisynths.clear();
    for (int i=0; i < numPartisynths; i++) {
        Partisynth ps;
        partisynths.push_back(ps);
    }
    
	// Partisynth initialization
    for (int i=0; i < partisynths.size(); i++) {
        partisynths[i].init(0.5f);
    }
    
    numPartisynths = 1;
    
    
    // populate emitters list
    numEmitters = 3;
    emitters.clear();
    for (int i=0; i < numEmitters; i++) {
        ofxParticleEmitter emitter;
        emitters.push_back(emitter);
    }

	// load emitter settings from XML
    for (int i=0; i < emitters.size(); i++) {
        if ( !emitters[i].loadFromXml( "circles_subdued.pex"/*"drugs_subdued.pex"*/ ) )
        {
            ofLog( OF_LOG_ERROR, "testApp::setup() - failed to load emitter[" + ofToString(i) + "] config" );
        }    
    }
    
	// 2 output channels,
	// 0 input channels
	// 22050 samples per second
	// 512 samples per buffer
	// 4 num buffers (latency)
	
	int bufferSize              = 512;
	sampleRate                  = 44100;
	phase                       = 0;
	phaseAdder                  = 0.0f;
	phaseAdderTarget            = 0.0f;
    phaseAdderTargetTween       = 0.5f;
	volume                      = 0.25f;
	bNoise                      = false;
    paused                      = false;
    waveform                    = 's';
    targetFrequency             = 440.0f;
    pan                         = 0.5f;
    volumeFrequencyAdjustment   = 1.0f;
    volumeWaveformAdjustment    = 0.9995f; // will sort of not work if you assign any values >=1.0f
    volumeAdjustment            = volumeFrequencyAdjustment * volumeWaveformAdjustment;
    
    updateParticleTexture       = true;
    xmlFilename                 = "circles_subdued.pex";
    texFilename                 = "0005_circle.png";
    
    screenID                    = 'e';
    
    // ===============----

	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);
	
	//soundStream.listDevices();
	
	//if you want to set the device id to be different than the default
	//soundStream.setDeviceID(1); 	//note some devices are input only and some are output only 

	// soundStream.setup(this, 2, 0, sampleRate, bufferSize, 4);
    
    setPhaseAdderTarget();
}


//--------------------------------------------------------------
void partisynthmngr::update(){
    
    ///*
    
    if (updatePartisynths > 0 ) {
        cout << "====" << endl;
        cout << "== update() ==" << endl;
        cout << "updatePartisynths = 1" << endl;
        cout << "numPartisynths = " << numPartisynths << endl;
        cout << "partisynths.size() = " << partisynths.size() << endl;

        if (partisynths.size() == numPartisynths) {
            /*
            cout << "(parti partisynths.size() == numPartisynths) = true"
            << endl;
            cout << "creating new partisynth" << endl;
            Partisynth ps;
            partisynths.push_back(ps);
            cout << "partisynths.size() = " << partisynths.size() << endl;
             */
        } else {
            cout << "initializing: partisynths[" << numPartisynths << "]" << endl;
            partisynths[partisynths.size() - 1].init();
            cout << "incrementing: numPartisynths " << endl;
            numPartisynths++;
            cout << "numPartisynths = " << numPartisynths << endl;
            cout << "====" << endl;
            
            updateSizeAdustments();
        }
        updatePartisynths = 0;
    } else if (updatePartisynths < 0){
        if (numPartisynths > 1) {
            cout << "====" << endl;
            cout << "== update() ==" << endl;
            cout << "updatePartisynths = -1" << endl;
            cout << "numPartisynths = " << numPartisynths << endl;
            cout << "partisynths.size() = " << partisynths.size() << endl;

            cout << "decrementing: numPartisynths " << endl;
            numPartisynths--;
            cout << "numPartisynths = " << numPartisynths << endl;
            // partisynths[numPartisynths].exit();
            cout << "====" << endl;

            updateSizeAdustments();
        }
        updatePartisynths = 0;
    }
    //*/

    
    // Partisynth updates
    if (numPartisynths > 0) {
        for (int i=0; i < numPartisynths; i++) {
            // TODO: Do I need some kind of test in here to make sure partisynths[i] exists?
            // if (partisynths[i].active)
                partisynths[i].update();
        }
    }
    
    // Partisynth mouse moved calls
    for (int i=0; i < numPartisynths; i++) {
        int x = ofGetMouseX();
        int y = ofGetMouseY();
        // set default values so partisynths don't get stuck in the upper left of the screen
        if (!x && !y) {
            x = ofGetWidth() / 2.0f;
            y = ofGetHeight() / 2.0f;
        }
        float rps = 1.0f / (i+1);
        float seconds = ofGetElapsedTimeMillis() / 1000.0f; // TODO: should eventually use beats to relate this to music more instead of seconds
        float radians = TWO_PI * seconds * rps; 
        partisynths[i].mouseMoved(x + 100 * i * sin(radians), y + 100 * i * cos(radians));
    }
    
    updateEmitters();
}

void partisynthmngr::updateSizeAdustments(){
    float sizeAdjustment = sqrt(sqrt(1.0f / (float)numPartisynths));
    cout << "====" << endl;
    cout << "updateSizeAdustments(): sizeAdjustment = " << sizeAdjustment << endl;
    for (int i=0; i<partisynths.size(); i++){
        // partisynths[i].sizeAdjustment = (numPartisynths - i) * pow(sizeAdjustment, i+1);
        partisynths[i].sizeAdjustment = sizeAdjustment / (i+1);
        cout << "calculating partisynths["<<i<<"].sizeAdjustment = " << partisynths[i].sizeAdjustment << endl;
    }
    updateProperties();
    cout << "====" << endl;
}

void partisynthmngr::updateEmitters(){
    for (int i=0; i < emitters.size(); i++) {
        if (updateParticleTexture) {
            emitters[i].loadFromXml(xmlFilename);
            updateProperties();
            // emitters[i].maxParticles = 3000;
            // emitters[i].changeTexture(filename);
        }
        
        if (paused != emitters[i].paused) {
            emitters[i].pause();
        }

        // particle emission angle rotates according to sample volume
        emitters[i].angle = (360/emitters.size()*2) * (lAudio[0] + rAudio[0] + 1.0f) * volumeAdjustment + i * (360 / emitters.size());
        // make colors more eratic as pitch increases
        emitters[i].startColor.red = targetFrequency / 10000.0f;
        emitters[i].startColorVariance.red = heightPct/2;
        emitters[i].startColor.green = heightPct/2;
        emitters[i].startColorVariance.green = heightPct;
        emitters[i].startColor.blue = 1 - (1-heightPct/2);
        emitters[i].startColor.alpha = heightPct*2;
        emitters[i].finishColor.red     = 0.0f;
        emitters[i].finishColor.green   = 0.0f;
        emitters[i].finishColor.blue    = 0.0f;
        emitters[i].finishColor.alpha   = 0.0f;
        emitters[i].startParticleSize = 10 / (1.05f-volume) + (1-heightPct/2)  * 60.0f;
        emitters[i].startParticleSizeVariance = 0.0f; // 20 + (1-heightPct)  * 60.0f;
        emitters[i].finishParticleSize = 1 + heightPct  * 60.0f;
        emitters[i].finishParticleSizeVariance = 0.0f; // 20 + (1-heightPct)  * 60.0f;
        emitters[i].speed = 40 + heightPct * 5000.0f * volume;
        emitters[i].speedVariance = 0.0f;
        emitters[i].particleLifespan = MIN( 0.5f + 100.0f / emitters[i].speed, 30.0f);
        
        // TODO: update more emitter properties based on waveform (texture shape), and other properties    
        
        
        emitters[i].update();
    }
    
    if (updateParticleTexture) {
        updateParticleTexture = false;
    }

}

void partisynthmngr::updateProperties(){

    int x = ofGetMouseX();
    int y = ofGetMouseY();

    updateProperties(x, y);    
}

void partisynthmngr::updateProperties(int x, int y){
    
    int width = ofGetWidth();
	float height = ofGetHeight();

    // set default values so we don't get ridiculous pitches at start of program
    if (!x && !y) {
        x = width / 2.0f;
        y = height / 2.0f;
    }
    
    pan = (float)x / (float)width;
	heightPct = ((height-y) / height);

    // linear relationship between frequency and mouse Y
    // targetFrequency = 2000.0f * heightPct); 
    // exponential relationship between frequency and mouse Y
    // TODO: could make this "snap" to chromatic scales
	targetFrequency = 100.0f * pow(1.059463094359f, heightPct*75.0f); 
    
	setPhaseAdderTarget();
        
    for (int i=0; i < emitters.size(); i++) {
        emitters[i].sourcePosition.x = x;
        emitters[i].sourcePosition.y = y;
    }
}

//--------------------------------------------------------------
void partisynthmngr::draw(){
    
  
    string  screenLabel         = "PARTISYNTH";
    bool    screenShake         = false;
    bool    screenFlicker       = false;

    bool    drawEmitter         = false;
    bool    drawWaveforms       = false;
    bool    drawInstructions    = false;
    bool    drawReport          = false;

    ofPushMatrix();

    // set elements to display based on screenID
    switch (screenID) {
        
        case 'e': // emitter screen
            screenLabel         += ": emitter screen";
            if (!paused) {
                screenFlicker       = true;
                screenShake         = true;
            }
            drawEmitter         = true;
            drawInstructions    = true;
            // drawReport          = true;
            break;
       
        case 'd': // debug screen
        default: // modified from original audioOutputExample application
            screenLabel += ": debug screen";
            // drawWaveforms = true;
            drawEmitter = true;
            drawInstructions = true;
            // drawReport = true;
            break;
    }

    if (screenShake) {
        int mult = 5;
        ofTranslate(ofRandomf() * volume * pow(targetFrequency,mult)    / pow(2000.0f, mult  ), 
                    ofRandomf() * volume * pow(targetFrequency,mult)    / pow(2000.0f, mult  ), 
                    ofRandomf() * volume * pow(targetFrequency,mult/2)  / pow(2000.0f, mult/2) );        
    }

    if (screenFlicker) {
        float mult = 5;
        ofBackground(16     + ofRandom( pow(targetFrequency, mult)      / pow(2048.0f, mult ) ), 
                     64     + ofRandom( pow(targetFrequency, mult)      / pow(2048.0f, mult ) ),
                     128    + ofRandom( pow(targetFrequency, mult)      / pow(2048.0f, mult ) ) );
    }
    else {
        ofBackground(16, 64, 128);
    }

    if (drawInstructions) {
        // instructions
        ofSetColor(225,255,0);
        screenLabel += " | fps: " + ofToString( ofGetFrameRate(), 2 );
        ofDrawBitmapString(screenLabel, 32, 32);
        string instructions = "press 'p' to paurse/unpause the audio\n";
        instructions += "press 's' for sinewave (default)\n";
        instructions += "press 't' for triangle wave\n";
        instructions += "press 'i' for irregular triangle";
        instructions += "press 'w' for sawtooth wave //////\n";
        instructions += "press 'W' for sawtooth wave \\\\\\\\\\\\\n";
        instructions += "press left mouse button more more particles\n";
        instructions += "press right mouse button for less particles\n";
        ofDrawBitmapString(instructions, 31, 62);
    
    }
    
    if (drawEmitter) {
        
        // Partisynth drawing calls
        for (int i=0; i < numPartisynths; i++) {
            partisynths[i].draw();
        }
        
        /*
        for (int i=0; i < emitters.size(); i++) {
            emitters[i].draw( 0, 0 );
        }
         */

    }
    
    if (drawWaveforms) {
        ////////
        // draw waveforms
        ////////
        ofNoFill();
        
        // draw the left channel:
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(32, 150, 0);
        
        ofSetColor(225);
        ofDrawBitmapString("Left Channel", 4, 18);
        
        ofSetLineWidth(1);	
        ofRect(0, 0, 900, 200);
        
        ofSetColor(245, 58, 135);
        ofSetLineWidth(3);
        
        ofBeginShape();
        for (unsigned int i = 0; i < lAudio.size(); i++){
            float x =  ofMap(i, 0, lAudio.size(), 0, 900, true);
            ofVertex(x, 100 -lAudio[i]*180.0f);
        }
        ofEndShape(false);
        
        ofPopMatrix();
        ofPopStyle();
        
        // draw the right channel:
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(32, 350, 0);
        
        ofSetColor(225);
        ofDrawBitmapString("Right Channel", 4, 18);
        
        ofSetLineWidth(1);	
        ofRect(0, 0, 900, 200);
        
        ofSetColor(245, 58, 135);
        ofSetLineWidth(3);
        
        ofBeginShape();
        for (unsigned int i = 0; i < rAudio.size(); i++){
            float x =  ofMap(i, 0, rAudio.size(), 0, 900, true);
            ofVertex(x, 100 -rAudio[i]*180.0f);
        }
        ofEndShape(false);
        
        ofPopMatrix();
        ofPopStyle();
    }
    
    if (drawReport) {
        // display report
        ofSetColor(225, 255, 0);
        
        // decide which waveform to include in reportString
        string waveformString;
        switch (waveform) {
            case 't':
                waveformString = "triangle";
                break;
            case 'i':
                waveformString = "irregular triangle";
                break;
            case 'w':
                waveformString = "sawtooth";
                break;
            case 'W':
                waveformString = "sawtooth reversed";
                break;
            case 'q':
                waveformString = "square";
                break;
            case 's':
            default:
                waveformString = "sine";
                break;
        }
        
        // left column report        
        string reportString = "volume: ("+ofToString(volume, 2)+") modify with -/+ keys\npan: ("+ofToString(pan, 2)+") modify with mouse x\nsynthesis: ";
        if( !bNoise ){
            reportString += waveformString + " wave (" + ofToString(targetFrequency, 2) + "hz) modify with mouse y";
            reportString += "\nsmoothing: (" + ofToString(phaseAdderTargetTween, 2) + ") modify with 'up' and 'down' arrows";
            reportString += "\nvolumeWaveformAdjustment: ("+ofToString(volumeWaveformAdjustment, 5)+")";
            reportString += "\nvolumeFrequencyAdjustment: ("+ofToString(volumeFrequencyAdjustment, 5)+")";
            reportString += "\nvolumeAdjustment: ("+ofToString(volumeAdjustment, 5)+")";                
        }
        else{
            reportString += "noise";	
        }
        ofDrawBitmapString(reportString, 32, 579);
        // right column report
        if( !bNoise ){
            reportString = "heightPct: ("+ofToString(heightPct, 2)+") modify with mouse y";
            reportString += "\n";
        }
        ofDrawBitmapString(reportString, 632, 579);        
    }

    ofPopMatrix();
}


//--------------------------------------------------------------
void partisynthmngr::keyPressed  (int key){
    
    for (int i = 0; i < partisynths.size(); i++){ 
        // keyPressed event needs to be passed to all partisynths so waveformes remain consistent
        partisynths[i].keyPressed(key);
    }

    string  filename                    = "";

    if (key == '-' || key == '_' ){
		volume -= 0.05;
		volume = MAX(volume, 0);
	} 
    else if (key == '+' || key == '=' ){
		volume += 0.05;
		volume = MIN(volume, 1);
	}
	
    /* // TODO: cleanup sounstream stuff -> needs to be set up in class that exetends ofBaseApp
	if( key == 'p' ){
        if (paused) {
            soundStream.start();
            paused = false;
        } 
        else {
            soundStream.stop();
            paused = true;
        }
	}
    //*/
    
    // handle screen ids and waveform id
    switch (key) {
        case 'e': // emitter screen
        case 'd': // debug screen
            screenID = key;
            break;
            
        case 't': // triangle
            xmlFilename = "triangles_subdued.pex";
            texFilename = "0003_triangle.png";
            updateParticleTexture = true;
            break;
        case 'i': // irregular triangle
            xmlFilename = "trianglesIrregular_subdued.pex";
            texFilename = "0002_triangle_irregular.png";
            updateParticleTexture = true;
            break;
        case 'w': // sawtooth
            xmlFilename = "sawtooth_subdued.pex";
            texFilename = "0001_sawtooth.png";
            updateParticleTexture = true;
            break;
        case 'W': // sawtooth reversed
            xmlFilename = "sawtooth_reversed_subdued.pex";
            texFilename = "0000_sawtooth_reversed.png";
            updateParticleTexture = true;
            break;
        case 'q': // square
            xmlFilename = "squares_subdued.pex";
            texFilename = "0004_square.png";
            updateParticleTexture = true;
            break;
        case 's': // sine
            xmlFilename = "circles_subdued.pex";
            texFilename = "0005_circle.png";
            updateParticleTexture = true;
            break;
        default:
            //TODO: some key input message here?
            break;
    }
    if (updateParticleTexture) {
        waveform = key;        
    }
    
    // modify frequency smoothing
	if (key == OF_KEY_DOWN){
		phaseAdderTargetTween -= 0.05f;
		phaseAdderTargetTween = MAX(phaseAdderTargetTween, 0.0f);
	} 
    else if (key == OF_KEY_UP){
		phaseAdderTargetTween += 0.05f;
		phaseAdderTargetTween = MIN(phaseAdderTargetTween, 0.95f);
	}
    
    
}

//--------------------------------------------------------------
void partisynthmngr::keyReleased  (int key){

}

//--------------------------------------------------------------
void partisynthmngr::mouseMoved(int x, int y ){
    
    updateProperties(x, y);
    
}

//--------------------------------------------------------------
void partisynthmngr::setPhaseAdderTarget () {
    phaseAdderTarget = (targetFrequency / (float) sampleRate) * TWO_PI;
}

//--------------------------------------------------------------
void partisynthmngr::mouseDragged(int x, int y, int button){

    mouseMoved(x, y);

}

//--------------------------------------------------------------
void partisynthmngr::mousePressed(int x, int y, int button){
	bNoise = true;
    
    cout << "====" << endl;
    cout << "mousePressed(): button = " << button << endl;

    
    switch (button) {
        case OF_MOUSE_BUTTON_1:
            if (!ofGetKeyPressed(OF_KEY_CONTROL)) {
                // increase number of partisynths
                updatePartisynths = 1;
            } else {
                cout << "OF_KEY_CONTROL = pressed" << button << endl;
                // decrease number of partisynths
                updatePartisynths = -1;
            }
            break;
                
        case OF_MOUSE_BUTTON_2:
        default:
            // decrease number of partisynths
            updatePartisynths = -1;
            break;
    }
    
    // hand event down to active particles so they get mouse pressed functions
    for (int i = 0; i < partisynths.size(); i++) {
        partisynths[i].mousePressed(x, y, button);
    }
}


//--------------------------------------------------------------
void partisynthmngr::mouseReleased(int x, int y, int button){
	bNoise = false;

    // hand event down to active particles so they get mouse pressed functions
    for (int i = 0; i < partisynths.size(); i++) {
        partisynths[i].mouseReleased(x, y, button);
    }
}

//--------------------------------------------------------------
void partisynthmngr::windowResized(int w, int h){

}

//--------------------------------------------------------------
void partisynthmngr::audioOut(float * output, int bufferSize, int nChannels){
    
    // clear audio buffers
    for (int i = 0; i < bufferSize; i++){
        output[i*nChannels    ] = 0; 
        output[i*nChannels + 1] = 0;
    }

    // have partisynths write their audio to the buffer
    for (int i=0; i < numPartisynths; i++) {
        partisynths[i].audioOut(output, bufferSize, nChannels); 
    }
}

//--------------------------------------------------------------
void partisynthmngr::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void partisynthmngr::dragEvent(ofDragInfo dragInfo){ 

}
