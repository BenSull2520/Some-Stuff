#include <vector>
#include <cmath>
#include <sstream> 
#include <cstdio>

const double G = 1.0;  // Gravitational constant
const double dt = 0.5; // Time step

using namespace std;

struct Vector2D {

    //Deals with how the vectors are added and subtracted, as well as how multiplication and division by scalars works

    double x, y;

    Vector2D operator+(const Vector2D& other) const {
        return {x + other.x, y + other.y};
    }

    Vector2D operator-(const Vector2D& other) const {
        return {x - other.x, y - other.y};
    }

    Vector2D operator*(double scalar) const {
        return {x * scalar, y * scalar};
    }

    Vector2D operator/(double scalar) const {
        return {x / scalar, y / scalar};
    }
};

struct Planet {

    // This structure 

    double mass;
    Vector2D position;
    Vector2D velocity;
    vector<Vector2D> trail;

    Planet(double m, Vector2D p, Vector2D v) : mass(m), position(p), velocity(v) {}

    void Move() {
        trail.push_back(position);
        position = position + velocity * dt;
    }

    void ApplyGravity(const Planet& sun) {
        Vector2D distance = sun.position - position;
        double distanceMag = sqrt(distance.x * distance.x + distance.y * distance.y);
        Vector2D distanceUnit = distance / distanceMag;
        double forceMag = G * mass * sun.mass / (distanceMag * distanceMag);
        Vector2D force = distanceUnit * forceMag;
        Vector2D acceleration = force / mass;
        velocity = velocity + acceleration * dt;
    }
};

class SolarSystem {

    // This coordinates the interactions between the sun and each of the planets

    public:
    vector<Planet> planets;
    Planet sun;
    double size;

    SolarSystem(double size, const Planet& sun) : size(size), sun(sun) {}

    // Adding different plants to the solarsystem

    void AddPlanet(const Planet& planet) {
        planets.push_back(planet);
    }

    // Moving planets

    void UpdatePlanets() {
        for (auto& planet : planets) {
            planet.Move();
        }
    }

    // Applying gravity

    void ApplyGravity() {
        for (auto& planet : planets) {
            planet.ApplyGravity(sun);
        }
    }

    // Storing each frame

    void StoreFrame(FILE* gnuplotPipe) {
        if (gnuplotPipe) {
            // Plot the sun
            fprintf(gnuplotPipe, "%f %f\n", sun.position.x, sun.position.y);
            fprintf(gnuplotPipe, "e\n");

            // Plot the planets
            for (const auto& planet : planets) {
                fprintf(gnuplotPipe, "%f %f\n", planet.position.x, planet.position.y);
            }
            fprintf(gnuplotPipe, "e\n");

            // Plot the trails of all planets
            for (const auto& planet : planets) {
                for (const auto& pos : planet.trail) {
                    fprintf(gnuplotPipe, "%f %f\n", pos.x, pos.y);
                }
                fprintf(gnuplotPipe, "\n");
            }
            fprintf(gnuplotPipe, "e\n");
        }
    }
};

int main() {

    // Creating sun and solar system

    Planet sun(1e8, {0, 0}, {0, 0});
    SolarSystem solarSystem(10000, sun);

    // Initialising planets

    Planet planet1(1e3, {-2000, -2000}, {80, -50});
    Planet planet2(1e3, {-2000, -2000}, {150, 0});
    Planet planet3(1e5, {-2000, -2000}, {50, -100});
    Planet planet4(1e3, {-2000, -2000}, {100, 50});

    solarSystem.AddPlanet(planet1);
    solarSystem.AddPlanet(planet2);
    solarSystem.AddPlanet(planet3);
    solarSystem.AddPlanet(planet4);
    
    // Simulating, plotting and saving as a gif

    FILE* gnuplotPipe = popen("gnuplot -persist", "w");
    if (gnuplotPipe) {
        fprintf(gnuplotPipe, "set terminal gif animate delay 10 size 800,800\n");
        fprintf(gnuplotPipe, "set output 'Multi_Planet_Sim.gif'\n");
        fprintf(gnuplotPipe, "set xrange [-%f:%f]\n", solarSystem.size / 2, solarSystem.size / 2);
        fprintf(gnuplotPipe, "set yrange [-%f:%f]\n", solarSystem.size / 2, solarSystem.size / 2);
        fprintf(gnuplotPipe, "unset key\n");
        fprintf(gnuplotPipe, "set title 'Multi-Planet Simulation'\n");

        int frame_count = 250;

        for (int frame = 0; frame < frame_count; ++frame) {

            if (frame % 10 == 0){
                printf("Frame %d/ %d\n", frame, frame_count);
            }

            solarSystem.ApplyGravity();
            solarSystem.UpdatePlanets();
            fprintf(gnuplotPipe, "plot '-' with points pt 7 ps 2 lc rgb 'yellow' title 'Sun', '-' with points pt 7 ps 1.5 lc rgb 'blue' title 'Planets', '-' with lines lc rgb 'light-blue' title 'Planet Trails'\n");
            solarSystem.StoreFrame(gnuplotPipe);
        }

        pclose(gnuplotPipe);
    }

    printf("Done\n");
    return 0;
}