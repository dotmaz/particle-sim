#include <GL/glut.h>
#include <stdbool.h>
#include <stdio.h>

/* ---------------------------------------- Preprocessor Constants ---------------------------------------- */

// Window resolution
#define WINDOW_SIZE 800

// Square size of cell grid
#define GRID_SIZE 200
#define CELL_SIZE .01f

// Time between updates in milliseconds
#define UPDATE_RATE 15

/* ---------------------------------------- Enums ---------------------------------------- */

// CellType - Type of cell used in decisions for next state (corresponds to user-interactable elements)
// PlantDNAType - TEMPORARY - To store specific gene sequences

typedef enum { AIR, SAND, WATER, ROCK, WOOD, LEAF, FIRE } CellType;
typedef enum { BASIC_PLANT, FAT_PLANT, MERGE_PLANT } PlantDNAType;

/* ---------------------------------------- Data Structures ---------------------------------------- */

// Cell defines the cell's main data structure
typedef struct {
  CellType type;
  bool isValid;
  int age;
  int treeAge;
  float hueOffset;

  PlantDNAType plantDNAType;
} Cell;

// Neighborhood used to provide an easy to use API when performing updates
typedef struct {
  Cell *top;
  Cell *bottom;
  Cell *left;
  Cell *right;
  Cell *topLeft;
  Cell *topRight;
  Cell *bottomLeft;
  Cell *bottomRight;
} Neighborhood;

// CellTypeProperties define attributes with values that vary for different types
typedef struct {
  float density;  // UNUSED: TODO Implement density to allow denser cells to fall below less dense cells in performCellUpdates()
  bool isFluid;   // UNUSED: TODO Replace checks for water with checks for isFluid property when handling liquid physics in performCellUpdates()
  float color[3]; // Color of the cell type
} CellTypeProperties;

// List of CellTypeProperties corresponding to available cell types
CellTypeProperties cellTypeProperties[] = {
    {0.0, false, {0.0f, 0.0f, 0.0f}},      // Properties for AIR
    {1.6, false, {0.8f, 0.6f, 0.2f}},      // Properties for SAND
    {1.0, true, {0.0f, 0.0f, 1.0f}},       // Properties for WATER
    {2.5, false, {0.5f, 0.5f, 0.5f}},      // Properties for ROCK
    {2.5, false, {0.36f, 0.27f, 0.08f}},   // Properties for WOOD
    {1.2, false, {0.0f, 0.0f, 0.0f}},      // Properties for LEAF
    {0.5, false, {0.812f, 0.098f, 0.098f}} // Properties for FIRE
};

// PlantDNA defines the parameters that decide how plant growth is randomly generated
typedef struct {
  double woodMaxAge;           // Maximum age of wood cell before it stops spreading
  double woodMaxTreeAge;       // Maximum age of wood cell's propogated plant age before the cell stops spreading
  double woodGrowthUp;         // Probability for upward wood spread
  double woodGrowthHorizontal; // Probability for horizontal wood spread
  double woodGrowthDown;       // Probability for downward wood spread
  double woodLeafGrowth;       // Probability for leaves to sprout from wood
  double leafMaxAge;           // Maximum age of leaf cell before it stops spreading
  double leadMaxTreeAge;       // Maximum age of leaf cell's propogated plant age before the cell stops spreading
  double leafGrowthRate;       // Probability for leaf spread
  float color[3];              // Leaf color (this overrides the cell's color derived from CellTypeProperties)
} PlantDNA;

// List of specific gene sequences (corresponds to PlantDNAType)
PlantDNA plantDNAs[] = {
    {20, 20, .02, .005, .002, .001, 70, 5, .01, {0.168f, 0.51f, 0.165f}},   // Properties for BASIC PLANT
    {100, 10, .01, .002, .002, .001, 100, 5, .02, {0.91F, 0.447F, 0.978f}}, // Properties for FAT PLANT
    // {60, 15, .015, .0035, .002, .001, 85, 5, .015, {0.539f, 0.4785f, 0.5715f}} // Properties for MERGE PLANT
};

/* ---------------------------------------- Globals ---------------------------------------- */

const int TYPE_SIZE = 7; // Total number of cell types

Cell grid[GRID_SIZE][GRID_SIZE];     // Active cell grid
Cell nextGrid[GRID_SIZE][GRID_SIZE]; /// Next state cell grid
CellType currentElement = WOOD;      // Cell type to track current element
const char *cellTypeNames[] = {"Air", "Sand", "Water", "Rock", "Wood", "Leaf", "Fire"};

// Placing cells with mouse
int strokeSize = 0;           // Default stroke size
const int minStrokeSize = 0;  // Minimum stroke size
const int maxStrokeSize = 20; // Maximum stroke size

// Rendering
bool renderForward = true; // Oscillating boolean to decide the order in which cells are rendered
bool isPaused = false;     // Tracks if the simulation is paused

/* ---------------------------------------- Function Headers ---------------------------------------- */

void performCellUpdates(int x, int y);
Neighborhood getNeighbors(int x, int y);

/* ---------------------------------------- Main Functions ---------------------------------------- */

// Initalize grid to empty cells
void initGrid() {
  for (int x = 0; x < GRID_SIZE; x++) {
    for (int y = 0; y < GRID_SIZE; y++) {
      grid[x][y].type = AIR;
      grid[x][y].isValid = true;
      grid[x][y].age = 0;
      grid[x][y].treeAge = 0;
      grid[x][y].hueOffset = 0.0f;
    }
  }
}

// Perform updates to all cells
void performGridUpdates() {

  // Copy current state grid to new state grid
  for (int x = 0; x < GRID_SIZE; x++) {
    for (int y = 0; y < GRID_SIZE; y++) {
      nextGrid[x][y] = grid[x][y];
    }
  }

  // Perform updates on new state
  if (renderForward) {
    for (int x = 0; x < GRID_SIZE; x++) {
      for (int y = 0; y < GRID_SIZE; y++) {
        performCellUpdates(x, y);
      }
    }
  } else {
    for (int x = GRID_SIZE - 1; x >= 0; x--) {
      for (int y = 0; y < GRID_SIZE; y++) {
        performCellUpdates(x, y);
      }
    }
  }

  // Copy new state grid to current state grid
  for (int x = 0; x < GRID_SIZE; x++) {
    for (int y = 0; y < GRID_SIZE; y++) {
      grid[x][y] = nextGrid[x][y];
    }
  }

  renderForward = !renderForward;
}

// Perform updates on a particular cell given it's coordinates
void performCellUpdates(int x, int y) {
  Neighborhood neighbors = getNeighbors(x, y);
  Cell *cell = &grid[x][y];
  Cell *nextCell = &nextGrid[x][y];

  // if (neighbors.bottom.isValid == false || neighbors.left.isValid == false) {
  //   printf("%i %i\n", x, y);
  // }

  if (cell->type == SAND || cellTypeProperties[cell->type].isFluid) {
    // bool canMoveDown = y > 0 && nextGrid[x][y - 1].type == AIR;
    bool canMoveBottomRight = x < GRID_SIZE - 1 && y > 0 && nextGrid[x + 1][y - 1].type == AIR;
    bool canMoveBottomLeft = x > 0 && y > 0 && nextGrid[x - 1][y - 1].type == AIR;
    bool canMoveRight = cellTypeProperties[cell->type].isFluid && x < GRID_SIZE - 1 && nextGrid[x + 1][y].type == AIR;
    bool canMoveLeft = cellTypeProperties[cell->type].isFluid && x > 0 && nextGrid[x - 1][y].type == AIR;

    // if (canMoveDown) {
    //   nextGrid[x][y].type = AIR;
    //   nextGrid[x][y - 1].type = grid[x][y].type;
    // }

    if (neighbors.bottom->isValid && neighbors.bottom->type == AIR) {
      nextCell->type = AIR;
      neighbors.bottom->type = cell->type;
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

  if (grid[x][y].type == WOOD) {

    const double woodMaxAge = plantDNAs[grid[x][y].plantDNAType].woodMaxAge;
    const double woodMaxTreeAge = plantDNAs[grid[x][y].plantDNAType].woodMaxTreeAge;
    const double woodGrowthUp = plantDNAs[grid[x][y].plantDNAType].woodGrowthUp;
    const double woodGrowthHorizontal = plantDNAs[grid[x][y].plantDNAType].woodGrowthHorizontal;
    const double woodGrowthDown = plantDNAs[grid[x][y].plantDNAType].woodGrowthDown;
    const double woodLeafGrowth = plantDNAs[grid[x][y].plantDNAType].woodLeafGrowth;

    if (grid[x][y].age < woodMaxAge && grid[x][y].treeAge < woodMaxTreeAge) {
      for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
          if ((!i && !j) || x + i < 0 || x + i > GRID_SIZE - 1 || y + j < 0 || y + j > GRID_SIZE - 1) {
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
            nextGrid[x + i][y + j].hueOffset = (float)((double)rand() / (double)RAND_MAX) * .2f - .1f;
            nextGrid[x + i][y + j].plantDNAType = grid[x][y].plantDNAType; // Propagate plant DNA type
          }

          double leafRand = ((double)rand() / (double)RAND_MAX);
          if (leafRand < woodLeafGrowth) {
            nextGrid[x + i][y + j].type = LEAF;
            nextGrid[x + i][y + j].treeAge = 0;
            nextGrid[x + i][y + j].age = 0;
            nextGrid[x + i][y + j].plantDNAType = grid[x][y].plantDNAType; // Propagate plant DNA type
          }
        }
      }
    }
  }

  if (grid[x][y].type == LEAF) {

    const double leafMaxAge = plantDNAs[grid[x][y].plantDNAType].leafMaxAge;
    const double leadMaxTreeAge = plantDNAs[grid[x][y].plantDNAType].leadMaxTreeAge;
    const double leafGrowthRate = plantDNAs[grid[x][y].plantDNAType].leafGrowthRate;

    // First value decides how sparse the tree will be (max age before leaf
    // stops spreading) Second value decides how far the leaves can
    // potentially spread (max tree age before leaf stops spreading)
    if (grid[x][y].age < leafMaxAge && grid[x][y].treeAge < leadMaxTreeAge) {
      for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
          // Ignore out of bounds
          if ((!i && !j) || x + i < 0 || x + i > GRID_SIZE - 1 || y + j < 0 || y + j > GRID_SIZE - 1)
            continue;

          // Only grow into empty space
          if (nextGrid[x + i][y + j].type != AIR)
            continue;

          // Decide the speed at which leaves spread while they are alive
          double randNum = ((double)rand() / (double)RAND_MAX);
          if (randNum < leafGrowthRate) {
            nextGrid[x + i][y + j].type = LEAF;
            nextGrid[x + i][y + j].age = 0;
            nextGrid[x + i][y + j].treeAge = grid[x][y].treeAge + 1;
            nextGrid[x + i][y + j].hueOffset = (float)((double)rand() / (double)RAND_MAX) * .2f - .1f;
            nextGrid[x + i][y + j].plantDNAType = grid[x][y].plantDNAType; // Propagate plant DNA type
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
          // Ignore out of bounds
          if ((!i && !j) || x + i < 0 || x + i > GRID_SIZE - 1 || y + j < 0 || y + j > GRID_SIZE - 1)
            continue;

          // Only spread into flammable cells
          if (cellTypeProperties[nextGrid[x + i][y + j].type].isFluid || nextGrid[x + i][y + j].type == ROCK || nextGrid[x + i][y + j].type == AIR)
            continue;

          // Decide the speed at which fire spread while it's still alive
          double randNum = ((double)rand() / (double)RAND_MAX);
          if (randNum < .025) {
            nextGrid[x + i][y + j].type = FIRE;
            nextGrid[x + i][y + j].hueOffset = (float)((double)rand() / (double)RAND_MAX) * .3f - .15f;
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

/* ---------------------------------------- Helper Functions ---------------------------------------- */

// Render a string on the screen
void renderBitmapString(float x, float y, void *font, const char *string) {
  const char *c;
  glRasterPos2f(x, y);
  for (c = string; *c != '\0'; c++) {
    glutBitmapCharacter(font, *c);
  }
}

// Place block of cells at a given position
void placeBlock(int gridX, int gridY, CellType type, PlantDNAType plantDNAType) {
  for (int i = -strokeSize; i <= strokeSize; i++) {
    for (int j = -strokeSize; j <= strokeSize; j++) {
      if (i * i + j * j <= strokeSize * strokeSize) {
        int newX = gridX + i;
        int newY = gridY + j;
        if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE) {
          grid[newX][newY].type = type;
          grid[newX][newY].age = 0;
          grid[newX][newY].treeAge = 0;
          grid[newX][newY].plantDNAType = plantDNAType;
        }
      }
    }
  }
}

Neighborhood getNeighbors(int x, int y) {
  Neighborhood neighborhood;

  // Initialize all neighbors to default value
  Cell defaultCell = {0, false};
  neighborhood.top = &defaultCell;
  neighborhood.bottom = &defaultCell;
  neighborhood.left = &defaultCell;
  neighborhood.right = &defaultCell;
  neighborhood.topLeft = &defaultCell;
  neighborhood.topRight = &defaultCell;
  neighborhood.bottomLeft = &defaultCell;
  neighborhood.bottomRight = &defaultCell;

  // Check and assign neighbors, ensuring we don't go out of bounds
  if (y < GRID_SIZE - 1)
    neighborhood.top = &nextGrid[x][y + 1];
  if (y > 0)
    neighborhood.bottom = &nextGrid[x][y - 1];
  if (x > 0)
    neighborhood.left = &nextGrid[x - 1][y];
  if (x < GRID_SIZE - 1)
    neighborhood.right = &nextGrid[x + 1][y];
  if (x > 0 && y < GRID_SIZE - 1)
    neighborhood.topLeft = &nextGrid[x - 1][y + 1];
  if (x < GRID_SIZE - 1 && y < GRID_SIZE - 1)
    neighborhood.topRight = &nextGrid[x + 1][y + 1];
  if (x > 0 && y > 0)
    neighborhood.bottomLeft = &nextGrid[x - 1][y - 1];
  if (x < GRID_SIZE - 1 && y > 0)
    neighborhood.bottomRight = &nextGrid[x + 1][y - 1];

  return neighborhood;
}

/* ---------------------------------------- Event Listener Callbacks ---------------------------------------- */

void mouseFunction(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    int gridX = (x * GRID_SIZE) / WINDOW_SIZE;
    int gridY = GRID_SIZE - (y * GRID_SIZE) / WINDOW_SIZE;
    if (currentElement == LEAF) {
      placeBlock(gridX, gridY, WOOD, FAT_PLANT);
    } else {
      placeBlock(gridX, gridY, currentElement, BASIC_PLANT);
    }
  }
}

void motionFunction(int x, int y) {
  int gridX = (x * GRID_SIZE) / WINDOW_SIZE;
  int gridY = GRID_SIZE - (y * GRID_SIZE) / WINDOW_SIZE; // Convert screen to grid coordinates
  if (currentElement == LEAF) {
    placeBlock(gridX, gridY, WOOD, FAT_PLANT);
  } else {
    placeBlock(gridX, gridY, currentElement, BASIC_PLANT);
  }
}

void keyboardFunction(unsigned char key, int x, int y) {
  // ... existing keyboard handling code ...
  if (key == 53) { // Assuming 53 is the ASCII code for numpad 5
    isPaused = !isPaused;
  } else if (key == '6') {
    if (isPaused) {
      performGridUpdates();
      glutPostRedisplay();
    }
  } else if (key == GLUT_KEY_RIGHT) {
    currentElement = (currentElement + 1) % TYPE_SIZE; // Cycle to the next element
  } else if (key == GLUT_KEY_LEFT) {
    currentElement = (currentElement + TYPE_SIZE - 1) % TYPE_SIZE; // Cycle to the previous element
  } else if (key == ' ') {                                         // Check if the space bar is pressed
    initGrid();                                                    // Reset the grid
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

void reshape(int width, int height) { glutReshapeWindow(WINDOW_SIZE, WINDOW_SIZE); }

/* ---------------------------------------- Render Callbacks ---------------------------------------- */

// Display callback to render the grid of cells and UI elements
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
      if (type == LEAF) {
        glColor3f(plantDNAs[grid[x][y].plantDNAType].color[0] + grid[x][y].hueOffset, plantDNAs[grid[x][y].plantDNAType].color[1] + grid[x][y].hueOffset,
                  plantDNAs[grid[x][y].plantDNAType].color[2] + grid[x][y].hueOffset);
      } else {
        glColor3f(cellTypeProperties[type].color[0] + grid[x][y].hueOffset, cellTypeProperties[type].color[1] + grid[x][y].hueOffset,
                  cellTypeProperties[type].color[2] + grid[x][y].hueOffset);
      }

      glBegin(GL_QUADS);
      glVertex2f(xLeft, yBottom);
      glVertex2f(xLeft + CELL_SIZE, yBottom);
      glVertex2f(xLeft + CELL_SIZE, yBottom + CELL_SIZE);
      glVertex2f(xLeft, yBottom + CELL_SIZE);
      glEnd();
    }
  }

  glColor3f(1.0f, 1.0f,
            1.0f); // White color for text
  renderBitmapString(0.5f, 0.9f, GLUT_BITMAP_TIMES_ROMAN_24, cellTypeNames[currentElement]);

  char strokeSizeText[50];
  sprintf(strokeSizeText, "Stroke Size: %d", strokeSize);
  renderBitmapString(0.5f, 0.85f, GLUT_BITMAP_TIMES_ROMAN_24, strokeSizeText);

  glutSwapBuffers();
}

// Timer callback (run at UPDATE_RATE)
void timer(int value) {
  if (!isPaused) {
    performGridUpdates();
  }
  glutPostRedisplay();
  glutTimerFunc(UPDATE_RATE, timer, 0);
}

/* ---------------------------------------- Main Loop ---------------------------------------- */

int main(int argc, char **argv) {
  // Initialize GLUT and OpenGL
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
  glutCreateWindow("Particools");

  glMatrixMode(GL_PROJECTION);
  gluOrtho2D(-1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);

  // Attach event listener callbacks
  glutMouseFunc(mouseFunction);       // On mouse click
  glutMotionFunc(motionFunction);     // On mouse move while mouse is being clicked
  glutKeyboardFunc(keyboardFunction); // On key press
  glutSpecialFunc(specialInput);      // On special key press
  glutReshapeFunc(reshape);           // On window reshape

  // Attach render callbacks
  glutDisplayFunc(display);
  glutTimerFunc(UPDATE_RATE, timer, 0);

  // Initialize grid with cells
  initGrid();

  // Start the simulation
  glutMainLoop();
  return 0;
}