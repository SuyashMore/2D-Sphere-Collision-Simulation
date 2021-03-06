// Projectiletest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "SFML\Graphics.hpp"
#include <cmath>
#include <vector>
#include <time.h>
#include <random>
#include <windows.h>

// Experimental Features
#define RAPID_DROP true

// Shortcuts
#define LOG(x) std::cout << x << std::endl;
#define LOG2(x, y) std::cout << x << " " << y << std::endl;

// Constants
#define PI 3.14159
#define ENABLE true
#define DISABLE false

// variable Parameters
#define WALL_FRICTION 0.5f
#define FLOOR_FRICTION 0.8f
#define CEIL_FRICTION 0.3f
#define GRAVITY 5.0f
#define D_TIME 0.003f

#define RADIUS 20.0f
#define FORCE_MULTIPLIER 0.2f
#define MINIMUM_FORCE_TO_RELEASE 20.0f
#define CLOCK_CORRECTION 0.5f

// Utility Functions
void swap(float& a, float& b)
{
    float c = a;
    a = b;
    b = c;
}

float toDegrees(float angle) { return (180 / PI) * angle; }

float toRad(float angle) { return (PI / 180) * angle; }

float distanceof(float x1, float y1, float x2, float y2)
{
    return powf(powf(x1 - x2, 2) + powf(y1 - y2, 2), 0.5);
}

// Data Containers
struct Window {
    int width;
    int height;
    Window(int width, int height)
        : width(width)
        , height(height)
    {
    }
};

struct Particle_meta {
    int x = 0;
    int y = 0;
    float Force = 0;
    float force_angle = 0;

    void print_Data_location()
    {
        std::cout << "Reference Particls :: x = " << x << " , y  = " << y
                  << std::endl;
    }
    void print_Data_forces()
    {
        std::cout << "Reference Particle :: Force = " << Force
                  << " , Force_angle = " << force_angle << std::endl;
    }
    void print_Data()
    {
        print_Data_location();
        print_Data_forces();
    }
};

struct Mouse {
    int x = 0;
    int y = 0;
    bool isButtonPressed_L = false;
    bool isButtonHold_L = false;
    bool isButtonPressed_R = false;
    bool isButtonHold_R = false;

    void print_Data()
    {
        std::cout << "@Mouse :: "
                  << "x :" << x << " , y :" << y << " , Left :" << isButtonPressed_L
                  << " , Right :" << isButtonPressed_R << std::endl;
    }
};
Mouse mouse; // global mouse data object

// Main Classes
class Particles {
public:
    float x, y, velocity_x, velocity_y, displacement_x, displacement_y, radius;
    bool isFreezed = false;
    float clock_correction = 1;

    Particles(int x = 0, int y = 0, int radius = RADIUS, int velocity_x = 0,
        int velocity_y = 0)
    {
        this->x = x;
        this->y = y;
        this->radius = radius;
        this->velocity_x = velocity_x;
        this->velocity_y = velocity_y;
    }

    void process_motion_x()
    {
        displacement_x = velocity_x * D_TIME; // s = ut ;
        x += displacement_x * clock_correction;
    }
    void process_motion_y()
    {
        displacement_y = velocity_y * D_TIME - 0.5 * (GRAVITY)*powf(D_TIME, 2); // s = ut + 0.5*a*t^2
        velocity_y = velocity_y - (GRAVITY) * (D_TIME); // v = u + at

        y -= displacement_y * clock_correction;
    }

    void process_motion(float cc)
    {
        clock_correction = cc;
        // LOG(clock_correction);
        if (!isFreezed) {
            process_motion_x();
            process_motion_y();
        }
    }

    void impart_force(int Force, int Force_angle_degree)
    {
        Force *= FORCE_MULTIPLIER;
        velocity_x = Force * cosf(toRad(Force_angle_degree));
        velocity_y = Force * sinf(toRad(Force_angle_degree));
    }
    void freeze()
    {
        velocity_x = 0;
        velocity_y = 0;
        isFreezed = true;
    }
    void unfreeze() { isFreezed = false; }
};

class Simulation {
public:
    Simulation(Window* win)
    {
        sf::RenderWindow* t = new sf::RenderWindow(sf::VideoMode(win->width, win->height),
            "Particle Collision Simulation");
        app = t;
        window = win;

        circle.setRadius(RADIUS);
        circle.setOrigin(RADIUS, RADIUS);

        circle2.setRadius(RADIUS);
        circle2.setOrigin(RADIUS, RADIUS);

        srand(time(0));

        init(); // calls the Main Loop
    }

private:
    sf::RenderWindow* app;
    sf::Event event;
    Window* window;
    sf::CircleShape circle, circle2; // Reference circle , circle2 is for drawing
    // the simulating circles
    Particle_meta ref_particle; // reference particle
    std::vector<Particles> particles; // vector to hold all the particles

    void init()
    {
        // Main Loop
        while (app->isOpen()) {
            app->clear();
            manage_events();

            loop();
            app->display();
        }
    }

    void loop()
    {
        mouse_follower();
        update_mouse();
        simulate_particles();
        draw_particles();
    }

    void simulate_particles()
    {
        for (int i = 0; i < (particles.size()); i++) {
            particles[i].process_motion(1 + particles.size() * CLOCK_CORRECTION);
            check_limits(particles[i]); // check for collisions with the walls
            detect_collisions(particles[i], i);
        }
    }

    void check_limits(Particles& t) // checks for collisions with the walls
    {
        if (t.x >= (window->width - RADIUS)) // check for right Boundary
        {
            t.x = window->width - RADIUS - 1;
            t.velocity_x *= -1; // update direction
            t.velocity_x *= WALL_FRICTION; // account for inelastic collision
        }
        else if (t.x <= RADIUS) // check for left Boundary
        {
            t.x = RADIUS + 1;
            t.velocity_x *= -1; // update direction
            t.velocity_x *= WALL_FRICTION; // account for inelastic collision
        }

        if (t.y <= RADIUS) {
            t.y = RADIUS + 1;
            t.velocity_y *= -1;
            t.velocity_y *= CEIL_FRICTION;
        }
        else if (t.y >= (window->height - RADIUS)) {
            t.y = window->height - RADIUS - 1;
            t.velocity_y *= -1;
            t.velocity_y *= FLOOR_FRICTION;
        }
    }

    void detect_collisions(Particles& t, int self_index)
    {
        for (int i = 0; i < particles.size(); i++) // loop through all the
        // particles to check the DIFF.
        // in radiis of the two circles
        {
            if (i != self_index) {
                float d_bw_centres = distanceof(t.x, t.y, particles[i].x, particles[i].y);
                if (d_bw_centres <= 2 * RADIUS) {
                    swap(t.velocity_x, particles[i].velocity_x);
                    swap(t.velocity_y, particles[i].velocity_y);
                }
            }
        }
    }

    void draw_particles()
    {
        for (int i = 0; i < (particles.size()); i++) {
            circle2.setPosition(particles[i].x, particles[i].y);

            ////////////////////// Color Coding ////////////////
            switch (i % 6) {
            case 0:
                circle2.setFillColor(sf::Color::Red);
                break;
            case 1:
                circle2.setFillColor(sf::Color::Yellow);
                break;
            case 2:
                circle2.setFillColor(sf::Color::Blue);
                break;
            case 3:
                circle2.setFillColor(sf::Color::Green);
                break;
            case 4:
                circle2.setFillColor(sf::Color::Magenta);
                break;
            case 5:
                circle2.setFillColor(sf::Color::Cyan);
                break;
            default:
                circle2.setFillColor(sf::Color::White);
            }
            //////////////////////////////////////////

            app->draw(circle2);
        }
    }

    void manage_events()
    {
        while (app->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                app->close();
            }
        }
    }

    void update_mouse()
    {
        if (mouse.isButtonPressed_L && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            mouse.isButtonHold_L = true;
        }
        else {
            mouse.isButtonHold_L = false;
        }
        mouse.isButtonPressed_L = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        if (mouse.isButtonPressed_R && sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
            mouse.isButtonHold_R = true;
        }
        else {
            mouse.isButtonHold_R = false;
        }
        mouse.isButtonPressed_R = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

        sf::Vector2i mdata = sf::Mouse::getPosition(*app);
        mouse.x = mdata.x;
        mouse.y = mdata.y;
    }

    void mouse_follower()
    {
        LOG(particles.size());
        if (ref_particle.Force > MINIMUM_FORCE_TO_RELEASE && !mouse.isButtonHold_L) //  Throw/Instantiate Particle
        {
            // std::cout << "released from x:" << ref_particle.x << " , y:" <<
            // ref_particle.y << " With Force:" << ref_particle.Force << std::endl;
            particles.push_back(Particles(
                ref_particle.x, ref_particle.y,
                RADIUS)); // instantiate particle widht the reference parameters
            (particles[particles.size() - 1])
                .impart_force(ref_particle.Force, ref_particle.force_angle);

            ref_particle.Force = 0;
            ref_particle.x = mouse.x;
            ref_particle.y = mouse.y;
        }

        if (!mouse.isButtonHold_L) {
            circle.setPosition(mouse.x, mouse.y);
            ref_particle.x = mouse.x;
            ref_particle.y = mouse.y;
            app->draw(circle);
        }
        else if (ref_particle.Force < MINIMUM_FORCE_TO_RELEASE || mouse.isButtonHold_L) // Apply Force Mode
        {
            sf::Vector2f t = circle.getPosition();
            circle.setPosition(t.x, t.y);
            ref_particle.x = t.x;
            ref_particle.y = t.y;
            app->draw(circle);
            compute_Pull_Forces(true);
        }

        if (mouse.isButtonPressed_R && RAPID_DROP) // Rapid Drop
        {
            particles.push_back(Particles(
                ref_particle.x, ref_particle.y,
                RADIUS)); // instantiate particle widht the reference parameters
            (particles[particles.size() - 1])
                .impart_force(rand() % 100, rand() % 180 + 1);

            ref_particle.Force = 0;
            ref_particle.x = mouse.x;
            ref_particle.y = mouse.y;
            Sleep(30);
        }
    }

    void compute_Pull_Forces(bool enableGraphic)
    {
        if (mouse.isButtonHold_L) {
            // Distance Formula
            ref_particle.Force = distanceof(mouse.x, mouse.y, ref_particle.x, ref_particle.y);
            // Force angle
            ref_particle.force_angle = toDegrees(atanf(
                -(-mouse.y + ref_particle.y) / (-mouse.x + ref_particle.x + 0.01)));
            if (mouse.x > ref_particle.x) {
                ref_particle.force_angle += 180;
            }

            if (enableGraphic) {
                sf::VertexArray line(sf::LinesStrip, 2);
                line[0].position = sf::Vector2f(mouse.x, mouse.y);
                line[1].position = sf::Vector2f(ref_particle.x, ref_particle.y);
                app->draw(line);
            }
        }
    }
};

int main()
{
    Window window(1280, 800);

    Simulation obj(&window);

    return 0;
}