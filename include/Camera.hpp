#pragma once

class Camera {
private:
    struct vec3 {
        float x;
        float y;
        float z;
    };

public:
    // configuration
    vec3 angle { -0.4f, 0.4f, 0.f };
    vec3 shift { 0.f, 0.2f, 2.7f };
};