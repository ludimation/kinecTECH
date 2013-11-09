// /*
 
#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"
#include "ofxParticleEmitter.h"

class Partisynth {
public:
    
    Partisynth();
    ~Partisynth();
    
    void    init();
    void    exit();
    void    init(float multiplier);
    void    update();
    void    draw();
    
    void    mouseMoved(int x, int y );
    void    mouseDragged(int x, int y, int button);
    void    mousePressed(int x, int y, int button);
    void    mouseReleased(int x, int y, int button);
    
    void    updateEmitterPosition(int x, int y );
    // TODO: move debug & pause functionality into GUI    
    void    keyPressed (int key);
    char    screenID;
    // this function might be a tricky one
    // can I just call "oudioout()" with proper arguments from the testApp object?
    void    audioOut(float * input, int bufferSize, int nChannels);
    
    float   sizeAdjustment;
    float 	pan;
    float 	volume;
    bool    paused;
    // these need to be re-thought
    // perhaps could be set as min, max, current? 
    // Or simply hand this function a pct from 0.0 - 1.0
    float   height;
    float   heightPct; 
    // heightPct > pitch? 
    // seems like a place for improvements in terms of asking for:
    // - specific pitches relative to octaves
    // - specific tones
    // - could have a mode that snaps input to tones, semitones, and even particulat scales
    
protected: //how is this different from "private:"?
    void    updateEmitters();
    void    updateProperties(int x, int y);
    void    updateProperties();
	
    //------------------- for the wave synthesis
    void    setPhaseAdderTarget();

    // ofSoundStream soundStream; // TODO: clean up soundstream stuff
    float 	targetFrequency;
    float 	phase;
    float 	phaseAdder;
    float 	phaseAdderTarget;
    
    bool    updateParticleTexture;
    string  xmlFilename;
    string  texFilename;
    
    char    waveform;
    float   phaseAdderTargetTween;
    float   volumeWaveformAdjustment;
    float   volumeFrequencyAdjustment;
    float   volumeAdjustment;

    int		sampleRate;
    bool 	bNoise;    
    vector <float> lAudio;
    vector <float> rAudio;
    
    int     numEmitters;
    vector <ofxParticleEmitter> emitters;
    
};

//*/