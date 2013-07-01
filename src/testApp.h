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

    float smoothedVol;
    float scaledVol;

    int recBufferCounter;
    int recBufferSize;
    float inputTemp[1024];
    unsigned char recBuffer[1024];
    unsigned char recTemp[1024];


    int playBufferCounter;
    int playBufferSize;
    unsigned char playBuffer[16384];
    unsigned char playTemp[16384];

    bool speaking;
    bool ready;

    int connectTime;
    int deltaTime;

    ofSoundStream soundStream;
    ofxTCPServer TCPServer;
    ofxTCPClient tcpClient;
    ofxUDPManager udpConnection;
    ofxUDPManager udpReceiver;


    bool weConnected;

    char* ipaddr;
    int port;

    void sendOnTCP(char* data, int size, bool testWave);
    int recvOnTCP(char* receiveBytes, int numBytes);
};
