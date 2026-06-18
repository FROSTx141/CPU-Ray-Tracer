
////////////// PROGRAMMING FUNDAMENTALS PROJECT ////////////////////////
////////////////////// CPU RAY TRACER /////////////////////////////////
// 
///////// THIS PROJECT HAS DEPENDECIES, IT WILL NOT WORK FOR YOU IF YOU DON'T HAVE SPECIFIC DEPENDECIES INSTALLED.

// 
/////////// DEPENDECIES THIS PROJECT USES ///////////////////////////
/////////// 1. SFML GRAPHICS VERSION (2.4.2) ////////////////////////
/////////// 2. SFML SYSTEM VERSION (2.4.2) /////////////////////////
/////////// 3. SFML WINDOW VERSION (2.4.2) ////////////////////////
// 
///////////////////////////////////////////////////////////////////
// 
/////////////////  ADDITIONAL HEADER FILES REQUIRED ///////////////




#include "rt_utils.h"
#include "hittable_list.h"
#include "sphere.h"
#include "box.h"
#include "camera.h"
#include "material.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "mesh.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

bool debug_mode = false; // Toggle with the 'G' key

std::shared_ptr<Mesh> load_obj_model(const std::string& filename, Vec3 position, double scale, std::shared_ptr<Material> mat) {
    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename, reader_config)) {
        std::cerr << "TinyObjLoader Error: " << reader.Error() << std::endl;
        return nullptr;
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto my_mesh = std::make_shared<Mesh>();

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            tinyobj::index_t idx0 = shapes[s].mesh.indices[index_offset + 0];
            tinyobj::index_t idx1 = shapes[s].mesh.indices[index_offset + 1];
            tinyobj::index_t idx2 = shapes[s].mesh.indices[index_offset + 2];

            Point3 v0(attrib.vertices[3 * size_t(idx0.vertex_index) + 0],
                attrib.vertices[3 * size_t(idx0.vertex_index) + 1],
                attrib.vertices[3 * size_t(idx0.vertex_index) + 2]);

            Point3 v1(attrib.vertices[3 * size_t(idx1.vertex_index) + 0],
                attrib.vertices[3 * size_t(idx1.vertex_index) + 1],
                attrib.vertices[3 * size_t(idx1.vertex_index) + 2]);

            Point3 v2(attrib.vertices[3 * size_t(idx2.vertex_index) + 0],
                attrib.vertices[3 * size_t(idx2.vertex_index) + 1],
                attrib.vertices[3 * size_t(idx2.vertex_index) + 2]);

            my_mesh->add_triangle(v0, v1, v2, position, scale, mat);
            index_offset += fv;
        }
    }
    return my_mesh;
}

// Dynamic ray calculation routing helper
Color ray_color(const Ray& r, const Hittable& world, int depth) {
    HitRecord rec;
    if (depth <= 0) return Color(0, 0, 0);

    if (world.hit(r, 0.001, infinity, rec)) {
        Ray scattered;
        Color attenuation;
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth - 1);
        return Color(0, 0, 0);
    }

    Vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * Color(1.0, 1.0, 1.0) + t * Color(0.5, 0.7, 1.0);
}

void draw_debug_ray(sf::RenderWindow& window, const Point3& start, const Point3& end, sf::Color color) {
    // Basic screen projection projection mapping approximation for debugging
    // This maps 3D world coordinates onto 2D SFML screen space layout
    float screen_x = ((start.x() + 2.0) / 4.0) * 800.0;
    float screen_y = (1.0 - (start.y() + 1.0) / 2.0) * 450.0;
    float target_x = ((end.x() + 2.0) / 4.0) * 800.0;
    float target_y = (1.0 - (end.y() + 1.0) / 2.0) * 450.0;

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(screen_x, screen_y), color),
        sf::Vertex(sf::Vector2f(target_x, target_y), color)
    };
    window.draw(line, 2, sf::Lines);
}


int main()
{
    // Window Display Resolution Setup
    const int window_width = 950;
    const int window_height = 720;


    // Create an native OS Window framework context 
    sf::RenderWindow window(sf::VideoMode(window_width, window_height), "C++ Real-Time Ray Tracer Sandbox");
    window.setFramerateLimit(60);

    // Render configuration optimization fields
    int samples_per_pixel = 1;  // Low sampling for real time responsiveness when moving
    int max_depth = 8;
    bool camera_moved = true;

    // SFML Texture buffer layouts to pull data onto screen frame
    sf::Texture texture;
    texture.create(window_width, window_height);
    sf::Sprite sprite(texture);
    std::vector<sf::Uint8> pixel_buffer(window_width * window_height * 4); // RGBA layout

    // Scene Geometry 
    HittableList world;

    //auto material_ground = std::make_shared<Metal>(Color(0.8, 0.8, 0.), 0.0);  ground
    auto material_ground = std::make_shared<Lambertian>(Color(0.8, 0.8, 0.0));

    auto material_center = std::make_shared<Lambertian>(Color(0.1, 0.2, 0.5));
    auto material_left = std::make_shared<Dielectric>(1.5);
    auto material_right = std::make_shared<Metal>(Color(0.8, 0.6, 0.2), 0.0);

    //world.add(std::make_shared<Sphere>(Point3(0.0, -100.5, -1.0), 100.0, material_ground));  sphere
    world.add(std::make_shared<Box>(Point3(0.0, -1.0, -1.0), 100.0, 1.0, 100.0, material_ground)); // cube

    world.add(std::make_shared<Sphere>(Point3(0.0, 0.0, -1.0), 0.5, material_center));
    world.add(std::make_shared<Sphere>(Point3(-1.4, 0.0, -1.0), 0.5, material_left));
    world.add(std::make_shared<Sphere>(Point3(1.4, 0.0, -1.0), 0.5, material_right));

    //CUBE
    auto red_mat = std::make_shared<Lambertian>(Color(1.2, 0.2, 0.2));
    auto glass_mat = std::make_shared<Dielectric>(0.8);
    auto gold_metal = std::make_shared<Metal>(Color(0.8, 0.6, 0.2), 0.0);


    // centered at position X, Y, Z with size (W, H, D)
    world.add(std::make_shared<Box>(Point3(0.0, 0.0, -4.0), 1.0, 1.0, 1.0, red_mat));
    world.add(std::make_shared<Box>(Point3(1.5, 0.0, -4.0), 1.0, 1.0, 1.0, glass_mat));
    world.add(std::make_shared<Box>(Point3(-1.5, 0.0, -4.0), 1.0, 1.0, 1.0, gold_metal));



    /* ////////////////// .obj file //////////////////////////////////////////////////
    //auto material_ground = std::make_shared<Lambertian>(Color(0.8, 0.8, 0.0));
    //world.add(std::make_shared<Sphere>(Point3(0.0, -100.5, -1.0), 100.0, material_ground)); //ground plane


    auto gold_metal = std::make_shared<Metal>(Color(0.8, 0.6, 0.2), 0.0);
    auto glass_mat = std::make_shared<Dielectric>(1.5);

     //Arguments: File Path, Position Offset (X, Y, Z), Scale Multiplier, Material Style
    auto model_1 = load_obj_model("Tree.obj", Vec3(-1.5, 0.0, -2.0), 0.3, gold_metal);
    //if (model_1) world.add(model_1);

    auto model_2 = load_obj_model("bunny.obj", Vec3(1.5, 0.0, -1.5), 0.5, glass_mat);
    if (model_2) world.add(model_2);
    /*/

    // Interactive Camera Parameters tracking state
    Point3 lookfrom(-2, 2, 1);
    Point3 lookat(0, 0, -1);
    double aspect_ratio = double(window_width) / window_height;
    Camera cam(lookfrom, lookat, Vec3(0, 1, 0), 20, aspect_ratio);

    //  MOVEMENT VIEWTRACKING VARIABLES
    double yaw = -90.0;   // Horizon rotation (Facing down the -Z axis initially)
    double pitch = 0.0;   // Vertical look rotation tilt
    bool mouse_focused = true;

    window.setMouseCursorVisible(false); // Hide the standard OS arrow cursor
    sf::Mouse::setPosition(sf::Vector2i(window_width / 2, window_height / 2), window);

    //  clock before the loop to track time 
    sf::Clock fps_clock;

    // Application Window Loop 
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::G)
                {
                    debug_mode = !debug_mode;
                    camera_moved = true;
                }
            }

            // Press Escape to release/re-lock the mouse cursor context
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                mouse_focused = !mouse_focused;
                window.setMouseCursorVisible(!mouse_focused);
            }
        }

        // Mouse Look Rotation
        if (mouse_focused && window.hasFocus()) {
            sf::Vector2i center_pos(window_width / 2, window_height / 2);
            sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

            double delta_x = mouse_pos.x - center_pos.x;
            double delta_y = mouse_pos.y - center_pos.y;

            if (delta_x != 0 || delta_y != 0) {
                double sensitivity = 0.15;
                yaw += delta_x * sensitivity;
                pitch -= delta_y * sensitivity; // Inverted so dragging up tilts up

                // Prevent the camera from doing flips upside down
                if (pitch > 89.0)  pitch = 89.0;
                if (pitch < -89.0) pitch = -89.0;

                sf::Mouse::setPosition(center_pos, window); // Snaps mouse right back to center
                camera_moved = true;
            }
        }

        // KEYBOARD VECTOR RELATIVE DIRECTION FLYING CONTROL 
        double move_speed = 0.15;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) move_speed *= 2.5; // Sprint mode

        // Calculate a directional vector pointing forward based on angles
        Vec3 forward;
        forward.e[0] = cos(degrees_to_radians(yaw)) * cos(degrees_to_radians(pitch));
        forward.e[1] = sin(degrees_to_radians(pitch));
        forward.e[2] = sin(degrees_to_radians(yaw)) * cos(degrees_to_radians(pitch));
        forward = unit_vector(forward);

        Vec3 right = unit_vector(cross(forward, Vec3(0, 1, 0)));
        Vec3 up = Vec3(0, 1, 0); // World up axis for smooth vertical flight lifting

        // Move relative to current vector directions instead of locking to cardinal bounds
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { lookfrom += forward * move_speed; camera_moved = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { lookfrom -= forward * move_speed; camera_moved = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { lookfrom -= right * move_speed; camera_moved = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { lookfrom += right * move_speed;  camera_moved = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) { lookfrom += up * move_speed;   camera_moved = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) { lookfrom -= up * move_speed;  camera_moved = true; }

        // PROGRESSIVE SAMPLING CONTROLLER 
        if (camera_moved) {
            lookat = lookfrom + forward; // Target looks straight down the directional heading vector
            cam = Camera(lookfrom, lookat, Vec3(0, 1, 0), 20, aspect_ratio);
            samples_per_pixel = 1; // Real time performance speed
            max_depth = 4;        // Shorter ray depth bounds during real-time updates
        }
        else {
            // Progressive enhancement sharpen the scene when standing still
            if (samples_per_pixel < 16) {
                samples_per_pixel += 1;
                max_depth = 20;
            }
            else {
                // Scene fully rendered up to 32 SPP. Sleep thread and loop around to protect CPU usage.
                sf::sleep(sf::milliseconds(16));
                continue;
            }
        }

        // Start timing this specific frame calculation pass
        sf::Time frame_start_time = fps_clock.getElapsedTime();

        //  Parallel thread distributor
        int next_row = window_height - 1;
        std::mutex row_mutex;

        auto worker = [&]() {
            while (true) {
                int y;
                {
                    std::lock_guard<std::mutex> lock(row_mutex);
                    if (next_row < 0) return;
                    y = next_row--;
                }

                for (int x = 0; x < window_width; ++x) {
                    Color pixel_color(0, 0, 0);
                    for (int s = 0; s < samples_per_pixel; ++s) {
                        auto u = (double(x) + random_double()) / (window_width - 1);
                        auto v = (double(y) + random_double()) / (window_height - 1);
                        Ray r = cam.get_ray(u, v);
                        pixel_color += ray_color(r, world, max_depth);
                    }

                    auto scale = 1.0 / samples_per_pixel;
                    double r = std::sqrt(scale * pixel_color.x());
                    double g = std::sqrt(scale * pixel_color.y());
                    double b = std::sqrt(scale * pixel_color.z());

                    int index = (((window_height - 1) - y) * window_width + x) * 4;
                    pixel_buffer[index] = static_cast<sf::Uint8>(255.999 * clamp(r, 0.0, 0.999));
                    pixel_buffer[index + 1] = static_cast<sf::Uint8>(255.999 * clamp(g, 0.0, 0.999));
                    pixel_buffer[index + 2] = static_cast<sf::Uint8>(255.999 * clamp(b, 0.0, 0.999));
                    pixel_buffer[index + 3] = 255;
                }
            }
            };

        unsigned int num_threads = 6; // Set to exactly 6 threads //std::thread::hardware_concurrency(); // threads
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < num_threads; ++i) threads.emplace_back(worker);
        for (auto& t : threads) t.join();

        // PERFORMANCE METRICS 
        sf::Time frame_end_time = fps_clock.getElapsedTime();
        float frame_render_seconds = (frame_end_time - frame_start_time).asSeconds();
        float current_fps = 1.0f / frame_render_seconds;

        // Accurate Real time Total Ray Cast Calculations tracking
        int total_rays_cast = window_height * window_width * samples_per_pixel * max_depth;

        std::clog << "\r[FRAME DATA] Render Time: " << frame_render_seconds << "s | "
            << "FPS: " << static_cast<int>(current_fps) << " | "
            << "Samples: " << samples_per_pixel << " spp | "
            << "Rays Cast: " << total_rays_cast << "      " << std::flush;

        // Display Layout
        texture.update(pixel_buffer.data());
        window.clear();
        window.draw(sprite);

        // DRAW DEBUG 
        if (debug_mode) {
            // Draw a grid pattern showing center lines of ray intersections
            draw_debug_ray(window, Point3(-10, 0, -4), Point3(10, 0, -4), sf::Color::Green);


            for (int i = -5; i <= 5; i += 2) {
                draw_debug_ray(window, lookfrom, Point3(i * 0.5, 0.0, -4.0), sf::Color::Red);
            }
        }

        window.display();

        camera_moved = false;
    }

    return 0;
}