#ifndef TRAFFICLIGHT_LIGHT_H
#define TRAFFICLIGHT_LIGHT_H

#include <string>
#include <vector>


enum class Color {
    RED,
    YELLOW,
    GREEN,
};

enum class Symbol {
    CIRCLE,
    LEFT_ARROW,
    RIGHT_ARROW,
    UP_ARROW,
    NONE,
};

enum class Flash {
    NONE,
    FLASH_EVEN,
    FLASH_ODD,
};

class Lamp {
public:
    Color color;
    Symbol symbol;
    Flash flash;

    void draw();
};

class Spec {
    std::vector<Lamp> stages;
};

class Light {

};


#endif //TRAFFICLIGHT_LIGHT_H
