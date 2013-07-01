#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"

class testApp : public ofBaseApp{

private:
    ofSerial		serial;

public:

    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    void sendData();

    void audioIn(float * input, int bufferSize, int nChannels);
    void audioOut(float * output, int bufferSize, int nChannels);

    vector <float> volHistory;
    string msgTx, msgRx;

    int 	bufferCounter;
    int 	drawCounter;

    bool weConnected;

    float smoothedVol;
    float scaledVol;

    int recBufferCounter;
    int recBufferSize;
    unsigned char recBuffer[1024];
    float inputTemp[1024];
    unsigned char recTemp[1024];


    int playBufferCounter;
    int playBufferSize;
    unsigned char playBuffer[1024];
    unsigned char playTemp[1024];

    bool speaking;
    bool ready;

    float deltaTime;

    ofSoundStream soundStream;
    ofxUDPManager udpConnection;
};
