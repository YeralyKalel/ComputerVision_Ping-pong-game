#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <math.h>


#define WIDTH 800
#define HEIGHT 600

class Vector2 {
public:
    float x;
    float y;
    Vector2(float x = 0, float y = 0) {
        this->x = x;
        this->y = y;
    }

    bool isZero() {
        return !x && !y;
    }

    float GetLength() {
        return sqrtf(pow(x, 2) + pow(y, 2));
    }

    void Normalize() {
        float length = GetLength();
        x /= length;
        y /= length;

    }

    Vector2 operator+(const Vector2& vector2) {
        Vector2 res(0,0);
        res.x = x + vector2.x;
        res.y = y + vector2.y;
        return res;
    }

    Vector2& operator+=(const Vector2& vector2) {
        x += vector2.x;
        y += vector2.y;
        return *this;
    }

};

bool isGameOver;
cv::Scalar sceneColor = cv::Scalar(255, 255, 255);
cv::Scalar wallColor = cv::Scalar(0, 0, 0);
const char* windowName;
cv::Mat image;

int wallWidth;


void drawEnvironment(int wallWidth) {
    cv::rectangle(image, cv::Point(0, 0), cv::Point(WIDTH, HEIGHT), sceneColor, CV_FILLED); //Fill everything 

    cv::rectangle(image, cv::Point(0, 0), cv::Point(wallWidth, HEIGHT), wallColor, CV_FILLED); //left wall
    cv::rectangle(image, cv::Point(0, 0), cv::Point(WIDTH, wallWidth), wallColor, CV_FILLED); //top wall
    cv::rectangle(image, cv::Point(WIDTH, 0), cv::Point(WIDTH - wallWidth, HEIGHT), wallColor, CV_FILLED); //right wall
}

class Box {
public:
    Vector2 edges[2] = { Vector2 (0,0), Vector2 (10,10) };

    void Instantiate(Vector2 e1, Vector2 e2) {
        edges[0] = e1;
        edges[1] = e2;
    }

};

class MoveableObject: public Box {
public:

    void Instantiate(Vector2 e1, Vector2 e2, int speed, cv::Scalar color = cv::Scalar(0, 0, 0)) {
        _speed = speed;
        _color = color;
        Box::Instantiate(e1, e2);
        UpdatePositionBy();
    }

    void SetPositionX(float centerX) {
        Erase();

        float dHalf = (MoveableObject::edges[1].x - MoveableObject::edges[0].x) / 2;
        MoveableObject::edges[0].x = centerX - dHalf;
        MoveableObject::edges[1].x = centerX + dHalf;
        Draw();
    }

    void UpdatePositionBy(Vector2 delta = Vector2(0,0)) {
        if (delta.isZero()) {
            Draw();
        }
        else {
            Erase();

            Box::edges[0] += delta;
            Box::edges[1] += delta;
            Draw();
        }
    }
    
    int speed() {
        return _speed;
    }

private:
    int _speed;
    cv::Scalar _color;

    void Erase() {
        cv::rectangle(image, cv::Point(Box::edges[0].x, Box::edges[0].y), cv::Point(Box::edges[1].x, Box::edges[1].y), sceneColor, CV_FILLED);
    }

    void Draw() {
        cv::rectangle(image, cv::Point(Box::edges[0].x, Box::edges[0].y), cv::Point(Box::edges[1].x, Box::edges[1].y), _color, CV_FILLED);
    }

};

class Player : MoveableObject {
public:

    void Instantiate(Vector2 e1, Vector2 e2, int speed, cv::Scalar color = cv::Scalar(0, 0, 0)) {
        MoveableObject::Instantiate(e1, e2, speed, color);
    }

    void MoveMouse(int x) {
        float dHalf = (MoveableObject::edges[1].x - MoveableObject::edges[0].x)/2;
        
        if (x - dHalf <= wallWidth) {                        //left wall
            MoveableObject::SetPositionX(wallWidth + dHalf + 1);
        } 
        else if (x + dHalf >= WIDTH - wallWidth) {           //right wall
            MoveableObject::SetPositionX(WIDTH - wallWidth - dHalf - 1);
        }
        else {
            MoveableObject::SetPositionX(x);
        }

        
    }

    void MoveDirection(int direction) {
        Vector2 delta(direction * speed(), 0);
        if ((delta + edges[0]).x <= wallWidth) {  //left wall
            delta.x = wallWidth - edges[0].x + 1;
        }
        else if ((delta + edges[1]).x >= WIDTH - wallWidth) { //right wall
            delta.x = WIDTH - wallWidth - edges[1].x - 1;
        }
        UpdatePositionBy(delta);
    }

    Vector2* GetEdges() {
        return edges;
    }
};

Player player;

class Ball : MoveableObject {
public:
    Vector2 direction = Vector2(-0.6f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (0.6f - -0.6f))) , -1);

    Ball(Vector2 e1, Vector2 e2, int speed, cv::Scalar color = cv::Scalar(0, 0, 0)) {
        MoveableObject::Instantiate(e1, e2, speed, color);
    }

    void Move() {
        direction.Normalize();
        Vector2 delta(direction.x * speed(), direction.y * speed());

        Vector2 nextPosition[2];
        nextPosition[0] = delta + edges[0];
        nextPosition[1] = delta + edges[1];

        //Check collision with walls
        //top wall:
        if (nextPosition[1].y <= wallWidth) {
            delta.y = wallWidth - edges[1].y + 1;
            direction.y = -direction.y;
        }
        //left wall:
        if (nextPosition[0].x <= wallWidth) {
            delta.x = wallWidth - edges[0].x + 1;
            direction.x = -direction.x;
        }
        //right wall:
        if (nextPosition[1].x >= WIDTH - wallWidth) {
            delta.x = WIDTH - wallWidth - edges[1].x - 1;
            direction.x = -direction.x;
        }
        
        Vector2* pE = player.GetEdges();

        //Check collision with player

        if (nextPosition[0].y >= (pE + 1)->y)
        {
            float tanA = direction.x / direction.y;
            float nextX = ((pE + 1)->y - edges[0].y) * tanA + edges[0].x;
            //std::cout << nextX << ", edges: " << (pE + 0)->x << " and " << (pE + 1)->x << std::endl;

            if (nextX > (pE + 0)->x && nextX < (pE + 1)->x) {
                delta.y = (pE + 1)->y - edges[0].y - 1;
                direction = Vector2(-0.6f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (0.6f - -0.6f))), -1);
            }
            else {
                isGameOver = true;
            }
        }

        UpdatePositionBy(delta);
    }
};

void MouseCallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    if (event == cv::EVENT_MOUSEMOVE)
    {
        player.MoveMouse(x);
    }

}

int main(int argc, char** argv)
{
    image = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC3);
    windowName = "Game Window";

    wallWidth = 30;

    int playerHeight = 25;
    int playerWidth = 120;
    int playerSpeed = 20;

    int ballSize = 20;
    int ballSpeed = 30;

    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(windowName, MouseCallBackFunc, NULL);

    isGameOver = false;
    drawEnvironment(wallWidth);
    player.Instantiate(Vector2(WIDTH / 2 - playerWidth / 2, HEIGHT), Vector2(WIDTH / 2 + playerWidth / 2, HEIGHT - playerHeight), playerSpeed);

    Ball ball(Vector2(WIDTH / 2 - ballSize / 2, HEIGHT - playerHeight - 1), Vector2(WIDTH / 2 + ballSize / 2, HEIGHT - playerHeight - ballSize - 1), ballSpeed);

    int key;
    while (true) {
        key = cv::waitKey(100);
        if (key == 27) {
            break;
        }
        else if (isGameOver) {
            break;
        }
        else {
            switch (key) {
            case 'a':
                player.MoveDirection(-1);
                //std::cout << "Key is pressed" << std::endl;
                break;
            case 'd':
                player.MoveDirection(1);
                //std::cout << "Key is pressed" << std::endl;
                break;
                //default:
                    //std::cout << "Not pressed" << std::endl;
            }

            ball.Move();

            cv::imshow(windowName, image);
        }
        
    }


    return 0;
}
