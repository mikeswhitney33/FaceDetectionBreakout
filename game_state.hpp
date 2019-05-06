#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <opencv2/opencv.hpp>

typedef cv::Vec<float, 2> vec2;

const int bricks_width = 20;
const cv::Scalar colors[5] {
	cv::Scalar(0, 0, 255),
	cv::Scalar(255, 0, 0),
	cv::Scalar(0, 255, 0),
	cv::Scalar(255, 255, 0),
	cv::Scalar(0, 255, 255)
};


cv::Point vec2Point(vec2 v){
	return cv::Point(v[0], v[1]);
}


bool RectRectIntersect(cv::Rect r1, cv::Rect r2) {
	return !(r1.x + r1.width < r2.x ||
		r2.x + r2.width < r1.x ||
		r1.y + r1.height < r2.y ||
		r2.y + r2.height < r1.y);
	return true;
}


class Paddle{
public:
	Paddle():width(60), height(10), color(cv::Scalar(0, 255, 0)){}

	void draw(cv::Mat &frame) {
		cv::rectangle(frame, getRect(), color, cv::FILLED);
	}

	cv::Rect getRect() {
		return cv::Rect(pos[0] - width/2, pos[1] + height/2, width, height);
	}

	vec2 pos;
	float width;
	float height;
	cv::Scalar color;
	vec2 lastpos;
};


class Brick {
public:
	Brick(cv::Rect b, int l):box(b), lives(l) {
	}

	void draw(cv::Mat &frame) {
		cv::rectangle(frame, box, colors[lives], cv::FILLED);
	}

	int lives;
	cv::Rect box;

};


class Ball {
public:
	Ball():radius(10), color(cv::Scalar(0, 0, 255)){}
	Ball(float r, cv::Scalar c = cv::Scalar(0, 0, 255)):radius(r),color(c){}

	void draw(cv::Mat &frame) {
		cv::circle(frame, vec2Point(pos), radius, color, cv::FILLED);
	}

	cv::Rect getRect() {
		return cv::Rect(pos[0] - radius, pos[1] - radius, radius*2, radius*2);
	}

	vec2 pos;
	int radius;
private:
	cv::Scalar color;
};

struct GameObjects {
	Ball ball;
	Paddle paddle;
	std::vector<Brick> bricks;
};


class GameState {
public:
	GameState() {

	}
	GameState(GameState* state): gameObjects(state->gameObjects) {}

	void setBricks(const cv::Mat &frame) {
		int num_bricks_per_row = 10;
		int width = frame.cols / num_bricks_per_row;
		int height= 20;
		for(int y = 0;y < 5;y++) {
			for(int x = 0; x < num_bricks_per_row; x++) {
				gameObjects.bricks.push_back(Brick(cv::Rect(x * width, y * height, width, height), 4 - y));
			}
		}
	}

	virtual bool isPlaying() {
		return false;
	}

	virtual bool update(float dt, cv::Mat &frame, cv::Rect* face) {
		gameObjects.paddle.lastpos = gameObjects.paddle.pos;
		if (face) {
			gameObjects.paddle.pos = vec2(face->x + (face->width / 2), frame.rows - 30);
		}

		gameObjects.paddle.draw(frame);

		for(int i = 0;i < gameObjects.bricks.size();i++) {
			if(gameObjects.bricks[i].lives >= 0) {
				gameObjects.bricks[i].draw(frame);
			}
		}

		return false;
	}
	GameObjects gameObjects;
};


class StartGameState: public GameState {
public:
	StartGameState():GameState(){}
	StartGameState(GameState* state):GameState(state){}

	virtual bool update(float dt, cv::Mat &frame, cv::Rect* face) {
		GameState::update(dt, frame, face);
		if (face) {
			gameObjects.ball.pos = vec2(gameObjects.paddle.pos[0], gameObjects.paddle.pos[1] - gameObjects.paddle.height / 2 - 1);
		}

		gameObjects.ball.draw(frame);

		return false;
	}
};


class PlayGameState: public GameState {
public:
	PlayGameState(GameState* state):GameState(state), dir(0, -1){
		vec2 vel = gameObjects.paddle.pos - gameObjects.paddle.lastpos;
		float vel_norm = (abs(vel[0] + abs(vel[1])));
		if(vel_norm > 1) {
			dir = dir + vel / vel_norm;
		}
	}

	virtual bool isPlaying() {
		return true;
	}

	virtual bool update(float dt, cv::Mat &frame, cv::Rect* face) {
		GameState::update(dt, frame, face);

		vec2 next = gameObjects.ball.pos + 10 * dir;
		float radius = gameObjects.ball.radius;

		vec2 normal;
		bool hit = false;
		if(next[0] - radius < 0) {
			normal = vec2(1, 0);
			hit = true;
		}
		if(next[0] + radius >= frame.cols) {
			normal = vec2(-1, 0);
			hit = true;
		}
		if(next[1] - radius < 0) {
			normal = vec2(0, 1);
			hit = true;
		}
		if(next[1] + radius >= frame.rows) {
			return true;
		}
		cv::Rect nextRec(next[0] - radius, next[1] - radius, radius*2, radius*2);
		if(RectRectIntersect(nextRec, gameObjects.paddle.getRect())) {
			normal = vec2(0, -1);
			hit = false;
			dir = dir - 2 * (dir.dot(normal) * normal);


			vec2 vel = gameObjects.paddle.pos - gameObjects.paddle.lastpos;

			float vel_norm = (abs(vel[0] + abs(vel[1])));
			if(vel_norm > 1) {
				dir = dir - vel / vel_norm;
			}
		}
		else {
			for(int i = 0;i < gameObjects.bricks.size();i++) {
				if(gameObjects.bricks[i].lives >= 0 && RectRectIntersect(nextRec, gameObjects.bricks[i].box)){
					gameObjects.bricks[i].lives--;
					if(gameObjects.ball.pos[0] < gameObjects.bricks[i].box.x) {
						normal = vec2(-1, 0);
					}
					if(gameObjects.ball.pos[0] > gameObjects.bricks[i].box.x + gameObjects.bricks[i].box.width) {
						normal = vec2(1, 0);
					}
					if(gameObjects.ball.pos[1] > gameObjects.bricks[i].box.y + gameObjects.bricks[i].box.height) {
						normal = vec2(0, 1);
					}
					if(gameObjects.ball.pos[1] < gameObjects.bricks[i].box.y) {
						normal = vec2(0, -1);
					}
					hit = true;
					break;
				}
			}
		}

		if(hit) {
			dir = dir - 2 * (dir.dot(normal) * normal);
		}

		gameObjects.ball.pos += 10 * dir;
		gameObjects.ball.draw(frame);
		return false;
	}

private:
	vec2 dir;
};



#endif
