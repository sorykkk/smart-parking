#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class ISensor {
protected:
    String name;
    String type;
    String technology;
    int index;
public:
    virtual ~ISensor() = default;
    virtual String toJson() const = 0;
    virtual String getName() const = 0;
    virtual String getType() const = 0;
    virtual String getTechnology() const = 0;
    virtual int getIndex() const = 0;
    virtual bool checkState() = 0;
};

#endif