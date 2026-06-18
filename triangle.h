#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"
#include "vec3.h"

class Triangle : public Hittable {
public:
    Point3 v0, v1, v2;
    std::shared_ptr<Material> mat_ptr;

    Triangle(Point3 _v0, Point3 _v1, Point3 _v2, std::shared_ptr<Material> m)
        : v0(_v0), v1(_v1), v2(_v2), mat_ptr(m) {
    }

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        // Möller–Trumbore intersection algorithm
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 h = cross(r.direction(), edge2);
        double a = dot(edge1, h);

        if (a > -1e-6 && a < 1e-6) return false; // Ray is parallel to this triangle

        double f = 1.0 / a;
        Vec3 s = r.origin() - v0;
        double u = f * dot(s, h);

        if (u < 0.0 || u > 1.0) return false;

        Vec3 q = cross(s, edge1);
        double v = f * dot(r.direction(), q);

        if (v < 0.0 || u + v > 1.0) return false;

        // Calculate exactly where along the ray the intersection occurs
        double t = f * dot(edge2, q);

        if (t > t_min && t < t_max) {
            rec.t = t;
            rec.p = r.at(rec.t);
            // Calculate flat surface normal
            Vec3 outward_normal = unit_vector(cross(edge1, edge2));
            rec.set_face_normal(r, outward_normal);
            rec.mat_ptr = mat_ptr;
            return true;
        }

        return false;
    }
};

#endif