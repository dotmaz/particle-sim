#include <GL/glut.h>
#include <stdbool.h>
#include <stdio.h>

#define WINDOW_SIZE 800
// #define GRID_SIZE 800
// #define CELL_SIZE .0025f
// #define GRID_SIZE 400
// #define CELL_SIZE .005f
#define GRID_SIZE 200
#define CELL_SIZE .01f
// #define GRID_SIZE 100
// #define CELL_SIZE .02f
#define UPDATE_RATE 15 // in milliseconds

const int TYPE_SIZE = 7;

typedef enum { AIR, SAND, WATER, ROCK, WOOD, LEAF, FIRE } CellType;
typedef enum { SOLID, LIQUID, STATIC } CellMaterial;
typedef enum { BASIC_PLANT } PlantDNAType;

typedef struct {
  CellType type;
  CellMaterial material;
  int age;
  int treeAge;
  float hueOffset;

  PlantDNAType plantDNAType;
} Cell;

typedef struct {
  // Wood growth & leaf birth parameters
  double woodMaxAge;
  double woodMaxTreeAge;
  double woodGrowthUp;
  double woodGrowthHorizontal;
  double woodGrowthDown;
  double woodLeafGrowth;
  // Leaf growth parameters
  double leafMaxAge;
  double leadMaxTreeAge;
  double leafGrowthRate;
  // Leaf color
  float color[3];
} PlantDNA;

PlantDNA plantDNAs[] = {
    {20,
     20,
     .02,
     .005,
     .002,
     .001,
     70,
     5,
     .01,
     {0.168f, 0.51f, 0.165f}}, // Properties for BASIC PLANT
};

typedef struct {
  float density;
  bool isFluid;
  float color[3]; // RGB color
} CellProperties;

CellProperties cellProperties[] = {
    {0.0,
     false,
     {0.0f, 0.0f, 0.0f}}, // Properties for AIR (white or transparent)
    {1.6, false, {0.8f, 0.6f, 0.2f}},      // Properties for SAND
    {1.0, true, {0.0f, 0.0f, 1.0f}},       // Properties for WATER
    {2.5, false, {0.5f, 0.5f, 0.5f}},      // Properties for ROCK
    {2.5, false, {0.36f, 0.27f, 0.08f}},   // Properties for WOOD
    {1.2, false, {0.168f, 0.51f, 0.165f}}, // Properties for LEAF
    {0.5, false, {0.812f, 0.098f, 0.098f}} // Properties for FIRE
};

Cell grid[GRID_SIZE][GRID_SIZE];
Cell nextGrid[GRID_SIZE][GRID_SIZE];

CellType currentElement = WOOD; // Start with SAND as the current element

int strokeSize = 0; // Default stroke size
const int minStrokeSize = 0;
const int maxStrokeSize = 20;
bool maz = true;

void initGrid() {
  for (int x = 0; x < GRID_SIZE; x++) {
    for (int y = 0; y < GRID_SIZE; y++) {
      grid[x][y].type = AIR;
      grid[x][y].age = 0;
      grid[x][y].treeAge = 0;
      grid[x][y].hueOffset = 0.0f;
      nextGrid[x][y].type = AIR;
      nextGrid[x][y].age = 0;
      nextGrid[x][y].treeAge = 0;
      nextGrid[x][y].hueOffset = 0.0f;
    }
  }
}

bool isPaused = false; // Tracks if the simulation is paused

void applyPhysics() {
  // Copy current grid to nextGrid
  for (int x = 0; x < GRID_SIZE; x++) {
    for (int y = 0; y < GRID_SIZE; y++) {
      nextGrid[x][y] = grid[x][y];
    }
  }

  void performUpdates(int x, int y) {
    if (grid[x][y].type == SAND || grid[x][y].type == WATER) {
      bool canMoveDown = y > 0 && nextGrid[x][y - 1].type == AIR;
      bool canMoveBottomRight =
          x < GRID_SIZE - 1 && y > 0 && nextGrid[x + 1][y - 1].type == AIR;
      bool canMoveBottomLeft =
          x > 0 && y > 0 && nextGrid[x - 1][y - 1].type == AIR;
      bool canMoveRight = grid[x][y].type == WATER && x < GRID_SIZE - 1 &&
                          nextGrid[x + 1][y].type == AIR;
      bool canMoveLeft =
          grid[x][y].type == WATER && x > 0 && nextGrid[x - 1][y].type == AIR;

      if (canMoveDown) {
        nextGrid[x][y].type = AIR;
        nextGrid[x][y - 1].type = grid[x][y].type;
      } else if (canMoveBottomLeft) {
        nextGrid[x][y].type = AIR;
        nextGrid[x - 1][y - 1].type = grid[x][y].type;
      } else if (canMoveBottomRight) {
        nextGrid[x][y].type = AIR;
        nextGrid[x + 1][y - 1].type = grid[x][y].type;
      } else if (canMoveLeft) {
        nextGrid[x][y].type = AIR;
        nextGrid[x - 1][y].type = grid[x][y].type;
      } else if (canMoveRight) {
        nextGrid[x][y].type = AIR;
        nextGrid[x + 1][y].type = grid[x][y].type;
      }
    }

    const double woodMaxAge = 20;
    const double woodMaxTreeAge = 20;
    const double woodGrowthUp = .02;
    const double woodGrowthHorizontal = .005;
    const double woodGrowthDown = .002;
    const double woodLeafGrowth = .001;

    if (grid[x][y].type == WOOD) {
      if (grid[x][y].age < woodMaxAge && grid[x][y].treeAge < woodMaxTreeAge) {
        for (int i = -1; i <= 1; i++) {
          for (int j = -1; j <= 1; j++) {
            if ((!i && !j) || x + i < 0 || x + i > GRID_SIZE - 1 || y + j < 0 ||
                y + j > GRID_SIZE - 1) {
              continue;
            } // Ignore out of bounds
            if (nextGrid[x + i][y + j].type != AIR)
              continue; // Only grow into empty space
            double randNum = ((double)rand() / (double)RAND_MAX);
            double randStat;
            // Probability tree for upward bias
            if (j == 1) {
              randStat = woodGrowthUp;
            } else if (j == 0) {
              randStat = woodGrowthHorizontal;
            } else {
              randStat = woodGrowthDown;
            }
            if (randNum < randStat) {

              nextGrid[x + i][y + j].type = WOOD;
              nextGrid[x + i][y + j].treeAge = grid[x + i][y + j].treeAge + 1;
              nextGrid[x + i][y + j].age = 0;
              nextGrid[x + i][y + j].hueOffset =
                  (float)((double)rand() / (double)RAND_MAX) * .2f - .1f;
            }

            double leafRand = ((double)rand() / (double)RAND_MAX);
            if (leafRand < woodLeafGrowth) {
              nextGrid[x + i][y + j].type = LEAF;
              nextGrid[x + i][y + j].treeAge = 0;
              nextGrid[x + i][y + j].age = 0;
            }
          }
        }
      }
    }

    const double leafMaxAge = 70;
    const double leadMaxTreeAge = 5;
    const double leafGrowthRate = .01;

    if (grid[x][y].type == LEAF) {
      // First value decides how sparse the tree will be (max age before leaf
      // stops spreading) Second value decides how far the leaves can
      // potentially spread (max tree age before leaf stops spreading)
      if (grid[x][y].age < leafMaxAge && grid[x][y].treeAge < leadMaxTreeAge) {
        for (int i = -1; i <= 1; i++) {
          for (int j = -1; j <= 1; j++) {
            if ((!i && !j) || x + i < 0 || x + i > GRID_SIZE - 1 || y + j < 0 ||
                y + j > GRID_SIZE - 1) {
              continue;
            } // Ignore out of bounds
            if (nextGrid[x + i][y + j].type != AIR)
              continue; // Only grow into empty space
            double randNum = ((double)rand() / (double)RAND_MAX);
            // This decides the speed at which leaves spread while they are
            // alive
            if (randNum < leafGrowthRate) {
              nextGrid[x + i][y + j].type = LEAF;
              nextGrid[x + i][y + j].age = 0;
              nextGrid[x + i][y + j].treeAge = grid[x][y].treeAge + 1;
              nextGrid[x + i][y + j].hueOffset =
                  (float)((double)rand() / (double)RAND_MAX) * .2f - .1f;
            }
          }
        }
      }
    }

    if (grid[x][y].type == FIRE) {
      if (grid[x][y].age < 20) {
        double randNum = ((double)rand() / (double)RAND_MAX);
        for (int i = -1; i <= 1; i++) {
          for (int j = -1; j <= 1; j++) {
            if ((!i && !j) || x + i < 0 || x + i > GRID_SIZE - 1 || y + j < 0 ||
                y + j > GRID_SIZE - 1) {
              continue;
            } // Ignore out of bounds
            if (nextGrid[x + i][y + j].type == ROCK ||
                nextGrid[x + i][y + j].type == WATER ||
                nextGrid[x + i][y + j].type == AIR)
              continue; // Only grow into empty space
            double randNum = ((double)rand() / (double)RAND_MAX);
            // This decides the speed at which leaves spread while they are
            // alive
            if (randNum < .025) {
              nextGrid[x + i][y + j].type = FIRE;
              nextGrid[x + i][y + j].hueOffset =
                  (float)((double)rand() / (double)RAND_MAX) * .3f - .15f;
              nextGrid[x + i][y + j].age = 0;
            }
          }
        }
      } else if (grid[x][y].age > 25) {
        nextGrid[x][y].type = AIR;
        nextGrid[x][y].hueOffset = 0.0f;
      }
    }

    // Incrememnt age for every cell
    nextGrid[x][y].age = nextGrid[x][y].age + 1;
  }

  if (maz) {
    for (int x = 0; x < GRID_SIZE; x++) {
      for (int y = 0; y < GRID_SIZE; y++) {
        performUpdates(x, y);
      }
    }
  } else {
    for (int x = GRID_SIZE - 1; x >= 0; x--) {
      for (int y = 0; y < GRID_SIZE; y++) {
        performUpdates(x, y);
      }
    }
  }

  // Copy nextGrid to grid
  for (int x = 0; x < GRID_SIZE; x++) {
    for (int y = 0; y < GRID_SIZE; y++) {
      grid[x][y] = nextGrid[x][y];
    }
  }

  maz = !maz;
}

void placeBlock(int gridX, int gridY, CellType type) {
  for (int i = -strokeSize; i <= strokeSize; i++) {
    for (int j = -strokeSize; j <= strokeSize; j++) {
      if (i * i + j * j <= strokeSize * strokeSize) {
        int newX = gridX + i;
        int newY = gridY + j;
        if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE) {
          grid[newX][newY].type = type;
          grid[newX][newY].age = 0;
          grid[newX][newY].treeAge = 0;
          if (type == WOOD) {
            grid[newX][newY].plantDNAType = BASIC_PLANT;
          }
        }
      }
    }
  }
}

void mouseFunction(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    int gridX = (x * GRID_SIZE) / WINDOW_SIZE;
    int gridY = GRID_SIZE - (y * GRID_SIZE) / WINDOW_SIZE;
    placeBlock(gridX, gridY, currentElement); // Example: placing SAND
  }
}

void motionFunction(int x, int y) {
  int gridX = (x * GRID_SIZE) / WINDOW_SIZE;
  int gridY = GRID_SIZE - (y * GRID_SIZE) /
                              WINDOW_SIZE; // Convert screen to grid coordinates
  placeBlock(gridX, gridY, currentElement);
}

void keyboardFunction(unsigned char key, int x, int y) {
  // ... existing keyboard handling code ...
  if (key == 53) { // Assuming 53 is the ASCII code for numpad 5
    isPaused = !isPaused;
  } else if (key == '6') {
    if (isPaused) {
      applyPhysics();
      glutPostRedisplay();
    }
  } else if (key == GLUT_KEY_RIGHT) {
    currentElement =
        (currentElement + 1) % TYPE_SIZE; // Cycle to the next element
  } else if (key == GLUT_KEY_LEFT) {
    currentElement = (currentElement + TYPE_SIZE - 1) %
                     TYPE_SIZE; // Cycle to the previous element
  } else if (key == ' ') {      // Check if the space bar is pressed
    initGrid();                 // Reset the grid
  }
}

void specialInput(int key, int x, int y) {
  switch (key) {
  case GLUT_KEY_RIGHT:
    currentElement = (currentElement + 1) % TYPE_SIZE;
    break;
  case GLUT_KEY_LEFT:
    currentElement = (currentElement + TYPE_SIZE - 1) % TYPE_SIZE;
    break;
  case GLUT_KEY_UP:
    strokeSize = (strokeSize < maxStrokeSize) ? strokeSize + 1 : maxStrokeSize;
    break;
  case GLUT_KEY_DOWN:
    strokeSize = (strokeSize > minStrokeSize) ? strokeSize - 1 : minStrokeSize;
    break;
  }
}

void renderBitmapString(float x, float y, void *font, const char *string) {
  const char *c;
  glRasterPos2f(x, y);
  for (c = string; *c != '\0'; c++) {
    glutBitmapCharacter(font, *c);
  }
}

// void keyboardFunction(unsigned char key, int x, int y) {
//     if (key == ' ') {  // Check if the space bar is pressed
//         initGrid();    // Reset the grid
//     }
// }

void reshape(int width, int height) {
  glutReshapeWindow(WINDOW_SIZE, WINDOW_SIZE);
}

void display() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();

  for (int x = 0; x < GRID_SIZE; x++) {
    for (int y = 0; y < GRID_SIZE; y++) {
      CellType type = grid[x][y].type;
      float xLeft = (x * CELL_SIZE) - 1.0f;
      float yBottom = (y * CELL_SIZE) - 1.0f;

      // Set color based on cell properties
      if (grid[x][y].plantDNAType) {
        glColor3f(
            plantDNAs[grid[x][y].plantDNAType].color[0] + grid[x][y].hueOffset,
            plantDNAs[grid[x][y].plantDNAType].color[1] + grid[x][y].hueOffset,
            plantDNAs[grid[x][y].plantDNAType].color[2] + grid[x][y].hueOffset);
      } else {
        glColor3f(cellProperties[type].color[0] + grid[x][y].hueOffset,
                  cellProperties[type].color[1] + grid[x][y].hueOffset,
                  cellProperties[type].color[2] + grid[x][y].hueOffset);
      }

      glBegin(GL_QUADS);
      glVertex2f(xLeft, yBottom);
      glVertex2f(xLeft + CELL_SIZE, yBottom);
      glVertex2f(xLeft + CELL_SIZE, yBottom + CELL_SIZE);
      glVertex2f(xLeft, yBottom + CELL_SIZE);
      glEnd();
    }
  }

  glColor3f(1.0f, 1.0f, 1.0f); // White color for text
  const char *elementNames[] = {"Air",  "Sand", "Water", "Rock",
                                "Wood", "Leaf", "Fire"};
  renderBitmapString(0.5f, 0.9f, GLUT_BITMAP_TIMES_ROMAN_24,
                     elementNames[currentElement]);

  char strokeSizeText[50];
  sprintf(strokeSizeText, "Stroke Size: %d", strokeSize);
  renderBitmapString(0.5f, 0.85f, GLUT_BITMAP_TIMES_ROMAN_24, strokeSizeText);

  glutSwapBuffers();
}

void timer(int value) {
  if (!isPaused) {
    applyPhysics();
  }
  glutPostRedisplay();
  glutTimerFunc(UPDATE_RATE, timer, 0);
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
  glutCreateWindow("Falling Sand Simulation");
  initGrid();

  glutDisplayFunc(display);
  glutTimerFunc(UPDATE_RATE, timer, 0);

  glMatrixMode(GL_PROJECTION);
  gluOrtho2D(-1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);

  glutMouseFunc(mouseFunction);
  glutMotionFunc(motionFunction);
  glutKeyboardFunc(keyboardFunction);
  glutSpecialFunc(specialInput);

  glutReshapeFunc(reshape);

  glutMainLoop();
  return 0;
}