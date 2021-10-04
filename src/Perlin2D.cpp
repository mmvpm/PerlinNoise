#include "include/Perlin2D.hpp"

Perlin2D::Perlin2D(int tile_size, int octaves) : tile_size(tile_size), octaves(octaves) {}

float Perlin2D::get_plain_noise(point_float point) {
    int x_start = (int) point.first;
    int x_end = x_start + 1;
    int y_start = (int) point.second;
    int y_end = y_start + 1;

    std::vector<float> dots;
    for (int grid_x = x_start; grid_x <= x_end; ++grid_x) {
        for (int grid_y = y_start; grid_y <= y_end; ++grid_y) {
            point_int grid_point = {grid_x, grid_y};
            if (angles.count(grid_point) == 0) {
                angles[grid_point] = generate_angle();
            }
            point_float gradient = angle_to_point(angles[grid_point]);
            dots.push_back(
                gradient.first * (point.first - (float) grid_x) +
                gradient.second * (point.second - (float) grid_y)
            );
        }
    }

    float s = smooth_step(point.second - (float) y_start);
    float inter_left  = linear_interpolation(s, dots[0], dots[1]);
    float inter_right = linear_interpolation(s, dots[2], dots[3]);

    s = smooth_step(point.first - (float) x_start);
    float inter = linear_interpolation(s, inter_left, inter_right);

    return inter * scale_factor;
}

void Perlin2D::update_angles(float eps) {
    for (auto &[point, angle]: angles) {
        double xy_coefficient = std::sin(point.first) * std::sin(point.second);
        angle += eps * (float) (std::abs(xy_coefficient) + 1.f);
        if (angle > 2 * M_PI)
            angle -= 2 * M_PI;
        angles[point] = angle;
    }
}

float Perlin2D::compute_noise(float x, float y) {
    float result = 0;
    for (int o = 0; o < octaves; ++o) {
        float o2 = 1 << o;
        x *= o2;
        y *= o2;
        if (tile_size != 0) {
            float m = (float) tile_size * o2;
            x = x - (float) ((int) (x / m)) * m;
            y = y - (float) ((int) (y / m)) * m;
        }
        result += get_plain_noise({x, y}) / o2;
    }
    result /= 2.f - (float) std::pow(2, 1 - octaves);
    return result;
}