#ifndef CAMERA_H
#define CAMERA_H

#include "rt_utils.h"

class Camera {
private:
    Point3 origin;
    Point3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
    Vec3 u, v, w; // Ensure these are accessible by the getters

public:
    Camera(
        Point3 lookfrom,
        Point3 lookat,
        Vec3   vup,
        double vfov, // top-to-bottom field-of-view in degrees
        double aspect_ratio

    ) {
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2.0 * h;
        auto viewport_width = aspect_ratio * viewport_height;

        auto w = unit_vector(lookfrom - lookat);
        auto u = unit_vector(cross(vup, w));
        auto v = cross(w, u);

        origin = lookfrom;
        horizontal = viewport_width * u;
        vertical = viewport_height * v;
        lower_left_corner = origin - horizontal / 2 - vertical / 2 - w;
    }

    // Add these lines to expose the internal orientation vectors:
    Vec3 get_forward() const { return -unit_vector(w); } // Forward facing vector
    Vec3 get_right()   const { return unit_vector(u); }   // Right facing vector
    Vec3 get_up()      const { return unit_vector(v); }   // Up facing vector



    Ray get_ray(double s, double t) const {
        return Ray(origin, lower_left_corner + s * horizontal + t * vertical - origin);
    }
};

#endif