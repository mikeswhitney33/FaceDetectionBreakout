#include <opencv2/opencv.hpp>
#include <iostream>
#include <time.h>

#include "game_state.hpp"


cv::CascadeClassifier classifier;
GameState* state;
time_t last_time;


int show_webcam(void (*process_frame)(float, cv::Mat, cv::Mat &, bool));
void mark_faces(float, cv::Mat, cv::Mat &, bool);


int main(int argc, char** argv) {
	srand(time(NULL));


	state = new StartGameState();
	classifier.load("lbpcascade_frontalface_improved.xml");
	return show_webcam(mark_faces);
}


void mark_faces(float dt, cv::Mat src, cv::Mat &dst, bool detect) {
	cv::Mat frame_gray;
	cv::cvtColor(src, frame_gray, cv::COLOR_BGR2GRAY);
	cv::equalizeHist(frame_gray, frame_gray);

	static std::vector<cv::Rect> faces;
	if (detect)
		classifier.detectMultiScale(frame_gray, faces);

	if(state->update(dt, dst, (faces.size() == 0) ? NULL : &faces[0])) {
		state = new StartGameState(state);
	}
}


int show_webcam(void (*process_frame)(float, cv::Mat, cv::Mat &, bool)) {
	cv::Mat frame;
	cv::VideoCapture cap(0);
	// cv::VideoWriter writer("out/demo.mov", cv::VideoWriter::fourcc('m','p','4','v'), 15.0, cv::Size(640, 360));

	if(!cap.isOpened()) {
		std::cerr << "ERROR! unable to open camera" << std::endl;
		return -1;
	}
	int detect = 0;

	last_time = time(NULL);
	bool first_frame = true;

	for(;;) {
		time_t cur_time = time(NULL);
		float dt = difftime(cur_time, last_time);
		last_time = cur_time;

		cap.read(frame);
		if(frame.empty()) {
			std::cerr << "ERORR! blank frame grabbed" << std::endl;
			break;
		}
		cv::resize(frame, frame, cv::Size(), 0.5, 0.5);
		cv::flip(frame, frame, 1);

		if(first_frame) {
			state->setBricks(frame);
			first_frame = false;
		}

		if(process_frame) {
			process_frame(dt, frame, frame, detect % 2 == 0);
		}

		detect = (detect + 1) % 2;

		cv::imshow("Face Detection Breakout", frame);
		// writer.write(frame);

		char c = (char) cv::waitKey(1);
		if(c == 'q' || c == 'Q' || c == 27) {
			break;
		}
		else if(c == 'p' && !state->isPlaying()) {
			state = new PlayGameState(state);
		}
	}
	// writer.release();
	cap.release();
	return 0;
}
