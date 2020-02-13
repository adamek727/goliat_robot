//
// Created by Matous Hybl on 2018-10-02.
//

#include <iostream>
#include "../utils/i2c/I2C.h"
#include "../Configuration.h"
#include "../utils/utils.h"
#include <cmath>

#ifndef FIRMWARE_MOTORS_H
#define FIRMWARE_MOTORS_H

#define driverAddress 0x72
#define odometryAddress 0x01

#define registerValueRPS 262.144

struct __attribute__((packed, aligned(1))) Odometry {
    float ds = 0;
    float da = 0;
    float x = 0; // meters
    float y = 0;
    float a = 0; // radians
};

class Motors {

public:
    Motors(I2C *i2c, Configuration *configuration);
    // cannot use default arguments because default value is stored in configuration struct
    void forward();
    void backward();
    void rotateLeft();
    void rotateRight();
    void turnLeft();
    void turnRight();

    void forward(float linearSpeed, float angularSpeed = 0);

    void backward(float leftSpeed, float rightSpeed);
    void rotateLeft(float leftSpeed, float rightSpeed);
    void rotateRight(float leftSpeed, float rightSpeed);
    void turnLeft(float speed);
    void turnRight(float speed);
    void stop();

    void resetOdometry();

    Odometry readOdometry();

private:
    I2C *i2c;
    Configuration *configuration;
    int16_t targetLeftSpeed = 0;
    int16_t targetRightSpeed = 0;

    int16_t actualLeftSpeed = 0;
    int16_t actualRightSpeed = 0;

    Odometry odometry;

    std::mutex mutex;
    std::mutex odometryMutex;

    void update();
    void updateOdometry();
    void setSpeed(float leftLinear, float rightLinear, float angular = 0);
};


#endif //FIRMWARE_MOTORS_H
