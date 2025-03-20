#include <cmath>
#include <vector>
#include <fstream>
#include <ctime>
#include <random>

using namespace std;

const double time_step = 0.1;
const int num_particles = 100;
const double box_size = 10.0;
const int grid_size = static_cast<int>(ceil(sqrt(num_particles)));

struct Particle {
    double x, y;
    double vx, vy;
    double radius;
    double mass;

    void Move(double restitution) {
        x += vx * time_step;
        y += vy * time_step;

        // Reflect from walls
        if (x - radius < 0 || x + radius > box_size) {
            vx = -vx;
            if (x - radius < 0) x = radius;
            if (x + radius > box_size) x = box_size - radius;
        }
        if (y - radius < 0 || y + radius > box_size) {
            vy = -vy;
            if (y - radius < 0) y = radius;
            if (y + radius > box_size) y = box_size - radius;
        }
    }

    void Collision(Particle &other, double restitution) {
        double dx = x - other.x;
        double dy = y - other.y;
        double distance = sqrt(dx * dx + dy * dy);
        double minDist = radius + other.radius;

        if (distance < minDist) {
            // Calculate normal and tangent unit vectors
            double nx = dx / distance;
            double ny = dy / distance;
            double tx = -ny;
            double ty = nx;

            // Project velocities onto the normal and tangent directions
            double v1n = vx * nx + vy * ny;
            double v1t = vx * tx + vy * ty;
            double v2n = other.vx * nx + other.vy * ny;
            double v2t = other.vx * tx + other.vy * ty;

            // Calculate new normal velocities based on restitution and mass
            double m1 = mass;
            double m2 = other.mass;
            double new_v1n = (v1n * (m1 - m2) + 2 * m2 * v2n) / (m1 + m2);
            double new_v2n = (v2n * (m2 - m1) + 2 * m1 * v1n) / (m1 + m2);

            // Apply restitution
            new_v1n *= restitution;
            new_v2n *= restitution;

            // Update velocities
            vx = new_v1n * nx + v1t * tx;
            vy = new_v1n * ny + v1t * ty;
            other.vx = new_v2n * nx + v2t * tx;
            other.vy = new_v2n * ny + v2t * ty;
        }
    }
};

void update_positions(vector<Particle> &particles, double restitution) {
    for (auto &p : particles) {
        p.Move(restitution);
    }
}

void handle_collisions(vector<Particle> &particles, double restitution) {
    for (size_t i = 0; i < particles.size(); ++i) {
        for (size_t j = i + 1; j < particles.size(); ++j) {
            particles[i].Collision(particles[j], restitution);
        }
    }
}

double TotalKineticEnergy(const vector<Particle> &particles) {
    double totalEnergy = 0.0;
    for (const auto &p : particles) {
        totalEnergy += 0.5 * p.mass * (p.vx * p.vx + p.vy * p.vy);
    }
    return totalEnergy;
}

int main() {
    srand(time(0));
    vector<Particle> particles;

    // Track occupied cells - new particles won't be created in occupied cells in order to avoid overlapping
    
    bool grid[grid_size][grid_size] = {false};  
    double cell_size = box_size / grid_size;

    // A method of creating a normal distribution
    random_device rd;
    mt19937 gen(rd());
    normal_distribution<> normal_dist(0.0, 1.5);

    // Initialize particles without overlap
    for (int i = 0; i < num_particles; ++i) {
        int cell_x, cell_y;
        do {
            cell_x = rand() % grid_size;
            cell_y = rand() % grid_size;
        } while (grid[cell_x][cell_y]);  // Find an empty cell

        grid[cell_x][cell_y] = true;

        Particle p;
        p.x = (cell_x + 0.5) * cell_size;
        p.y = (cell_y + 0.5) * cell_size;
        p.vx = normal_dist(gen); 
        p.vy = normal_dist(gen);  
        
        // All particles have equal mass and radius
        p.radius = 0.1;
        p.mass = 1; 

        particles.push_back(p);
    }

    double restitution;
    printf("Enter the coefficient of restitution (0 to 1): ");
    scanf("%lf", &restitution);
    if (restitution > 1 || restitution <0){
        restitution = 1;
        printf("Error in input - Coefficient of Restitution has been set to 1.\n");
    }

    printf("Coefficient of Restitution = %.2f\n", restitution);

    FILE *gnuplotPipe = popen("gnuplot -persist", "w");
    if (gnuplotPipe) {
        fprintf(gnuplotPipe, "set terminal gif animate delay 1 size 800,800\n");
        fprintf(gnuplotPipe, "set output 'Particle_Collision.gif'\n");
        fprintf(gnuplotPipe, "set xrange [0:%f]\n", box_size);
        fprintf(gnuplotPipe, "set yrange [0:%f]\n", box_size);
        fprintf(gnuplotPipe, "unset key\n");
        fprintf(gnuplotPipe, "set title 'Particle Collision Simulation, Coefficient of Restitution = %.2f'\n", restitution);

        FILE *kineticEnergyFile = fopen("kinetic_energy.dat", "w");
        int frame_count = 300;

        for (int frame = 0; frame < frame_count; ++frame) {
            handle_collisions(particles, restitution);
            update_positions(particles, restitution);

            // Calculate total kinetic energy
            double totalKineticEnergy = TotalKineticEnergy(particles);
            fprintf(kineticEnergyFile, "%f %f\n", frame * time_step, totalKineticEnergy);

            // Plot particles
            fprintf(gnuplotPipe, "plot ");
            for (size_t i = 0; i < particles.size(); ++i) {
                fprintf(gnuplotPipe, "'-' with circles lc rgb 'red' fill solid 1.0 title 'Particle %zu'%s", i + 1, (i == particles.size() - 1) ? "\n" : ", ");
            }
            for (const auto &particle : particles) {
                fprintf(gnuplotPipe, "%f %f %f\n", particle.x, particle.y, particle.radius);
                fprintf(gnuplotPipe, "e\n");
            }
        }

        fclose(kineticEnergyFile);
        pclose(gnuplotPipe);

        // Plot kinetic energy graph
        gnuplotPipe = popen("gnuplot -persist", "w");
        if (gnuplotPipe) {
            fprintf(gnuplotPipe, "set terminal png size 800,600\n");
            fprintf(gnuplotPipe, "set output 'Kinetic_Energy.png'\n");
            fprintf(gnuplotPipe, "set yrange [0:*]\n");
            fprintf(gnuplotPipe, "set title 'Total Kinetic Energy Over Time, Coefficient of Restitution = %.2f'\n", restitution);
            fprintf(gnuplotPipe, "set xlabel 'Time'\n");
            fprintf(gnuplotPipe, "set ylabel 'Total Kinetic Energy'\n");
            fprintf(gnuplotPipe, "plot 'kinetic_energy.dat' with lines lw 2 title 'Kinetic Energy'\n");
            pclose(gnuplotPipe);
        }
    }

    printf("Simulation complete.");
    return 0;
}
