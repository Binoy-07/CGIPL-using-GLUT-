#include <GL/glut.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <sstream>
#include <string>

// Window dimensions and game area definitions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int GAME_AREA_WIDTH = 700;  // Left side: game area
const int SIDEBAR_WIDTH = WINDOW_WIDTH - GAME_AREA_WIDTH; // 200 pixels

// Sidebar button constants
const int BUTTON_X = GAME_AREA_WIDTH + 10;
const int BUTTON_WIDTH = 90;
const int BUTTON_HEIGHT = 50;
const int PAUSE_BUTTON_Y = 500;
const int RESTART_BUTTON_Y = 430;
const int END_BUTTON_Y = 360;

// Game settings
const float BASE_ASTEROID_SPEED = 50.0f; // Base speed for round 1
const float BULLET_SPEED = 300.0f;
const int MAX_LIVES = 3;

// Round durations (milliseconds)
const int ROUND1_DURATION = 15000; // 15 seconds
const int ROUND2_DURATION = 20000; // 20 seconds
// Round 3: no time limit; speed increases every 5 seconds

// Structure for an asteroid
struct Asteroid {
    float x, y;       // Position
    float size;       // Radius
    float speed;      // Vertical speed
    float r, g, b;    // Color components
};

// Structure for a bullet
struct Bullet {
    float x, y;       // Position
    bool active;      // Active flag
};

std::vector<Asteroid> asteroids;
std::vector<Bullet> bullets;

int score = 0;
int lives = MAX_LIVES;
bool gameOver = false;
int currentRound = 1; // Rounds: 1, 2, or 3

// For showing round titles
bool showRoundTitle = true;
int roundTitleDisplayTime = 3000; // milliseconds to show title
int roundTitleStartTime = 0;      // when current round title started
int round3StartTime = 0;          // when round 3 begins

// Pause state flag
bool paused = false;

// Gun (plane) properties (restricted to game area)
float gunX = GAME_AREA_WIDTH / 2.0f;
float gunY = 50.0f;
// We'll use these dimensions to help design the plane shape

// Timing for asteroid spawn (milliseconds)
int lastSpawnTime = 0;
int spawnInterval = 700; // updated per round

// Current speed applied to newly spawned asteroids
float currentAsteroidSpeed = BASE_ASTEROID_SPEED;

// Helper function to draw text at (x, y)
void drawText(float x, float y, const std::string &text, void *font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}
void drawTextbutton(float x, float y, const std::string &text, void *font = GLUT_BITMAP_HELVETICA_12) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}


// Helper to draw a button with a filled rectangle, border, and centered label
void drawButton(float x, float y, float w, float h, const std::string &label) {
    // Button background
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
    // Button border
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
    // Approximate centering for label (adjust as needed)
    float textWidth = label.size() * 7;
    float textX = x + (w - textWidth) / 2;
    float textY = y + h / 2 - 5;
    drawTextbutton(textX, textY, label);
}

// New function: Draw a 2D plane shape to represent the player's gun.
void drawPlaneGun() {
    // Draw the fuselage as a diamond shape
    glColor3f(0, 1, 0);  // bright green
    glBegin(GL_POLYGON);
        glVertex2f(gunX, gunY + 25);  // Nose
        glVertex2f(gunX - 15, gunY);   // left mid
        glVertex2f(gunX, gunY - 10);   // tail
        glVertex2f(gunX + 15, gunY);   // right mid
    glEnd();

    // Draw the left wing
    glColor3f(0, 0.8, 0);
    glBegin(GL_POLYGON);
        glVertex2f(gunX - 15, gunY + 5);
        glVertex2f(gunX - 35, gunY);
        glVertex2f(gunX - 15, gunY - 5);
    glEnd();

    // Draw the right wing
    glBegin(GL_POLYGON);
        glVertex2f(gunX + 15, gunY + 5);
        glVertex2f(gunX + 35, gunY);
        glVertex2f(gunX + 15, gunY - 5);
    glEnd();
}

// Display callback: draws the game area, asteroids, bullets, HUD, sidebar, and the player's plane gun.
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw game area background
    glColor3f(0, 0, 0);
    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(GAME_AREA_WIDTH, 0);
        glVertex2f(GAME_AREA_WIDTH, WINDOW_HEIGHT);
        glVertex2f(0, WINDOW_HEIGHT);
    glEnd();

    // Draw HUD: Score (top left) and Lives (top right of game area)
    std::stringstream ss;
    ss << "Score: " << score;
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(10, WINDOW_HEIGHT - 20, ss.str());

    float boxWidth = 90, boxHeight = 40;
    float boxX = GAME_AREA_WIDTH - boxWidth - 10;
    float boxY = WINDOW_HEIGHT - boxHeight - 10;
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP);
        glVertex2f(boxX, boxY);
        glVertex2f(boxX + boxWidth, boxY);
        glVertex2f(boxX + boxWidth, boxY + boxHeight);
        glVertex2f(boxX, boxY + boxHeight);
    glEnd();
    std::stringstream ls;
    ls << "Lives: " << lives;
    drawText(boxX + 10, boxY + 15, ls.str());

    // Draw asteroids
    for (const auto &asteroid : asteroids) {
        glColor3f(asteroid.r, asteroid.g, asteroid.b);
        glBegin(GL_POLYGON);
            const int num_segments = 20;
            for (int i = 0; i < num_segments; i++) {
                float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
                float dx = asteroid.size * cosf(theta);
                float dy = asteroid.size * sinf(theta);
                glVertex2f(asteroid.x + dx, asteroid.y + dy);
            }
        glEnd();
    }

    // Draw bullets
    for (const auto &bullet : bullets) {
        if (bullet.active) {
            glColor3f(1, 0, 0);
            glBegin(GL_QUADS);
                glVertex2f(bullet.x - 2, bullet.y);
                glVertex2f(bullet.x + 2, bullet.y);
                glVertex2f(bullet.x + 2, bullet.y + 10);
                glVertex2f(bullet.x - 2, bullet.y + 10);
            glEnd();
        }
    }

    // Draw the player's gun using the new plane shape
    drawPlaneGun();

    // Display round title if within display period
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    if (showRoundTitle && currentTime - roundTitleStartTime < roundTitleDisplayTime) {
        std::string roundTitle = "Round " + std::to_string(currentRound);
        glColor3f(1, 1, 0);
        drawText(GAME_AREA_WIDTH / 2 - 40, WINDOW_HEIGHT / 2, roundTitle, GLUT_BITMAP_TIMES_ROMAN_24);
    } else {
        showRoundTitle = false;
    }

    // If paused (and not game over), show "Paused" overlay in game area
    if (paused && !gameOver) {
        std::string pausedMsg = "Paused";
        glColor3f(1, 1, 1);
        drawText(GAME_AREA_WIDTH / 2 - 30, WINDOW_HEIGHT / 2, pausedMsg, GLUT_BITMAP_TIMES_ROMAN_24);
    }

    // If game over, display the Game Over message in game area
    if (gameOver) {
        std::string msg = "Game Over";
        glColor3f(1, 0, 0);
        drawText(GAME_AREA_WIDTH / 2 - 40, WINDOW_HEIGHT / 2, msg, GLUT_BITMAP_TIMES_ROMAN_24);
    }

    // Draw the sidebar background
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
        glVertex2f(GAME_AREA_WIDTH, 0);
        glVertex2f(WINDOW_WIDTH, 0);
        glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
        glVertex2f(GAME_AREA_WIDTH, WINDOW_HEIGHT);
    glEnd();

    // Draw sidebar buttons
    drawButton(BUTTON_X, PAUSE_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Pause/Resume");
    drawButton(BUTTON_X, RESTART_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Restart Game");
    drawButton(BUTTON_X, END_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "End Game");

    glutSwapBuffers();
}

// Create a new asteroid in the game area (left 600 pixels)
void spawnAsteroid() {
    Asteroid a;
    a.x = rand() % (GAME_AREA_WIDTH - 20) + 10;  // keep some margin in game area
    a.y = WINDOW_HEIGHT + 20;
    a.size = rand() % 30 + 10; // size between 10 and 24
    a.speed = currentAsteroidSpeed;
    a.r = (rand() % 100) / 100.0f;
    a.g = (rand() % 100) / 100.0f;
    a.b = (rand() % 100) / 100.0f;
    asteroids.push_back(a);
}

// Reset game state for restart
void restartGame() {
    score = 0;
    lives = MAX_LIVES;
    gameOver = false;
    currentRound = 1;
    showRoundTitle = true;
    roundTitleStartTime = glutGet(GLUT_ELAPSED_TIME);
    round3StartTime = 0;
    lastSpawnTime = roundTitleStartTime;
    asteroids.clear();
    bullets.clear();
    gunX = GAME_AREA_WIDTH / 2.0f;
    paused = false;
}

// Update callback (roughly 60 FPS) handles game logic, round transitions, and spawning.
void update(int value) {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    static int previousTime = currentTime;

    if (paused) {
        previousTime = currentTime;
        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }

    float dt = (currentTime - previousTime) / 1000.0f;
    previousTime = currentTime;

    if (gameOver) {
        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }

    // Handle round transitions
    if (currentRound == 1 && currentTime - roundTitleStartTime > ROUND1_DURATION) {
        currentRound = 2;
        showRoundTitle = true;
        roundTitleStartTime = currentTime;
    } else if (currentRound == 2 && currentTime - roundTitleStartTime > ROUND2_DURATION) {
        currentRound = 3;
        showRoundTitle = true;
        roundTitleStartTime = currentTime;
        round3StartTime = currentTime;
    }

    // Adjust asteroid speed and spawn frequency based on round
    if (currentRound == 1) {
        currentAsteroidSpeed = BASE_ASTEROID_SPEED;
        spawnInterval = 1000; // 1 second
    } else if (currentRound == 2) {
        currentAsteroidSpeed = BASE_ASTEROID_SPEED * 2;
        spawnInterval = 500;
    } else if (currentRound == 3) {
        int timeSinceRound3 = currentTime - round3StartTime;
        int increments = timeSinceRound3 / 5000; // every 5 seconds
        currentAsteroidSpeed = BASE_ASTEROID_SPEED * 6 + increments * 50;
        spawnInterval = 300;
    }

    // Spawn asteroids based on round (increasing number each round)
    if (currentTime - lastSpawnTime > spawnInterval) {
        int asteroidsToSpawn = (currentRound == 1) ? 1 : (currentRound == 2) ? 2 : 3;
        for (int i = 0; i < asteroidsToSpawn; ++i) {
            spawnAsteroid();
        }
        lastSpawnTime = currentTime;
    }

    // Update asteroids and check collisions with the gun
    for (size_t i = 0; i < asteroids.size(); i++) {
        asteroids[i].y -= asteroids[i].speed * dt;
        if (asteroids[i].y - asteroids[i].size < gunY + 25 &&   // 25 is nose height of plane
            asteroids[i].x > gunX - 35 &&                       // considering wing span
            asteroids[i].x < gunX + 35) {
            lives--;
            asteroids.erase(asteroids.begin() + i);
            i--;
            if (lives <= 0) {
                gameOver = true;
            }
            continue;
        }
        if (asteroids[i].y + asteroids[i].size < 0) {
            asteroids.erase(asteroids.begin() + i);
            i--;
        }
    }

    // Update bullets
    for (size_t i = 0; i < bullets.size(); i++) {
        if (bullets[i].active) {
            bullets[i].y += BULLET_SPEED * dt;
            if (bullets[i].y > WINDOW_HEIGHT) {
                bullets[i].active = false;
            }
        }
    }

    // Check collisions between bullets and asteroids
    for (size_t i = 0; i < bullets.size(); i++) {
        if (!bullets[i].active)
            continue;
        for (size_t j = 0; j < asteroids.size(); j++) {
            float dx = bullets[i].x - asteroids[j].x;
            float dy = bullets[i].y - asteroids[j].y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < asteroids[j].size) {
                score += 10;
                bullets[i].active = false;
                asteroids.erase(asteroids.begin() + j);
                break;
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// Keyboard callback: fire a bullet when space is pressed.
void keyboard(unsigned char key, int x, int y) {
    if (gameOver)
        return;
    if (key == ' ') {
        Bullet b;
        b.x = gunX;
        b.y = gunY + 25;  // bullet starts at the nose of the plane
        b.active = true;
        bullets.push_back(b);
    }
}

// Special keys callback: move the gun left/right (restricted to game area)
void specialKeys(int key, int x, int y) {
    if (gameOver)
        return;
    const float moveSpeed = 10.0f;
    if (key == GLUT_KEY_LEFT) {
        gunX -= moveSpeed;
        if (gunX - 35 < 0)   // consider wing width
            gunX = 35;
    } else if (key == GLUT_KEY_RIGHT) {
        gunX += moveSpeed;
        if (gunX + 35 > GAME_AREA_WIDTH)
            gunX = GAME_AREA_WIDTH - 35;
    }
}

// Mouse callback: detect clicks in the sidebar and check for button activation.
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Convert y coordinate from window (top-left origin) to OpenGL (bottom-left origin)
        int oglY = WINDOW_HEIGHT - y;
        if (x >= GAME_AREA_WIDTH) {
            // Pause/Resume button
            if (x >= BUTTON_X && x <= BUTTON_X + BUTTON_WIDTH &&
                oglY >= PAUSE_BUTTON_Y && oglY <= PAUSE_BUTTON_Y + BUTTON_HEIGHT) {
                paused = !paused;
                return;
            }
            // Restart Game button
            if (x >= BUTTON_X && x <= BUTTON_X + BUTTON_WIDTH &&
                oglY >= RESTART_BUTTON_Y && oglY <= RESTART_BUTTON_Y + BUTTON_HEIGHT) {
                restartGame();
                return;
            }
            // End Game button
            if (x >= BUTTON_X && x <= BUTTON_X + BUTTON_WIDTH &&
                oglY >= END_BUTTON_Y && oglY <= END_BUTTON_Y + BUTTON_HEIGHT) {
                exit(0);
            }
        }
    }
}

// Reshape callback: force a fixed window size and set up a 2D orthographic projection.
void reshape(int w, int h) {
    glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    srand(time(0));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Asteroid Shooting Game");

    glClearColor(0, 0, 0, 1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);

    roundTitleStartTime = glutGet(GLUT_ELAPSED_TIME);
    lastSpawnTime = roundTitleStartTime;
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}
