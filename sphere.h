#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"

extern bool debug_mode;

class Sphere : public Hittable {
public:
    Point3 center;
    double radius = 30;
    std::shared_ptr<Material> mat_ptr;

    Sphere() {}
    Sphere(Point3 cen, double r, std::shared_ptr<Material> m)
        : center(cen), radius(r), mat_ptr(m) {
    };

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        Vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;

        auto discriminant = half_b * half_b - a * c;
        if (discriminant < 0) return false;
        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (-half_b - sqrtd) / a;
        if (root < t_min || t_max < root) {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || t_max < root)
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        Vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mat_ptr;

        if (debug_mode) {
            // 1. Calculate the local relative position from the sphere's center
            Vec3 local_p = unit_vector(rec.p - center);

            // 2. Convert 3D Cartesian coordinates to Spherical coordinates (phi, theta)
            // phi ranges from 0 to 2*pi, theta ranges from 0 to pi
            double phi = std::atan2(local_p.z(), local_p.x()) + 3.1415926535897932385;
            double theta = std::acos(local_p.y());

            // 3. Define grid frequency (how many lines) and thickness
            double grid_density = 12.0; // Number of longitudinal/latitudinal wire rings
            double thickness = 0.05;    // Width of the neon wire lines

            // 4. Use a sine wave function to check if the point lands on a grid boundary
            double u_line = std::abs(std::sin(phi * grid_density));
            double v_line = std::abs(std::sin(theta * grid_density));

            // If the ray didn't hit near a latitude or longitude line, pass through it (transparent)
            if (u_line > thickness && v_line > thickness) {
                return false; // Ray passes right through the gaps in the cage
            }
        }

        return true;
    }
};

#endif