
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#endif

#ifdef __unix__
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#ifdef _WIN32
#include <GL/glut.h>
#include <gl/gl.h>
#include <gl/glu.h>
#endif
        /* world size and storage array */
#define WORLDX 100
#define WORLDY 50
#define WORLDZ 100

	/* list of cubes to draw with each screen update */
#define MAX_DISPLAY_LIST 500000

/*Maximum number of humans allowed at the same time*/
#define MAX_HUMANS 20

	/* maximum number of user defined colours */
#define NUMBERCOLOURS 100

#define TUBE_COUNT 10

//defines for simplification of default colours
#define EMPTY 0
#define GREEN 1
#define BLUE 2
#define RED 3
#define BLACK 4
#define WHITE 5
#define PURPLE 6
#define ORANGE 7
#define YELLOW 8

typedef struct Human {
    int x, y, z; //coords for the head of the person. subtract y for chest and legs
} Human;