

#include "partisynth.h"

// TODO: (longterm)
// - develop credits, including:
//      - audioOutputExample source code
//      - James M's contributions: 
//          1) working through math for linear tone/mouse relationship, and 
//          2) square wave
// - separate synth into an addon
// - create functionality to put debug reports into a GUI (to be called from main app) so that all partisynth properties can be read/accessed, etc

//--------------------------------------------------------------

Partisynth::Partisynth(){
    screenID                    = 'e';
    numEmitters                 = 3;
    updateParticleTexture       = false;
    xmlFilename                 = "circles_subdued.pex";
    texFilename                 = "0005_circle.png";
    
    
    sizeAdjustment              = 1.0f;
	phase                       = 0;
	phaseAdder                  = 0.0f;
	phaseAdderTarget            = 0.0f;
    phaseAdderTargetTween       = 0.9f;
	volume                      = 1.0f;
	bNoise                      = false;
    paused                      = false;
    waveform                    = 's';
    targetFrequency             = 440.0f;
    pan                         = 0.5f;
    volumeFrequencyAdjustment   = 1.0f;
    volumeWaveformAdjustment    = 0.9995f; // will sort of not work if you assign any values >=1.0f
    volumeAdjustment            = volumeFrequencyAdjustment * volumeWaveformAdjustment;
    
    
    // ===============----
    
 	sampleRate                  = 44100; 
}

Partisynth::~Partisynth(){
    exit();
}

void Partisynth::exit(){
    if ( lAudio.size() != 0 ) {
        lAudio.clear();
    }
    //lAudio = NULL;
    
    if ( rAudio.size() != 0 ) {
        rAudio.clear();    
    }
    //rAudio = NULL;

    if ( emitters .size() != 0 ) {
        for (int i = 0; i < emitters.size(); i++) {
            emitters[i].exit();
        }
        emitters.clear();
    }
    //emitters = NULL;
    
}

void Partisynth::init(){
    init(1.0f);
}

void Partisynth::init(float multiplier){
//*    
	
	volume                      = multiplier;

    // ===============----
    
	// 2 output channels,
	// 0 input channels
	// 22050 samples per second
	// 512 samples per buffer
	// 4 num buffers (latency)
    int bufferSize              = 512;
	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);
    // TODO: trouble-shoot soundstream, is acting funny
    // "No matching member function for call to 'setup'
	//soundStream.setup(this, 2, 0, sampleRate, bufferSize, 4);

    // NOTES:
    //soundStream.listDevices();
	//if you want to set the device id to be different than the default
	//soundStream.setDeviceID(1); 	//note some devices are input only and some are output only 
    
    // populate emitter array
    emitters.clear();
    for (int i=0; i < numEmitters; i++) {
        ofxParticleEmitter emitter;
        emitters.push_back(emitter);
    }
    // load emitter settings from saved ".pex" file
    for (int i=0; i < emitters.size(); i++) {
        if ( !emitters[i].loadFromXml( xmlFilename ) )
        {
            ofLog( OF_LOG_ERROR, "testApp::setup() - failed to load emitter[" + ofToString(i) + "] config" );
        }    
    }

    setPhaseAdderTarget();
    updateProperties();
//*/
}

//--------------------------------------------------------------
void Partisynth::update(){
///*
    updateEmitters();
//*/
}

void Partisynth::updateEmitters(){
    for (int i=0; i < emitters.size(); i++) {
        if (updateParticleTexture) {
            emitters[i].loadFromXml(xmlFilename);
            updateProperties();
        }
        
        if (paused != emitters[i].paused) {
            emitters[i].pause();
        }

        // particle emission angle rotates according to sample volume
        emitters[i].angle = (360/emitters.size()*2) * (lAudio[0] + rAudio[0] + 1.0f) + i * (360 / emitters.size());
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
        emitters[i].startParticleSize = 0.5f / (1.05f-volume) + (heightPct) * 60.0f * sizeAdjustment;
        emitters[i].startParticleSizeVariance = 0.0f; // 20 + (1-heightPct)  * 60.0f;
        emitters[i].finishParticleSize = (1-heightPct) * 120.0f * sizeAdjustment;
        emitters[i].finishParticleSizeVariance = 0.0f; // 20 + (1-heightPct)  * 60.0f;
        emitters[i].speed = 40 + heightPct * 2500.0f * volume * sizeAdjustment;
        emitters[i].speedVariance = 0.0f;
        emitters[i].particleLifespan = MIN( 0.5f + 100.0f / emitters[i].speed, 30.0f);
        
        // TODO: update more emitter properties based on waveform (texture shape), and other properties    
        
        
        emitters[i].update();
    }
    
    if (updateParticleTexture) {
        updateParticleTexture = false;
    }

}

void Partisynth::updateProperties(){
    
    int x = ofGetMouseX();
    int y = ofGetMouseY();
    
    updateProperties(x, y);    
}

void Partisynth::updateProperties(int x, int y){
    
    int width = ofGetWidth();
	float height = ofGetHeight();

    // set default values so we don't get ridiculous pitches at start of program
    if (!x && !y) {
        x = width / 2.0f;
        y = height / 2.0f;
    }
    
    // application can crash if you try to set things to move
    // to values less than zero and 
    if (x < 0){
        x = 0;
    } else if (x > width) {
        x = width;
    }
    if (y < 0) {
        y = 0;
    } else if (y > height) {
        y = height;
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
void Partisynth::setPhaseAdderTarget () {
    phaseAdderTarget = (targetFrequency / (float) sampleRate) * TWO_PI;
}

//--------------------------------------------------------------
void Partisynth::audioOut(float * output, int bufferSize, int nChannels){
	//pan = 0.5f;
	float leftScale = 1 - pan;
	float rightScale = pan;
    
	// sin (n) seems to have trouble when n is very large, so we
	// keep phase in the range of 0-TWO_PI like this:
	while (phase > TWO_PI){
		phase -= TWO_PI;
	}
    
	if ( bNoise == true){
		// ---------------------- noise --------------
		for (int i = 0; i < bufferSize; i++){
            volumeWaveformAdjustment = 0.30f;
			lAudio[i] = ofRandom(-1, 1) * volume * leftScale;
			rAudio[i] = ofRandom(-1, 1) * volume * rightScale;
            output[i*nChannels    ] += lAudio[i] * volumeWaveformAdjustment * sizeAdjustment; 
            output[i*nChannels + 1] += rAudio[i] * volumeWaveformAdjustment * sizeAdjustment;
		}
	} 
    else {
        // smooth frequency changes
		phaseAdder = phaseAdderTargetTween * phaseAdder + (1-phaseAdderTargetTween) * phaseAdderTarget; 
		// phaseAdder = phaseAdderTarget; // no smoothing between frequency changes
		for (int i = 0; i < bufferSize; i++){
			phase += phaseAdder;
            
            float sample;
            
            // sometimes we only care about phase in terms of a percentage of TWO_PI
            float phaseClamped = phase;
            while (phaseClamped > TWO_PI){
                phaseClamped -= TWO_PI;
            }
            float pct = phaseClamped / TWO_PI;
            // sometimes you wan to mutlitpy in negative for first half of phase
            float multiplier;
            multiplier = (pct < 0.5f) ? 1.0f : -1.0f;
            
            //TODO: Make this waveform generator into an addon
            //      - make attenuation better than this?
            
            switch (waveform) {
                    
                case 't': // triangle wave /\/\/\/\/
                    pct -= (pct>0.5f) ? 0.5f : 0.0f;
                    sample = multiplier * (2.0f * (pct*2.0f) -1.0f);
                    volumeWaveformAdjustment = 0.400f;
                    break;
                    
                case 'i': // irregular triangle wave /\/\/\/\/ but wierder, looks like an actual saw blade
                    pct += (pct<0.5f) ? 0.5f : 0.0f;
                    sample = multiplier * (2.0f * pct -1.0f);
                    volumeWaveformAdjustment = 0.200f;
                    break;
                    
                case 'w' : // sawtooth wave "////////////"
                    sample = 2.0f * pct -1.0f;
                    volumeWaveformAdjustment = 0.125f;
                    break;
                    
                case 'W': // sawtooth wave "\\\\\\\\\\\\\\"
                    sample = -1 * (2.0f * pct -1.0f);
                    volumeWaveformAdjustment = 0.125f;
                    break;
                    
                case 'q': 
                    // square wave
                    sample = (sin(phase) > 0) ? 1 : -1;
                    volumeWaveformAdjustment = 0.050f;
                    break;
                    
                case 's': // sine wave (default)
                default:
                    sample = sin(phase);
                    volumeWaveformAdjustment = 0.9995f;
                    break;
            }
            
			lAudio[i] = sample * volume * leftScale;
			rAudio[i] = sample * volume * rightScale;
            // Adjust ouput volume based on both target frequency and waveform
            volumeWaveformAdjustment = 1.0f - ( (1.0f - volumeWaveformAdjustment) / (1.0f + heightPct));
            volumeFrequencyAdjustment = 0.0001f + pow(0.9f, targetFrequency/250.0f);
            volumeAdjustment = volumeFrequencyAdjustment *  volumeWaveformAdjustment;
            output[i*nChannels    ] += lAudio[i] * volumeAdjustment * sqrt(sizeAdjustment); 
            output[i*nChannels + 1] += rAudio[i] * volumeAdjustment * sqrt(sizeAdjustment);
		}
	} 
}

//--------------------------------------------------------------
void Partisynth::draw(){
///*    
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
            //screenLabel         += ": emitter screen";
            if (!paused) {
                // screenFlicker       = true; // TODO: make these be handled externally
                // screenShake         = true;
            }
            drawEmitter         = true;
            // drawInstructions    = true;
            // drawReport          = true;
            break;
       
        case 'd': // debug screen
        default: // modified from original audioOutputExample application
            //screenLabel += ": debug screen";
            //drawWaveforms = true;
            //drawEmitter = true;
            //drawInstructions = true;
            drawReport = true;
            break;
    }

    if (screenShake) {
        ofTranslate(ofRandomf() * volume * pow(targetFrequency,2)/400000, 
                    ofRandomf() * volume * pow(targetFrequency,2)/400000, 
                    ofRandomf() * volume * targetFrequency       /400);        
    }

/*
 
    if (screenFlicker) {
        ofBackground(16     + ofRandom( targetFrequency /100 ), 
                     64     + ofRandom( heightPct       /50 ), 
                     128    + ofRandom( targetFrequency /25 ));
    }
    else {
        ofBackground(16, 64, 128);
    }
     
//*/

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
        instructions += "press 'q' for square wave\n";
        ofDrawBitmapString(instructions, 31, 62);
    
    }
    
    if (drawEmitter) {
        for (int i=0; i < emitters.size(); i++) {
            emitters[i].draw( 0, 0 );
        }
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
//*/
}

//--------------------------------------------------------------
void Partisynth::keyPressed(int key){

    string  filename                    = "";

    if (key == '-' || key == '_' ){
		volume -= 0.05;
		volume = MAX(volume, 0);
	} 
    else if (key == '+' || key == '=' ){
		volume += 0.05;
		volume = MIN(volume, 1);
	}
	
    /* // TODO: clean up soundstream stuff
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
    // */
    
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
void Partisynth::mouseMoved(int x, int y ){
    
    updateProperties(x, y);
    
}

void Partisynth::updateEmitterPosition(int x, int y ) {

    updateProperties(x, y);
    
}

//--------------------------------------------------------------
void Partisynth::mouseDragged(int x, int y, int button){

    updateProperties(x, y);

}

//--------------------------------------------------------------
void Partisynth::mousePressed(int x, int y, int button){
	bNoise = true;
}

//--------------------------------------------------------------
void Partisynth::mouseReleased(int x, int y, int button){
	bNoise = false;
}

//*/