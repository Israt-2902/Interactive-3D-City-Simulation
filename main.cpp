
#include <GL/freeglut.h>
#include <cstdlib>
#include <ctime>
#include <cmath>

// ==========================================
// GLOBAL SETTINGS
// ==========================================

int cameraMode = 0;       // 0 = Isometric, 1 = First Person
int nightMode = 0;        // 0 = Day, 1 = Night
int weatherMode = 0;      // 0 = Clear, 1 = Rain, 2 = Snow
int trafficSignal = 0;    // 0 = Red, 1 = Yellow, 2 = Green

float vehicleSpeed = 0.12f;
float animationTime = 0.0f;
float cloudOffset = 0.0f;
float birdOffset = 0.0f;

const int PARTICLE_COUNT = 350;

struct Particle
{
    float x, y, z;
    float speed;
};

struct Vehicle
{
    float x, z;
    int direction;
    float r, g, b;
};

Particle particles[PARTICLE_COUNT];

// Vehicles move along four road directions
Vehicle cars[] =
{
    {-4, -45, 0, 0.9f, 0.1f, 0.1f},
    { 4,  45, 1, 0.1f, 0.3f, 0.9f},
    {-45, -4, 2, 0.9f, 0.8f, 0.1f},
    { 45,  4, 3, 0.1f, 0.8f, 0.3f},
    {-4, -25, 0, 0.9f, 0.9f, 0.9f},
    { 4,  25, 1, 0.9f, 0.4f, 0.1f}
};

const int CAR_COUNT = sizeof(cars) / sizeof(cars[0]);

// ==========================================
// BASIC 3D OBJECT FUNCTION
// ==========================================

void drawBox(float x, float y, float z,
             float sx, float sy, float sz,
             float r, float g, float b)
{
    glPushMatrix();

    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);

    glColor3f(r, g, b);
    glutSolidCube(1.0f);

    glPopMatrix();
}

// ==========================================
// GROUND AND ROADS
// ==========================================

void drawGround()
{
    glColor3f(0.20f, 0.40f, 0.17f);

    glNormal3f(0, 1, 0);

    glBegin(GL_QUADS);

    glVertex3f(-60, 0, -60);
    glVertex3f( 60, 0, -60);
    glVertex3f( 60, 0,  60);
    glVertex3f(-60, 0,  60);

    glEnd();
}

void drawRoads()
{
    drawBox(0, 0.10f, 0, 16, 0.10f, 110,
            0.13f, 0.14f, 0.16f);

    drawBox(0, 0.10f, 0, 110, 0.10f, 16,
            0.13f, 0.14f, 0.16f);

    // Sidewalks
    drawBox(-9.1f, 0.12f, 0, 2.2f, 0.25f, 110,
            0.65f, 0.65f, 0.63f);

    drawBox(9.1f, 0.12f, 0, 2.2f, 0.25f, 110,
            0.65f, 0.65f, 0.63f);

    drawBox(0, 0.12f, -9.1f, 110, 0.25f, 2.2f,
            0.65f, 0.65f, 0.63f);

    drawBox(0, 0.12f, 9.1f, 110, 0.25f, 2.2f,
            0.65f, 0.65f, 0.63f);

    // Curbs
    drawBox(-8.05f, 0.22f, 0, 0.25f, 0.35f, 110,
            0.9f, 0.9f, 0.85f);

    drawBox(8.05f, 0.22f, 0, 0.25f, 0.35f, 110,
            0.9f, 0.9f, 0.85f);

    drawBox(0, 0.22f, -8.05f, 110, 0.35f, 0.25f,
            0.9f, 0.9f, 0.85f);

    drawBox(0, 0.22f, 8.05f, 110, 0.35f, 0.25f,
            0.9f, 0.9f, 0.85f);
}

void drawRoadMarkings()
{
    // Double yellow center lines
    drawBox(-0.18f, 0.18f, 0, 0.12f, 0.02f, 110,
            1.0f, 0.8f, 0.0f);

    drawBox(0.18f, 0.18f, 0, 0.12f, 0.02f, 110,
            1.0f, 0.8f, 0.0f);

    drawBox(0, 0.18f, -0.18f, 110, 0.02f, 0.12f,
            1.0f, 0.8f, 0.0f);

    drawBox(0, 0.18f, 0.18f, 110, 0.02f, 0.12f,
            1.0f, 0.8f, 0.0f);

    // White dashed lane markings
    for (int i = -50; i <= 50; i += 8)
    {
        if (i > -15 && i < 15)
            continue;

        drawBox(-4, 0.19f, i, 0.12f, 0.02f, 4,
                1, 1, 1);

        drawBox(4, 0.19f, i, 0.12f, 0.02f, 4,
                1, 1, 1);

        drawBox(i, 0.19f, -4, 4, 0.02f, 0.12f,
                1, 1, 1);

        drawBox(i, 0.19f, 4, 4, 0.02f, 0.12f,
                1, 1, 1);
    }
}

void drawCrosswalks()
{
    for (int i = -6; i <= 6; i += 2)
    {
        drawBox(i, 0.20f, -12.5f, 0.9f, 0.02f, 1,
                1, 1, 1);

        drawBox(i, 0.20f, 12.5f, 0.9f, 0.02f, 1,
                1, 1, 1);

        drawBox(-12.5f, 0.20f, i, 1, 0.02f, 0.9f,
                1, 1, 1);

        drawBox(12.5f, 0.20f, i, 1, 0.02f, 0.9f,
                1, 1, 1);
    }
}

// ==========================================
// BUILDINGS
// ==========================================

void drawBuilding(float x, float z,
                  float w, float d, float h,
                  float r, float g, float b)
{
    // Main building
    drawBox(x, h / 2, z, w, h, d, r, g, b);

    // Roof border
    drawBox(x, h + 0.25f, z,
            w + 0.4f, 0.5f, d + 0.4f,
            r * 0.65f, g * 0.65f, b * 0.65f);

    // Front windows
    for (float y = 2.0f; y < h - 1; y += 3.0f)
    {
        for (float xx = -w / 2 + 1.5f;
             xx < w / 2 - 0.5f; xx += 2.5f)
        {
            drawBox(x + xx, y, z + d / 2 + 0.06f,
                    1.15f, 1.1f, 0.12f,
                    0.25f, 0.65f, 0.85f);
        }

        // Side windows
        for (float zz = -d / 2 + 1.5f;
             zz < d / 2 - 0.5f; zz += 2.5f)
        {
            drawBox(x + w / 2 + 0.06f, y, z + zz,
                    0.12f, 1.1f, 1.15f,
                    0.25f, 0.65f, 0.85f);
        }
    }

    // Main entrance
    drawBox(x, 1.2f, z + d / 2 + 0.08f,
            1.5f, 2.4f, 0.15f,
            0.25f, 0.13f, 0.08f);

    // Roof equipment
    drawBox(x, h + 0.9f, z,
            2.0f, 0.8f, 2.0f,
            0.5f, 0.5f, 0.52f);
}

void drawBuildings()
{
    // Different buildings around the city
    drawBuilding(-22, -23, 10, 10, 14,
                 0.75f, 0.55f, 0.35f);

    drawBuilding(-40, -25, 11, 10, 23,
                 0.65f, 0.70f, 0.76f);

    drawBuilding(-23, -42, 12, 10, 28,
                 0.75f, 0.75f, 0.80f);

    drawBuilding(22, -23, 11, 10, 19,
                 0.80f, 0.65f, 0.45f);

    drawBuilding(40, -25, 12, 10, 26,
                 0.55f, 0.65f, 0.80f);

    drawBuilding(23, -42, 11, 10, 15,
                 0.75f, 0.55f, 0.45f);

    drawBuilding(-23, 23, 11, 10, 21,
                 0.70f, 0.75f, 0.65f);

    drawBuilding(23, 23, 12, 10, 17,
                 0.65f, 0.65f, 0.80f);
}

// ==========================================
// TREES
// ==========================================

void drawFlower(float x, float y, float z, float size)
{
    for (int i = 0; i < 5; i++)
    {
        float a = i * 72.0f * 3.14159f / 180.0f;
        glPushMatrix();
        glTranslatef(x + cos(a) * size * 0.65f,
                     y + sin(a) * size * 0.65f, z);
        glColor3f(1.0f, 0.35f, 0.65f);
        glutSolidSphere(size * 0.48f, 8, 8);
        glPopMatrix();
    }
    glPushMatrix();
    glTranslatef(x, y, z + 0.08f);
    glColor3f(1.0f, 0.85f, 0.05f);
    glutSolidSphere(size * 0.32f, 8, 8);
    glPopMatrix();
}

void drawTree(float x, float z)
{
    drawBox(x, 1.5f, z, 0.6f, 3.0f, 0.6f,
            0.35f, 0.17f, 0.06f);

    float sway = 5.0f * sin(animationTime * 1.8f + x * 0.2f + z * 0.15f);
    glPushMatrix();
    glTranslatef(x, 3.0f, z);
    glRotatef(sway, 0, 0, 1);

    glPushMatrix();
    glTranslatef(0, 1.0f, 0);
    glColor3f(0.08f, 0.40f, 0.12f);
    glutSolidSphere(2.0, 16, 16);
    glTranslatef(0, 1.1f, 0);
    glColor3f(0.12f, 0.50f, 0.16f);
    glutSolidSphere(1.5, 16, 16);
    glPopMatrix();

    drawFlower(-1.0f, 1.4f, 1.55f, 0.42f);
    drawFlower(1.0f, 1.8f, 1.25f, 0.38f);
    drawFlower(0.0f, 2.6f, 1.0f, 0.40f);
    drawFlower(-1.0f, 3.2f, 0.7f, 0.35f);
    drawFlower(1.0f, 3.4f, 0.5f, 0.38f);
    glPopMatrix();
}

void drawTrees()
{
    for (int z = -45; z <= 45; z += 15)
    {
        drawTree(-11.5f, z);
        drawTree(11.5f, z);
    }

    for (int x = -45; x <= 45; x += 15)
    {
        drawTree(x, -11.5f);
        drawTree(x, 11.5f);
    }
}

// ==========================================
// WALKING PEOPLE
// ==========================================

void drawPerson(float x, float z, float phase, float r, float g, float b)
{
    float step = sin(animationTime * 5.0f + phase) * 25.0f;
    glPushMatrix();
    glTranslatef(x, 0, z);

    glPushMatrix();
    glTranslatef(0, 2.25f, 0);
    glColor3f(0.88f, 0.68f, 0.52f);
    glutSolidSphere(0.32f, 12, 12);
    glPopMatrix();

    drawBox(0, 1.35f, 0, 0.72f, 1.05f, 0.42f, r, g, b);

    glPushMatrix();
    glTranslatef(-0.48f, 1.65f, 0);
    glRotatef(-step, 1, 0, 0);
    drawBox(0, -0.38f, 0, 0.20f, 0.85f, 0.22f, 0.88f, 0.68f, 0.52f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.48f, 1.65f, 0);
    glRotatef(step, 1, 0, 0);
    drawBox(0, -0.38f, 0, 0.20f, 0.85f, 0.22f, 0.88f, 0.68f, 0.52f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.20f, 0.82f, 0);
    glRotatef(step, 1, 0, 0);
    drawBox(0, -0.38f, 0, 0.24f, 0.85f, 0.28f, 0.12f, 0.16f, 0.25f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.20f, 0.82f, 0);
    glRotatef(-step, 1, 0, 0);
    drawBox(0, -0.38f, 0, 0.24f, 0.85f, 0.28f, 0.12f, 0.16f, 0.25f);
    glPopMatrix();

    glPopMatrix();
}

void drawPeople()
{
    drawPerson(-9.1f, -30.0f, 0.0f, 0.85f, 0.15f, 0.12f);
    drawPerson(-9.1f, -20.0f, 1.5f, 0.15f, 0.35f, 0.85f);
    drawPerson(9.1f, 25.0f, 2.5f, 0.90f, 0.55f, 0.10f);
    drawPerson(9.1f, 38.0f, 0.8f, 0.20f, 0.65f, 0.30f);
    drawPerson(-30.0f, 9.1f, 1.0f, 0.65f, 0.20f, 0.65f);
    drawPerson(30.0f, -9.1f, 2.0f, 0.10f, 0.65f, 0.70f);
}

// ==========================================
// MOVING CLOUDS
// ==========================================

void drawCloud(float x, float y, float z, float s)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(s, s, s);
    glColor3f(0.96f, 0.98f, 1.0f);
    glutSolidSphere(2.0f, 12, 12);
    glTranslatef(2.0f, 0.3f, 0);
    glutSolidSphere(1.6f, 12, 12);
    glTranslatef(-4.0f, -0.2f, 0);
    glutSolidSphere(1.5f, 12, 12);
    glTranslatef(2.0f, 1.0f, 0);
    glutSolidSphere(1.7f, 12, 12);
    glPopMatrix();
}

void drawClouds()
{
    if (nightMode) return;
    glDisable(GL_LIGHTING);
    drawCloud(-35.0f + cloudOffset, 38.0f, -25.0f, 1.8f);
    drawCloud(5.0f + cloudOffset, 42.0f, -40.0f, 1.5f);
    drawCloud(-20.0f + cloudOffset, 35.0f, 35.0f, 1.3f);
    glEnable(GL_LIGHTING);
}

// ==========================================
// DAYTIME FLYING BIRDS
// ==========================================

void drawBird(float x, float y, float z, float phase)
{
    float wing = sin(animationTime * 8.0f + phase) * 0.65f;
    glPushMatrix();
    glTranslatef(x, y, z);
    glColor3f(0.12f, 0.12f, 0.15f);
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(-0.65f, wing, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0.65f, wing, 0);
    glEnd();
    glPopMatrix();
}

void drawBirds()
{
    if (nightMode) return;
    glDisable(GL_LIGHTING);
    for (int i = 0; i < 5; i++)
        drawBird(-35.0f + birdOffset + i * 2.0f,
                 30.0f + (i % 2) * 1.5f,
                 -5.0f - i * 1.2f, i * 0.7f);
    glEnable(GL_LIGHTING);
}

// ==========================================
// STREETLIGHTS
// ==========================================

void drawStreetlight(float x, float z)
{
    // Pole
    drawBox(x, 3.5f, z, 0.25f, 7.0f, 0.25f,
            0.25f, 0.25f, 0.28f);

    // Lamp arm
    drawBox(x + 0.7f, 6.8f, z,
            1.5f, 0.18f, 0.18f,
            0.25f, 0.25f, 0.28f);

    // Lamp
    drawBox(x + 1.35f, 6.65f, z,
            0.8f, 0.25f, 0.7f,
            nightMode ? 1.0f : 0.8f,
            nightMode ? 0.9f : 0.8f,
            nightMode ? 0.3f : 0.7f);
}

void drawStreetlights()
{
    for (int z = -40; z <= 40; z += 20)
    {
        drawStreetlight(-10.5f, z);
        drawStreetlight(10.5f, z);
    }

    for (int x = -40; x <= 40; x += 20)
    {
        drawStreetlight(x, -10.5f);
        drawStreetlight(x, 10.5f);
    }
}

// ==========================================
// TRAFFIC SIGNALS
// ==========================================

void drawTrafficLight(float x, float z)
{
    // Pole
    drawBox(x, 3.0f, z, 0.3f, 6.0f, 0.3f,
            0.2f, 0.2f, 0.2f);

    // Signal housing
    drawBox(x, 6.5f, z, 1.0f, 2.5f, 0.7f,
            0.12f, 0.12f, 0.12f);

    // Red light
    glPushMatrix();
    glTranslatef(x, 7.2f, z + 0.4f);
    glColor3f(trafficSignal == 0 ? 1.0f : 0.22f, 0, 0);
    glutSolidSphere(0.27, 12, 12);
    glPopMatrix();

    // Yellow light
    glPushMatrix();
    glTranslatef(x, 6.5f, z + 0.4f);
    glColor3f(trafficSignal == 1 ? 1.0f : 0.22f,
              trafficSignal == 1 ? 0.8f : 0.20f, 0);
    glutSolidSphere(0.27, 12, 12);
    glPopMatrix();

    // Green light
    glPushMatrix();
    glTranslatef(x, 5.8f, z + 0.4f);
    glColor3f(0, trafficSignal == 2 ? 1.0f : 0.22f, 0);
    glutSolidSphere(0.27, 12, 12);
    glPopMatrix();
}

void drawTrafficLights()
{
    drawTrafficLight(-10, -10);
    drawTrafficLight(10, -10);
    drawTrafficLight(-10, 10);
    drawTrafficLight(10, 10);
}

// ==========================================
// VEHICLES
// ==========================================

void drawCar(Vehicle car)
{
    glPushMatrix();

    glTranslatef(car.x, 0, car.z);

    if (car.direction >= 2)
        glRotatef(90, 0, 1, 0);

    // Main body
    drawBox(0, 0.75f, 0,
            1.5f, 0.75f, 2.7f,
            car.r, car.g, car.b);

    // Cabin
    drawBox(0, 1.35f, -0.1f,
            1.2f, 0.65f, 1.35f,
            0.35f, 0.65f, 0.85f);

    // Roof
    drawBox(0, 1.72f, -0.1f,
            1.15f, 0.08f, 1.3f,
            car.r * 0.8f, car.g * 0.8f, car.b * 0.8f);

    // Wheels
    for (int i = -1; i <= 1; i += 2)
    {
        for (int j = -1; j <= 1; j += 2)
        {
            drawBox(i * 0.78f, 0.42f, j * 0.85f,
                    0.28f, 0.45f, 0.48f,
                    0.04f, 0.04f, 0.04f);
        }
    }

    // Headlights
    drawBox(-0.45f, 0.75f, 1.37f,
            0.25f, 0.22f, 0.08f, 1, 1, 0.8f);

    drawBox(0.45f, 0.75f, 1.37f,
            0.25f, 0.22f, 0.08f, 1, 1, 0.8f);

    glPopMatrix();
}

void drawVehicles()
{
    for (int i = 0; i < CAR_COUNT; i++)
        drawCar(cars[i]);
}

void updateVehicles()
{
    // Red light stops vehicles.
    // Yellow slows them down.
    float speed = vehicleSpeed;

    if (trafficSignal == 0)
        speed = 0.0f;

    if (trafficSignal == 1)
        speed *= 0.3f;

    for (int i = 0; i < CAR_COUNT; i++)
    {
        if (cars[i].direction == 0)
        {
            cars[i].z += speed;
            if (cars[i].z > 55) cars[i].z = -55;
        }
        else if (cars[i].direction == 1)
        {
            cars[i].z -= speed;
            if (cars[i].z < -55) cars[i].z = 55;
        }
        else if (cars[i].direction == 2)
        {
            cars[i].x += speed;
            if (cars[i].x > 55) cars[i].x = -55;
        }
        else
        {
            cars[i].x -= speed;
            if (cars[i].x < -55) cars[i].x = 55;
        }
    }
}

// ==========================================
// WEATHER PARTICLE SYSTEM
// ==========================================

void initializeParticles()
{
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        particles[i].x = (rand() % 110) - 55;
        particles[i].y = rand() % 35 + 5;
        particles[i].z = (rand() % 110) - 55;

        particles[i].speed = 0.15f + (rand() % 100) / 250.0f;
    }
}

void updateWeather()
{
    if (weatherMode == 0)
        return;

    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        particles[i].y -= particles[i].speed;

        if (weatherMode == 1)
            particles[i].x += 0.03f;

        if (particles[i].y < 0)
        {
            particles[i].y = 35;
            particles[i].x = (rand() % 110) - 55;
            particles[i].z = (rand() % 110) - 55;
        }
    }
}

void drawWeather()
{
    if (weatherMode == 0)
        return;

    glDisable(GL_LIGHTING);
    glColor3f(0.85f, 0.9f, 1.0f);

    if (weatherMode == 1)
    {
        // Rain particles
        glBegin(GL_LINES);

        for (int i = 0; i < PARTICLE_COUNT; i++)
        {
            glVertex3f(particles[i].x,
                       particles[i].y,
                       particles[i].z);

            glVertex3f(particles[i].x - 0.1f,
                       particles[i].y - 0.8f,
                       particles[i].z);
        }

        glEnd();
    }
    else
    {
        // Snow particles
        glPointSize(4);

        glBegin(GL_POINTS);

        for (int i = 0; i < PARTICLE_COUNT; i++)
        {
            glVertex3f(particles[i].x,
                       particles[i].y,
                       particles[i].z);
        }

        glEnd();
    }

    glEnable(GL_LIGHTING);
}

// ==========================================
// DAY AND NIGHT LIGHTING
// ==========================================

void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat position[] = {-30, 60, 40, 0};

    GLfloat ambientDay[] = {0.55f, 0.55f, 0.55f, 1};
    GLfloat ambientNight[] = {0.16f, 0.16f, 0.25f, 1};

    GLfloat diffuseDay[] = {1, 1, 0.95f, 1};
    GLfloat diffuseNight[] = {0.35f, 0.35f, 0.55f, 1};

    glLightfv(GL_LIGHT0, GL_POSITION, position);

    if (nightMode == 0)
    {
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientDay);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseDay);

        glClearColor(0.53f, 0.75f, 0.95f, 1);
    }
    else
    {
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientNight);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseNight);

        glClearColor(0.015f, 0.02f, 0.09f, 1);
    }
}

// ==========================================
// COMPLETE CITY
// ==========================================

void drawCity()
{
    drawGround();
    drawRoads();
    drawRoadMarkings();
    drawCrosswalks();

    drawBuildings();
    drawTrees();
    drawPeople();
    drawClouds();
    drawBirds();
    drawStreetlights();

    drawTrafficLights();
    drawVehicles();
    drawWeather();
}

// ==========================================
// CAMERA SYSTEM
// ==========================================

void setCamera()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (cameraMode == 0)
    {
        // Isometric camera
        gluLookAt(
            55, 60, 65,
            0, 0, 0,
            0, 1, 0
        );
    }
    else
    {
        // First-person camera
        gluLookAt(
            0, 2.5f, 35,
            0, 2, -35,
            0, 1, 0
        );
    }
}

// ==========================================
// INFORMATION DISPLAY
// ==========================================

void drawText(float x, float y, const char* text)
{
    glRasterPos2f(x, y);

    while (*text)
    {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *text);
        text++;
    }
}

void drawHUD()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 1);

    drawText(15, h - 25, "3D CITY SIMULATION");

    drawText(15, h - 45,
             "1: Isometric   2: First Person");

    drawText(15, h - 65,
             "N: Night   D: Day   B: Rain   P: Snow");

    drawText(15, h - 85,
             "R: Red   Y: Yellow   G: Green");

    drawText(15, h - 105,
             "W: Increase Speed   S: Decrease Speed");

    if (trafficSignal == 0)
        drawText(15, h - 125, "Traffic: RED");
    else if (trafficSignal == 1)
        drawText(15, h - 125, "Traffic: YELLOW");
    else
        drawText(15, h - 125, "Traffic: GREEN");

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// ==========================================
// DISPLAY FUNCTION
// ==========================================

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupLighting();
    setCamera();

    drawCity();
    drawHUD();

    glutSwapBuffers();
}

// ==========================================
// ANIMATION TIMER
// ==========================================

void update(int value)
{
    animationTime += 0.016f;
    cloudOffset += 0.025f;
    birdOffset += 0.10f;
    if (cloudOffset > 85.0f) cloudOffset = -85.0f;
    if (birdOffset > 100.0f) birdOffset = -25.0f;

    updateVehicles();
    updateWeather();

    glutPostRedisplay();

    glutTimerFunc(16, update, 0);
}

// ==========================================
// WINDOW RESIZING
// ==========================================

void reshape(int w, int h)
{
    if (h == 0) h = 1;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        60.0,
        (float)w / h,
        0.1,
        200.0
    );

    glMatrixMode(GL_MODELVIEW);
}

// ==========================================
// KEYBOARD CONTROLS
// ==========================================

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case '1':
            cameraMode = 0;
            break;

        case '2':
            cameraMode = 1;
            break;

        case 'N':
        case 'n':
            nightMode = 1;
            break;

        case 'D':
        case 'd':
            nightMode = 0;
            break;

        case 'B':
        case 'b':
            weatherMode = 1;
            break;

        case 'P':
        case 'p':
            weatherMode = 2;
            break;

        case 'C':
        case 'c':
            weatherMode = 0;
            break;

        case 'R':
        case 'r':
            trafficSignal = 0;
            break;

        case 'Y':
        case 'y':
            trafficSignal = 1;
            break;

        case 'G':
        case 'g':
            trafficSignal = 2;
            break;

        case 'W':
        case 'w':
            vehicleSpeed += 0.05f;
            if (vehicleSpeed > 1.0f)
                vehicleSpeed = 1.0f;
            break;

        case 'S':
        case 's':
            vehicleSpeed -= 0.05f;
            if (vehicleSpeed < 0.0f)
                vehicleSpeed = 0.0f;
            break;

        case 27:
            exit(0);
    }

    glutPostRedisplay();
}

// ==========================================
// MAIN FUNCTION
// ==========================================

int main(int argc, char** argv)
{
    srand((unsigned int)time(0));

    initializeParticles();

    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(1200, 800);
    glutInitWindowPosition(50, 30);

    glutCreateWindow("Interactive 3D City Simulation");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutTimerFunc(16, update, 0);

    glutMainLoop();

    return 0;
}
