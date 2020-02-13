//
// Created by Matous Hybl on 2018-10-02.
//
#include "Motors.h"

Motors::Motors(I2C *i2c, Configuration *configuration) {
    this->i2c = i2c;
    this->configuration = configuration;

    std::thread([this, configuration]() {
                    while (true) {
                        this->update();
                        std::this_thread::sleep_for(
                                std::chrono::milliseconds(configuration->rampSampleTimeMs));
                    }
                }).detach();

    std::thread([this, configuration]() {
        while (true) {
            this->updateOdometry();
            std::this_thread::sleep_for(
                    std::chrono::milliseconds(configuration->odometrySampleTimeMs));
        }
    }).detach();
}

void Motors::forward() {
    forward(configuration->defaultSpeedInMps, 0);
}

void Motors::backward() {
    backward(configuration->defaultSpeedInMps, configuration->defaultSpeedInMps);
}

void Motors::rotateLeft() {
    rotateLeft(configuration->defaultSpeedInMps, configuration->defaultSpeedInMps);
}

void Motors::rotateRight() {
    rotateRight(configuration->defaultSpeedInMps, configuration->defaultSpeedInMps);
}

void Motors::turnLeft() {
    turnLeft(configuration->defaultSpeedInMps);
}

void Motors::turnRight() {
    turnRight(configuration->defaultSpeedInMps);
}

void Motors::backward(float leftSpeed, float rightSpeed) {
    setSpeed(-leftSpeed, -rightSpeed);
}

void Motors::rotateLeft(float leftSpeed, float rightSpeed) {
    setSpeed(-leftSpeed, rightSpeed);
}

void Motors::rotateRight(float leftSpeed, float rightSpeed) {
    setSpeed(leftSpeed, -rightSpeed);
}

void Motors::turnLeft(float speed) {
    setSpeed(0, speed);
}

void Motors::turnRight(float speed) {
    setSpeed(speed, 0);
}

void Motors::stop() {
    setSpeed(0, 0);
}

void Motors::setSpeed(float left, float right, float angular) {
    std::lock_guard<std::mutex> lock(mutex);
    targetLeftSpeed = static_cast<int16_t>((left + 0.5 * angular * configuration->axleWidth) / configuration->wheelCircumference() * registerValueRPS);
    targetRightSpeed = static_cast<int16_t>((right - 0.5 * angular * configuration->axleWidth) / configuration->wheelCircumference() * registerValueRPS);
}

void Motors::update() {
    std::lock_guard<std::mutex> lock(mutex);
    const int16_t rampStep = configuration->rampStep();
    if (targetRightSpeed == 0) {
        actualRightSpeed = 0;
    } else {
        actualRightSpeed = saturate(targetRightSpeed - actualRightSpeed, -rampStep, rampStep) + actualRightSpeed;
    }

    if (targetLeftSpeed == 0) {
        actualLeftSpeed = 0;
    } else {
        actualLeftSpeed = saturate(targetLeftSpeed - actualLeftSpeed, -rampStep, rampStep) + actualLeftSpeed;
    }

    const int16_t leftSpeed = -saturate(actualLeftSpeed, configuration->minAbsoluteSpeed, configuration->maxAbsoluteSpeed);
    const int16_t rightSpeed = saturate(actualRightSpeed, configuration->minAbsoluteSpeed, configuration->maxAbsoluteSpeed);

    int16_t speeds[2] = { leftSpeed, rightSpeed };

    try {
        i2c->write16bitArray(driverAddress, 0x00, speeds, 2);
    } catch (const char *error) {
        std::cout << error << std::endl;
    }
}

void Motors::updateOdometry() {
    int32_t values[2];

    try {
        i2c->read32bitArray(driverAddress, odometryAddress, values, 2);
    } catch(char *error) {
        throw "failed to read odometry";
    }

    auto left = (float) values[0];
    auto right = (float) -values[1];

    auto ds = (left + right) * configuration->wheelCircumference() / 6400 / 2;
    auto da = (left - right) * configuration->wheelCircumference() / 6400 / configuration->axleWidth;

    std::lock_guard<std::mutex> lock(odometryMutex);

    odometry.ds = ds;
    odometry.da = da;

    let dx = ds * std::cos(odometry.a + da / 2);
    let dy = ds * std::sin(odometry.a + da / 2);

    odometry.x += dx;
    odometry.y += dy;

    odometry.a += da;
}

Odometry Motors::readOdometry() {
    std::lock_guard<std::mutex> lock(odometryMutex);

    return odometry;

}

void Motors::forward(float linearSpeed, float angularSpeed) {
    setSpeed(linearSpeed, linearSpeed, angularSpeed);
}

void Motors::resetOdometry() {
    std::lock_guard<std::mutex> lock(odometryMutex);
    odometry.ds = 0;
    odometry.da = 0;
    odometry.x = 0;
    odometry.y = 0;
    odometry.a = 0;
}
