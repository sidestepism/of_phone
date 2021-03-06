#include "testApp.h"
#include "ofxNetwork.h"
#include <string.h>

#include "ofUtils.h"

//-------------------------------------------------------------
void testApp::setup(){
	serial.enumerateDevices();
	serial.setup("/dev/tty.usbmodem1431", 115200);

	ofSetVerticalSync(true);
	ofSetCircleResolution(80);
	ofBackground(54, 54, 54);

	// 0 output channels,
	// 2 input channels
	// 44100 samples per second
	// 256 samples per buffer
	// 4 num buffers (latency)

	soundStream.listDevices();

	//if you want to set a different device id
	//soundStream.setDeviceID(0); //bear in mind the device id corresponds to all audio devices, including  input-only and output-only devices.

	volHistory.assign(400, 0.0);

	bufferCounter	= 0;
	drawCounter		= 0;
	smoothedVol     = 0.0;
	scaledVol		= 0.0;

    // setup buffer
    recBufferCounter = 0;
    recBufferSize = 1024;
    playBufferCounter = 0;
    playBufferSize = 16384;

    memset(recBuffer, 0, 1024);
    memset(playBuffer, 0, 16384);

    ready = false;

	soundStream.setup(this, 2, 1, 48000, 128, 4);

//    ipaddr = "10.100.0.225";
//    port = 9750;
    ipaddr = "157.82.7.141";
    port = 9750;


//    ipaddr = (char *)ofSystemTextBoxDialog("Destination IP Addr", "10.100.0.225").c_str();
//    istringstream(ofSystemTextBoxDialog("Destination Port", "9750")) >> port;
    cout << "ipaddr " << ipaddr << ", port: " << port << endl;

    speaking = false;
    weConnected = false;
}


//--------------------------------------------------------------
void testApp::update(){
	//lets scale the vol up to a 0-1 range
	scaledVol = ofMap(smoothedVol, 0.0, 0.17, 0.0, 1.0, true);

	//lets record the volume into an array
	volHistory.push_back( scaledVol );

	//if we are bigger the the size we want to record - lets drop the oldest value
	if( volHistory.size() >= 400 ){
		volHistory.erase(volHistory.begin(), volHistory.begin()+1);
	}
}

//--------------------------------------------------------------
void testApp::draw(){
    /**
     * 定期的に実行する
     */
	ofSetColor(225);
	ofDrawBitmapString("XBEE Phone", 32, 32);
	ofDrawBitmapString("by Kuumeri Akaki & Ryohei Fushimi", 31, 92);
    if(speaking){
        ofDrawBitmapString("connected.", 31, 72);
    }else{
        ofDrawBitmapString("waiting clients..", 31, 72);
    }

	ofNoFill();

	// draw the left channel:
	ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 170, 0);

    ofSetColor(225);
    ofDrawBitmapString("Input Channel", 4, 18);

    ofSetLineWidth(1);
    ofRect(0, 0, 512, 200);

    ofSetColor(245, 58, 135);
    ofSetLineWidth(3);

    ofBeginShape();
    for (int i = 0; i < 128; i++){
        ofVertex(i*4, 100-inputTemp[i]*100.0f);
    }
    ofEndShape(false);

    ofPopMatrix();
	ofPopStyle();

	// draw the left channel:
	ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 170 + 200, 0);

    ofSetColor(225);
    ofDrawBitmapString("Output Channel", 4, 18);

    ofSetLineWidth(1);
    ofRect(0, 0, 512, 200);

    ofSetColor(245, 58, 135);
    ofSetLineWidth(3);

    ofBeginShape();
    for (int i = 0; i < playBufferSize; i++){
        ofVertex(i * 4, 100-(((float)(playBuffer[i]) - 128) / 256)*180.0f);
    }
    ofEndShape(false);

    ofPopMatrix();
	ofPopStyle();

	// draw the average volume:
	ofPushStyle();
    ofPushMatrix();
    ofTranslate(565, 170, 0);

    ofSetColor(225);
    ofDrawBitmapString("Scaled average vol (0-100): " + ofToString(scaledVol * 100.0, 0), 4, 18);
    ofRect(0, 0, 400, 400);

    ofSetColor(245, 58, 135);
    ofFill();
    ofCircle(200, 200, scaledVol * 190.0f);

    //lets draw the volume history as a graph
    ofBeginShape();
    for (int i = 0; i < volHistory.size(); i++){
        if( i == 0 ) ofVertex(i, 400);
        ofVertex(i, 400 - volHistory[i] * 70);
        if( i == volHistory.size() -1 ) ofVertex(i, 400);
    }
    ofEndShape(false);

    ofPopMatrix();
	ofPopStyle();

	drawCounter++;

	ofSetColor(225);
	string reportString = "buffers received: "+ofToString(bufferCounter)+"\ndraw routines called: "+ofToString(drawCounter)+"\nticks: " + ofToString(soundStream.getTickCount());
	ofDrawBitmapString(reportString, 32, 589);
    ofDrawBitmapString("rec buffer: " + ofToString(recBufferCounter) + " / " + ofToString(recBufferSize) , 32, 589 + 16 + 32);
    ofDrawBitmapString("play buffer: " + ofToString(playBufferCounter) + " / " + ofToString(playBufferSize) , 32, 589 + 32 + 32);
}

//--------------------------------------------------------------
void testApp::audioIn(float * input, int bufferSize, int nChannels){
	float curVol = 0.0;
	int numCounted = 0;

	for (int i = 0; i < bufferSize; i++){
		inputTemp[i]		= input[i] * 0.5;
		curVol += inputTemp[i] * inputTemp[i];
		numCounted += 1;
	}

	// this is how we get the mean of rms
	curVol /= (float)numCounted;

	// this is how we get the root of rms
	curVol = sqrt( curVol );

    // down sampling from 48000hz -> 8000hz
    // down sampling from 48000hz -> 9600hz (5)
    for (int i = 0; i < bufferSize; i+= 5){
        if(recBufferCounter >= recBufferSize){
            if (speaking) {
                this->sendData();
            }
            break;
        }
        int raw = (128 + 128 * 4 * inputTemp[i]);
        if(raw > 255)raw = 255;
        else if(raw < 0)raw = 0;
//        cout << "raw: " << raw << ", input: " << inputTemp[i] << endl;
        recBuffer[recBufferCounter++] = (unsigned int)raw;
        
//      cout << input[i] << endl;
    }
	smoothedVol *= 0.93;
	smoothedVol += 0.07 * curVol;
	bufferCounter++;

    /**
     reading from serial
     */

    if(ready){
        char readBuffer[playBufferSize];
        int bytes = recvOnTCP((char *)readBuffer, playBufferSize);

        if(bytes > 16384){

        }else if(bytes == 0){


        }
//        cout << "received, " << bytes << endl;
        if(playBufferCounter < playBufferSize){
            memcpy(&playBuffer[playBufferCounter], readBuffer, playBufferSize - playBufferCounter);
            playBufferCounter += bytes;
            if(playBufferCounter >= playBufferSize){
                playBufferCounter = playBufferSize;
                                cout << "Play buffer is full" << endl;
            }
        }
    }
}

void testApp::sendOnTCP(char* data, int size, bool testWave){

//    cout << "data sending " << data << endl;
//    for(int i = 0; i < size; i++){
//        cout << i << ": " << (char)data[i] << endl;
//    }

        if(udpConnection.Send(data, size)){

        }else{
            cout << "sending failed" << endl;
        }
}

int testApp::recvOnTCP(char* receiveBytes, int numBytes){
    int bytes = udpReceiver.Receive(receiveBytes, numBytes);
    if( bytes > 0 ){
//        cout << "data received (" << bytes << ") " << receiveBytes << endl;
//            for(int i = 0; i < bytes; i++){
//                cout << i << ": " << (unsigned short)receiveBytes[i] << endl;
//            }
            return bytes;
        }else{
//            cout << "got 0 bytes" << endl;
			weConnected = false;
            return 0;
		}
}

void testApp::sendData(){
//    cout << "Rec buffer dump:" << recBufferSize << endl;
//
//    for (int i = 0; i < recBufferSize; i++){
//          recBuffer[i] = i % 2 ? 128 - 8 : 128 + 8;
//          cout << (int)recBuffer[i] << endl;
//    }

//    if(speaking){
        sendOnTCP((char *)recBuffer, recBufferCounter, true);
//    }

//    serial.writeBytes(recBuffer, recBufferSize);

    // clear rec buffer
    recBufferCounter = 0;
    memset(recBuffer, 0, recBufferSize);
}

//--------------------------------------------------------------
void testApp::audioOut(float * output, int bufferSize, int nChannels){
    assert(playBufferCounter >= 0);

    float frameRate = 6;
    if(playBufferCounter < 128){
        return;
    }else if(playBufferCounter < 256){
        frameRate = 5.8;
    }else if(playBufferCounter < 512){
        frameRate = 6.4;
    }else if(playBufferCounter < 2048){
        frameRate = 6;
    }else{
        frameRate = 5.5;
    }

    // up sampling from 8000hz -> 48000hz
    int frame = 0;
    for (int i = 0; i < bufferSize; i++){
        frame = floor(i / frameRate);

        float sample = (((float)(playBuffer[frame]) - 128) / 256) * 0.5;
        if(playBuffer[frame] == 0){
            sample = 0;
        }

        output[i*2    ] = sample;
        output[i*2 + 1] = sample;
    }

    // playbuf    0 1 2 3 4 5 6 (playBufferCounter: 7, frame = 3)

    playBufferCounter -= (frame + 1);
    if(playBufferCounter < 0) playBufferCounter = 0;

    // playbuf    0 1 2 3 4 5 6 (playBufferCounter: 3, frame = 3)

    assert(playBufferCounter >= 0);
    assert(playBufferCounter < playBufferSize);

    memset(playTemp, 0, playBufferSize);

    // playbuf    0 1 2 3 4 5 6 (playBufferCounter: 3, frame = 3)
    // playtemp   - - - - - - - (playBufferCounter: 3, frame = 3)

    memcpy(playTemp, &playBuffer[frame + 1], playBufferSize - (frame + 1));

    // playbuf    0 1 2 3 4 5 6 (playBufferCounter: 3, frame = 3)
    // playtemp   4 5 6 - - - - (playBufferCounter: 3, frame = 3)

    memcpy(playBuffer, playTemp, playBufferSize);

    // playbuf    4 5 6 - - - - (playBufferCounter: 3, frame = 3)
    // playtemp   4 5 6 - - - - (playBufferCounter: 3, frame = 3)
    // ok!

//    cout << "playBuffer";
//    for(int i = 0; i < playBufferCounter; i++){
//        cout << (unsigned short)playBuffer[i] << " ";
//    }
//
//    cout << endl;

}


//--------------------------------------------------------------
void testApp::keyPressed(int key){
    string callMsg;
    if( key == 'e' ){
		soundStream.stop();
        speaking = false;
	}
	if( key == 's' ){
		soundStream.start();
        speaking = true;
	}

    if( key == 'a' ){
        udpConnection.Create();
        udpConnection.Connect(ipaddr, port);
        udpConnection.SetNonBlocking(true);

        //create the socket and bind to port 11999
        udpReceiver.Create();
        udpReceiver.Bind(port);
        udpReceiver.SetNonBlocking(true);

        cout << "setup" << endl;
        ready = true;
    }

	if( key == 'c' ){
        char callMsg[36] = "00000000calHello world!-----------\n";
        sendOnTCP(callMsg, 36, false);
		soundStream.start();
        speaking = true;
	}
    if( key == '1' ){
        char callMsg[36] = "00000000smsHello world!-----------\n";
        sendOnTCP(callMsg, 36, false);
	}	if( key == 'h' ){
        char callMsg[36] = "00000000end-----------------------\n";
        sendOnTCP(callMsg, 36, false);
        speaking = false;
		soundStream.stop();
        tcpClient.close();
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

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}