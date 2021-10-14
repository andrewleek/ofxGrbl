#include "ofxGrbl.h"

//--------------------------------------------------------------
void ofxGrbl::setup() {
	cout << "[ ofxGrbl ] setup()" << endl;
	readBuffer = "";
	status = "";

	// param
	isReadyToSend = true;
	isPause = false;

	// serial
	//serial.listDevices();
	isConnect = false;
	isDeviceReady = false;
	port = "COM3";
	baudrate = 115200;

	_settings.MaxSpeed = ofVec3f(10000,10000,10000);
	_settings.Accel = ofVec3f(100, 100, 100);
	_settings.FeedbackInterval = 10;
	_settings.SpindleSpeed = 100;
}

//--------------------------------------------------------------
void ofxGrbl::update() {

	//int waitCount = 10000;
	//while (!serial.available() && waitCount > 0) {
	//	cout << "[ waiting ]" << waitCount << endl;
	//	waitCount--;
	//}
	//if (waitCount == 0) {
	//	isConnect = false;
	//	cout << "[ timeout ]" << _port << " is busy." << endl;
	//}

	if (isConnect) {
		while (serial.available() > 0) {
			if (!isDeviceReady) {
				isDeviceReady = true;
				sendMessage("$$", true);
				setSettings();
			}

			char _byte = (char)serial.readByte();
			if (_byte == '\n' || _byte == '\r') {
				if (readBuffer != "") {
					cout << "[ ofxGrbl ] [ RECEIVE ] " << readBuffer << endl;
					if (readBuffer == "ok") {
						isReadyToSend = true;
						//sentCount--;
						//cout << "[ ofxGrbl ] Sent: " << sentCount << endl;
					}
					if (readBuffer == "error: Unsupported command") {
						cout << "[ ofxGrbl ] [ PAUSED ]" << endl;
						isPause = true;
						isReadyToSend = true;
					}
					if (readBuffer[0] == '<') {
						// parse grbl state message
						/*
						vector<string> _status = ofSplitString(readBuffer, ",");
						vector<string> _posx = ofSplitString(_status[1], ":");
						vector<string> _posz = ofSplitString(_status[3], ">");
						cout << "[ ofxGrbl ] [ POSITION ] " << _posx[1] << ", " << _status[2] << ", " << _posz[0] << endl;
						currentPos = ofVec2f(ofToFloat(_posx[1]) / (float)GRBL_WIDTH, ofToFloat(_status[2]) / (float)GRBL_HEIGHT);
						*/

						readBuffer = readBuffer.substr(1, readBuffer.length() - 2);
						vector<string> _status = ofSplitString(readBuffer, "|");
						status = _status[0];
						vector<string> _pos_str = ofSplitString(_status[1], ":");
						vector<string> _pos = ofSplitString(_pos_str[1], ",");
						cout << "[ ofxGrbl ] [ POSITION ] " << _pos[0] << ", " << _pos[1] << ", " << _pos[2] << endl;
						currentPos = ofVec3f(ofToFloat(_pos[0]) / (float)GRBL_WIDTH, ofToFloat(_pos[1]) / (float)GRBL_HEIGHT);
						// Events
						ofNotifyEvent(PositionEvent, currentPos);
					}
					readBuffer = "";
				}
			}
			else {
				readBuffer = readBuffer + _byte;
			}
		}
	}

	if (isConnect && isDeviceReady) {

		// send
		if (isReadyToSend && !isPause) {
			if (sendQueList.size() > 0) {
				sendMessage(sendQueList[0], true);

				sendQueList.erase(sendQueList.begin());
				cout << "[ ofxGrbl ] [ QUE ] " << sendQueList.size() << endl;

				isReadyToSend = false;
			}
		}

		// get status
		if (ofGetFrameNum() % (int)_settings.FeedbackInterval == 0) sendMessage("?", true);
	}
}

//--------------------------------------------------------------
void ofxGrbl::close() {
	if (isConnect) {
		if (bSpindle) setSpindle(false, true);
		sendMessage("G90 G0 X0 Y0 Z0", true);
	}
}

//--------------------------------------------------------------
void ofxGrbl::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofxGrbl::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofxGrbl::mouseMoved(int x, int y) {
	
}

//--------------------------------------------------------------
void ofxGrbl::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofxGrbl::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofxGrbl::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofxGrbl::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofxGrbl::dragEvent(ofDragInfo dragInfo) {
	for (int i = 0; i < dragInfo.files.size(); i++) {
		string _ext = ofFile(dragInfo.files[i]).getExtension();
		if (_ext == "gcode" || _ext == "nc" || _ext == "ngc") {
			loadFromFile(ofToDataPath(dragInfo.files[i]));
		}
		else {
			cout << "[ ofxGrbl ] Invalid extension. Please use ( .gcode / .ngc / .nc ). " << endl;
		}
	}
}

//--------------------------------------------------------------
void ofxGrbl::loadFromFile(string _path) {
	cout << "[ ofxGrbl ] loadFromFile( " << _path << " )" << endl;
	string _text = string(ofBufferFromFile(_path));
	vector<string> _linelist = ofSplitString(_text, "\n", true);
	cout << "[ ofxGrbl ] loadFromFile() : " << _linelist.size() << " lines." << endl;

	vector<ofVec3f> _tmpVec;
	for (int i = 0; i < _linelist.size(); i++)
	{
		sendMessage(_linelist[i]);
	}
}

//--------------------------------------------------------------
void ofxGrbl::sendMessage(string _msg, bool direct) {
	if (direct) {
		if (isConnect) {
			if (_msg != "") {
				string _message = _msg + "\n";
				unsigned char* writeByte = (unsigned char*)_message.c_str();
				serial.writeBytes(writeByte, _message.length());
				//sentCount++;
				cout << "[ ofxGrbl ] sendMessage( " << _msg << " )" << endl;
			}
			else {
				cout << "[ ofxGrbl ] sendMessage() : Message is empty." << endl;
			}
		}
		else {
			cout << "[ ofxGrbl ] sendMessage() : Serial is not connected." << endl;
		}
	}
	else {
		if (_msg != "") {
			string _message = _msg;
			sendQueList.push_back(_message);
		}
	}
}

bool ofxGrbl::checkMoveCommand(string _line) {
	return (_line.find('X') != string::npos) || (_line.find('Y') != string::npos) || (_line.find('Z') != string::npos);
}

ofVec3f ofxGrbl::gcodeToVec3f(string _line) {
	ofVec3f _result = ofVec3f::zero();
	vector<string> _commands = ofSplitString(_line, " ", true);
	for (int i = 0; i < _commands.size(); i++) {
		if (_commands[i][0] == 'X') {
			if (_commands[i].size() == 1) {
				// space parse
				_result.x = ofToFloat(_commands[i + 1]) / GRBL_WIDTH; // use next character
			}
			else {
				// no space parse
				_result.x = ofToFloat(_commands[i].substr(1)) / GRBL_WIDTH;
			}
		}
		else if (_commands[i][0] == 'Y') {
			if (_commands[i].size() == 1) {
				// space parse
				_result.y = ofToFloat(_commands[i + 1]) / GRBL_HEIGHT; // use next character
			}
			else {
				// no space parce
				_result.y = ofToFloat(_commands[i].substr(1)) / GRBL_HEIGHT;
			}
		}
		else if (_commands[i][0] == 'Z') {
			if (_commands[i].size() == 1) {
				// space parse
				_result.z = ofToFloat(_commands[i + 1]) / GRBL_DEPTH; // use next character
			}
			else {
				// no space parce
				_result.z = ofToFloat(_commands[i].substr(1)) / GRBL_DEPTH;
			}
		}
	}

	return _result;
}

string ofxGrbl::vec3fToGcode(ofVec3f _vec) {
	string _message;
	_message = "G90 G1 X" + ofToString(_vec.x * GRBL_WIDTH, 2) + " Y" + ofToString(_vec.y * GRBL_HEIGHT, 2);
	return _message;
}

void ofxGrbl::resetStrokes() {
	strokeList.clear();
	sendQueList.clear();
}

void ofxGrbl::home() {
	sendMessage("G90 G0 X" + ofToString(_settings.HomePosition.x) + " Y" + ofToString(_settings.HomePosition.y) + " Z" + ofToString(_settings.HomePosition.z), true);
}

void ofxGrbl::homing() {
	sendMessage("$H", true);
}

void ofxGrbl::killAlarmLock() {
	sendMessage("$X", true);
}

void ofxGrbl::connect(string _port, int _baudrate) {
	if (_port == "") _port = port;
	if (_baudrate <= 0) _baudrate = baudrate;

	cout << "[ ofxGrbl ] Connect( " << _port << ", " << _baudrate << " )" << endl;

	// reset serial
	if (isConnect || isDeviceReady) {
		serial.close();
		isConnect = false;
		isDeviceReady = false;
	}

	isConnect = serial.setup(_port, _baudrate);
	if (isConnect) {
		cout << "[ ofxGrbl ] Connected to " << _port << "@" << _baudrate << " !" << endl;
	} else {
		cout << _port << " is not exists." << endl;
	}
}

void ofxGrbl::setSettings() {
	// set mode
	if (_settings.Mode == "Laser") {
		sendMessage("$32=1");
	}
	else {
		sendMessage("$32=0");
	}
	// set spindle speed
	sendMessage("S" + ofToString((int)_settings.SpindleSpeed));

	// set max speed
	sendMessage("F" + ofToString(_settings.MaxSpeed.x, 4));
	sendMessage("$110=" + ofToString(_settings.MaxSpeed.x, 4));
	sendMessage("$111=" + ofToString(_settings.MaxSpeed.y, 4));
	sendMessage("$112=" + ofToString(_settings.MaxSpeed.z, 4));
	// set accesl
	sendMessage("$120=" + ofToString(_settings.Accel.x, 4));
	sendMessage("$121=" + ofToString(_settings.Accel.y, 4));
	sendMessage("$122=" + ofToString(_settings.Accel.z, 4));
	// set max travel
	sendMessage("$130=" + ofToString(_settings.MaxTravel.x, 4));
	sendMessage("$131=" + ofToString(_settings.MaxTravel.y, 4));
	sendMessage("$132=" + ofToString(_settings.MaxTravel.z, 4));

	setArea(_settings.MaxTravel.x, _settings.MaxTravel.y);
}

void ofxGrbl::setArea(float x, float y, float z) {
	cout << "[ ofxGrbl ] setArea(" << (int)x << ", " << (int)y << ", " << (int)z << ")" << endl;
	GRBL_WIDTH = x;
	GRBL_HEIGHT = y;
	GRBL_DEPTH = z;
}

void ofxGrbl::setHome(float x, float y, float z) {
	cout << "[ ofxGrbl ] setHome(" << (int)x << ", " << (int)y << ", " << (int)z << ")" << endl;
	_settings.HomePosition = ofVec3f(x, y, z);
}
void ofxGrbl::setHome(ofVec3f _homePos) {
	cout << "[ ofxGrbl ] setHome(" << _homePos.x << ", " << _homePos.y << ", " << _homePos.z << ")" << endl;
	_settings.HomePosition = ofVec3f(_homePos.x, _homePos.y, _homePos.z);
}

void ofxGrbl::setSpindle(bool _enable, bool _direct) {
	cout << "[ ofxGrbl ] setSpindle(" << _enable << ", " << _direct << ")" << endl;
	bSpindle = _enable;
	if (bSpindle) {
		sendMessage("M3", _direct);
	}
	else {
		sendMessage("M5", _direct);
	}
}

void ofxGrbl::setSpindleSpeed(int _speed, bool _direct) {
	cout << "[ ofxGrbl ] setSpindleSpeed(" << _speed << ", " << _direct << ")" << endl;
	_settings.SpindleSpeed = _speed;
	sendMessage("S" + ofToString((int)_settings.SpindleSpeed), _direct);
}

void ofxGrbl::setPosition(float _mmX, float _mmY, float _mmZ) {
	sendMessage("G90 G1 X" + ofToString(_mmX) + " Y" + ofToString(_mmY) + "  Z" + ofToString(_mmZ) + " ", false);
}
void ofxGrbl::setPosition(ofVec3f _pos) {
	setPosition(_pos.x, _pos.y, _pos.z);
}

void ofxGrbl::moveRight(float _mm) {
	sendMessage("G91 G1 X" + ofToString(_mm) + " Y0 Z0", true);
}

void ofxGrbl::moveLeft(float _mm) {
	sendMessage("G91 G1 X" + ofToString(-_mm) + " Y0 Z0", true);
}

void ofxGrbl::moveUp(float _mm) {
	sendMessage("G91 G1 X0 Y" + ofToString(_mm) + " Z0", true);
}

void ofxGrbl::moveDown(float _mm) {
	sendMessage("G91 G1 X0 Y" + ofToString(-_mm) + " Z0", true);
}
