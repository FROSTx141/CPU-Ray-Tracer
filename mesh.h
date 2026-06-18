#ifndef MESH_H
#define MESH_H

#include "hittable.h"
#include "triangle.h"
#include <vector>
#include <memory>

class Mesh : public Hittable {
public:
    std::vector<std::shared_ptr<Triangle>> triangles;

    Mesh() {}

    // Adds a triangle to the mesh with a position offset (Free Placement)
    void add_triangle(Point3 v0, Point3 v1, Point3 v2, Vec3 offset, double scale, std::shared_ptr<Material> mat) {
        // Apply scaling and position translation transforms
        Point3 p0 = (v0 * scale) + offset;
        Point3 p1 = (v1 * scale) + offset;
        Point3 p2 = (v2 * scale) + offset;
        triangles.push_back(std::make_shared<Triangle>(p0, p1, p2, mat));
    }

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        HitRecord temp_rec;
        bool hit_anything = false;
        auto closest_so_far = t_max;

        for (const auto& triangle : triangles) {
            if (triangle->hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }
};

#endif