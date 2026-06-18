#ifndef BOX_H
#define BOX_H

#include "hittable.h"
#include "vec3.h"

class Box : public Hittable {
public:
    Point3 box_min;
    Point3 box_max;
    std::shared_ptr<Material> mat_ptr;

    Box() {}
    // Define a box by providing its minimum corner point and maximum corner point
    Box(Point3 p_min, Point3 p_max, std::shared_ptr<Material> m)
        : box_min(p_min), box_max(p_max), mat_ptr(m) {
    }

    // NEW: Constructor that takes a base size AND a center position vector
    Box(Point3 center, double width, double height, double depth, std::shared_ptr<Material> m) : mat_ptr(m) {
        Vec3 half_extents(width / 2.0, height / 2.0, depth / 2.0);
        box_min = center - half_extents;
        box_max = center + half_extents;
    }

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        double t0 = t_min;
        double t1 = t_max;
        int hit_axis = -1;
        bool is_min_side = true;

        // Loop through all 3 axes (X, Y, Z slabs)
        for (int i = 0; i < 3; ++i) {
            double invD = 1.0 / r.direction()[i];
            double t_near = (box_min[i] - r.origin()[i]) * invD;
            double t_far = (box_max[i] - r.origin()[i]) * invD;

            if (t_near > t_far) std::swap(t_near, t_far);

            // Update hit boundary intervals
            if (t_near > t0) { t0 = t_near; hit_axis = i; is_min_side = true; }
            if (t_far < t1) { t1 = t_far; }

            if (t0 > t1) return false; // Ray missed the bounding box overlap interval
        }

        if (hit_axis == -1) return false;

        rec.t = t0;
        rec.p = r.at(rec.t);

        // Compute the exact flat outward surface normal vector based on the hit slab axis
        Vec3 outward_normal(0, 0, 0);
        outward_normal[hit_axis] = is_min_side ? -1.0 : 1.0;

        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mat_ptr;

        return true;
    }
};

#endif