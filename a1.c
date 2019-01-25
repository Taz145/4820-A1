
/* Derived from scene.c in the The OpenGL Programming Guide */
/* Keyboard and mouse rotation taken from Swiftless Tutorials #23 Part 2 */
/* http://www.swiftless.com/tutorials/opengl/camera2.html */

/* Frames per second code taken from : */
/* http://www.lighthouse3d.com/opengl/glut/index.php?fps */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "graphics.h"

extern GLubyte world[WORLDX][WORLDY][WORLDZ];

/* mouse function called by GLUT when a button is pressed or released */
void mouse(int, int, int, int);

/* initialize graphics library */
extern void graphicsInit(int *, char **);

/* lighting control */
extern void setLightPosition(GLfloat, GLfloat, GLfloat);
extern GLfloat* getLightPosition();

/* viewpoint control */
extern void setViewPosition(float, float, float);
extern void getViewPosition(float *, float *, float *);
extern void getOldViewPosition(float *, float *, float *);
extern void setViewOrientation(float, float, float);
extern void getViewOrientation(float *, float *, float *);

/* add cube to display list so it will be drawn */
extern void addDisplayList(int, int, int);

/* mob controls */
extern void createMob(int, float, float, float, float);
extern void setMobPosition(int, float, float, float, float);
extern void hideMob(int);
extern void showMob(int);

/* player controls */
extern void createPlayer(int, float, float, float, float);
extern void setPlayerPosition(int, float, float, float, float);
extern void hidePlayer(int);
extern void showPlayer(int);

/* 2D drawing functions */
extern void  draw2Dline(int, int, int, int, int);
extern void  draw2Dbox(int, int, int, int);
extern void  draw2Dtriangle(int, int, int, int, int, int);
extern void  set2Dcolour(float[]);
extern void  draw2Dpoint(int x, int y, int pointSize, int smooth);


/* flag which is set to 1 when flying behaviour is desired */
extern int flycontrol;
/* flag used to indicate that the test world should be used */
extern int testWorld;
/* flag to print out frames per second */
extern int fps;
/* flag to indicate the space bar has been pressed */
extern int space;
/* flag indicates the program is a client when set = 1 */
extern int netClient;
/* flag indicates the program is a server when set = 1 */
extern int netServer;
/* size of the window in pixels */
extern int screenWidth, screenHeight;
/* flag indicates if map is to be printed */
extern int displayMap;
/* flag indicates use of a fixed viewpoint */
extern int fixedVP;

//used for timing animation
clock_t start, end;

/* frustum corner coordinates, used for visibility determination  */
extern float corners[4][3];

/* determine which cubes are visible e.g. in view frustum */
extern void ExtractFrustum();
extern void tree(float, float, float, float, float, float, int);

/* allows users to define colours */
extern int setUserColour(int, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat,
    GLfloat, GLfloat, GLfloat);
void unsetUserColour(int);
extern void getUserColour(int, GLfloat *, GLfloat *, GLfloat *, GLfloat *,
    GLfloat *, GLfloat *, GLfloat *, GLfloat *);

/********* end of extern variable declarations **************/


    /*** collisionResponse() ***/
    /* -performs collision detection and response */
    /*  sets new xyz  to position of the viewpoint after collision */
    /* -can also be used to implement gravity by updating y position of vp*/
    /* note that the world coordinates returned from getViewPosition()
       will be the negative value of the array indices */
void collisionResponse() {
    int i;
    float nextX, nextY, nextZ;
    float currX, currY, currZ;
    getViewPosition(&nextX, &nextY, &nextZ);
    getOldViewPosition(&currX, &currY, &currZ);
    nextX *= -1;
    nextY *= -1;
    nextZ *= -1;

    //for use when checking the contents of the world array
    int nextXi, nextYi, nextZi; //floored integers of next positions
    int currXi, currYi, currZi; //floored integers of current positions
    float collBuff = 1.5;
    nextXi = (int)floor(nextX);
    nextYi = (int)floor(nextY);
    nextZi = (int)floor(nextZ);
    currXi = (int)floor(currX);
    currYi = (int)floor(currY);
    currZi = (int)floor(currZ);


    //detects out of bounds
    if (nextX < 0 || nextY < 0 || nextZ < 0 || nextX > 100 || nextY > 50 || nextZ > 100) {
        setViewPosition(currX, currY, currZ);
    }

    //block collision
    if (world[nextXi][nextYi][nextZi] != 0 &&
        (nextX < nextXi + 1.2 && nextY < nextYi + 1.2 && nextZ < nextZi + 1.2)) {
        setViewPosition(currX, currY, currZ); //don't let us move
    }
}


/******* draw2D() *******/
/* draws 2D shapes on screen */
/* use the following functions:             */
/*  draw2Dline(int, int, int, int, int);        */
/*  draw2Dbox(int x1, int y1, int x2, int y2);          */
/*  draw2Dtriangle(int, int, int, int, int, int);   */
/*  set2Dcolour(float []);              */
/* colour must be set before other functions are called */
void draw2D() {

    if (testWorld) {
        /* draw some sample 2d shapes */
        if (displayMap == 1) {
            GLfloat green[] = { 0.0, 0.5, 0.0, 0.5 };
            set2Dcolour(green);
            draw2Dline(0, 0, 500, 500, 15);
            draw2Dtriangle(0, 0, 200, 200, 0, 200);

            GLfloat black[] = { 0.0, 0.0, 0.0, 0.5 };
            set2Dcolour(black);
            draw2Dbox(500, 380, 524, 388);
        }
    }
    else {
        //draws a 1:1 scale minimap of the world. Does not have functionality for user defined colours
        if (displayMap == 1) {
            int x, y, z;
            int colour;
            GLfloat blue[] = { 0.0, 0.0, 1.0, 1.0 };
            GLfloat red[] = { 1.0, 0.0, 0.0, 1.0 };
            GLfloat green[] = { 0.0, 1.0, 0.0, 1.0 };
            GLfloat yellow[] = { 1.0, 1.0, 0.0, 1.0 };
            GLfloat purple[] = { 1.0, 0.0, 1.0, 1.0 };
            GLfloat orange[] = { 1.0, 0.64, 0.0, 1.0 };
            GLfloat white[] = { 1.0, 1.0, 1.0, 1.0 };
            GLfloat black[] = { 0.0, 0.0, 0.0, 1.0 };
            GLfloat grey[] = { 0.0, 0.0, 0.0, 0.5 };
            set2Dcolour(grey);
            draw2Dbox(0, 0, 100, 100);
            glDisable(GL_DEPTH_TEST);
            const char* colours[8] = { "blue","red","green","yellow","purple","orange","white","black" };
            for (x = 0; x < WORLDX - 1; x++) {
                for (z = 0; z < WORLDZ - 1; z++) {
                    y = WORLDY - 1;
                    while (world[x][y][z] == 0 && y > 0) y--; //ignore empty spaces
                    if (world[x][y][z] != 0) {
                        colour = world[x][y][z];
                        if (colour == 1) {
                            set2Dcolour(green);
                        }
                        else if (colour == 2) {
                            set2Dcolour(blue);
                        }
                        else if (colour == 3) {
                            set2Dcolour(red);
                        }
                        else if (colour == 4) {
                            set2Dcolour(black);
                        }
                        else if (colour == 5) {
                            set2Dcolour(white);
                        }
                        else if (colour == 6) {
                            set2Dcolour(purple);
                        }
                        else if (colour == 7) {
                            set2Dcolour(orange);
                        }
                        else if (colour == 8) {
                            set2Dcolour(yellow);
                        }
                        draw2Dpoint(x, z, 1, 1);
                    }
                }
            }
            glEnable(GL_DEPTH_TEST);
        }
    }
}


/*** update() ***/
/* background process, it is called when there are no other events */
/* -used to control animations and perform calculations while the  */
/*  system is running */
/* -gravity must also implemented here, duplicate collisionResponse */
void update() {
    int x, y, z;
    float *la;

    /* sample animation for the test world, don't remove this code */
    /* demo of animating mobs */
    if (testWorld) {

        /* sample of rotation and positioning of mob */
        /* coordinates for mob 0 */
        static float mob0x = 50.0, mob0y = 25.0, mob0z = 52.0;
        static float mob0ry = 0.0;
        static int increasingmob0 = 1;
        /* coordinates for mob 1 */
        static float mob1x = 50.0, mob1y = 25.0, mob1z = 52.0;
        static float mob1ry = 0.0;
        static int increasingmob1 = 1;
        /* counter for user defined colour changes */
        static int colourCount = 0;
        static GLfloat offset = 0.0;

        /* move mob 0 and rotate */
        /* set mob 0 position */
        setMobPosition(0, mob0x, mob0y, mob0z, mob0ry);

        /* move mob 0 in the x axis */
        if (increasingmob0 == 1) {
            mob0x += 0.2;
        }
        else
        {
            mob0x -= 0.2;
        }
        if (mob0x > 50) increasingmob0 = 0;
        if (mob0x < 30) increasingmob0 = 1;

        /* rotate mob 0 around the y axis */
        mob0ry += 1.0;
        if (mob0ry > 360.0) mob0ry -= 360.0;

        /* move mob 1 and rotate */
        setMobPosition(1, mob1x, mob1y, mob1z, mob1ry);

        /* move mob 1 in the z axis */
        /* when mob is moving away it is visible, when moving back it */
        /* is hidden */
        if (increasingmob1 == 1) {
            mob1z += 0.2;
            showMob(1);
        }
        else {
            mob1z -= 0.2;
            hideMob(1);
        }
        if (mob1z > 72) increasingmob1 = 0;
        if (mob1z < 52) increasingmob1 = 1;

        /* rotate mob 1 around the y axis */
        mob1ry += 1.0;
        if (mob1ry > 360.0) mob1ry -= 360.0;

        /* change user defined colour over time */
        if (colourCount == 1) offset += 0.05;
        else offset -= 0.01;
        if (offset >= 0.5) colourCount = 0;
        if (offset <= 0.0) colourCount = 1;
        setUserColour(9, 0.7, 0.3 + offset, 0.7, 1.0, 0.3, 0.15 + offset, 0.3, 1.0);

        /* end testworld animation */


    }
    else {
        double time_elapsed;
        end = clock();
        time_elapsed = ((double)end - start) / CLOCKS_PER_SEC;

        //do stuff based on how much time has passed here

        float x, y, z;
        getOldViewPosition(&x, &y, &z);
        y += time_elapsed * (-9.8);


    }
}


/* called by GLUT when a mouse button is pressed or released */
/* -button indicates which button was pressed or released */
/* -state indicates a button down or button up event */
/* -x,y are the screen coordinates when the mouse is pressed or */
/*  released */
void mouse(int button, int state, int x, int y) {

    if (button == GLUT_LEFT_BUTTON)
        printf("left button - ");
    else if (button == GLUT_MIDDLE_BUTTON)
        printf("middle button - ");
    else
        printf("right button - ");

    if (state == GLUT_UP)
        printf("up - ");
    else
        printf("down - ");

    printf("%d %d\n", x, y);
}

void fillGaps() {
    int x, y, z;
    for (x = 0; x < WORLDX - 1; x++) {
        for (z = 0; z < WORLDZ - 1; z++) {
            for (y = WORLDY - 1; y > 0; y--) {

                if (world[x][y][z] != 0 && world[x][y - 1][z] == 0) { //found a block and there is empty space below it
                    if (x == 0 && z == 0) { //bottom left corner
                        if (world[x + 1][y - 1][z] != 1 ||
                            world[x][y - 1][z + 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                    else if (x == 0 && z == WORLDZ - 1) { //top left
                        if (world[x + 1][y - 1][z] != 1 ||
                            world[x][y - 1][z - 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                    else if (x == WORLDX - 1 && z == 0) { //bottom right
                        if (world[x - 1][y - 1][z] != 1 ||
                            world[x][y - 1][z + 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                    else if (x == WORLDX - 1 && z == WORLDZ - 1) { //top right
                        if (world[x - 1][y - 1][z] != 1 ||
                            world[x][y - 1][z - 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                    else if (x == 0) { //left edge
                        if (world[x + 1][y - 1][z] != 1 ||
                            world[x][y - 1][z + 1] != 1 ||
                            world[x][y - 1][z + 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                    else if (x == WORLDX - 1) { //right edge
                        if (world[x - 1][y - 1][z] != 1 ||
                            world[x][y - 1][z + 1] != 1 ||
                            world[x][y - 1][z - 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                    else if (z == 0) { //bottom edge
                        if (world[x + 1][y - 1][z] != 1 ||
                            world[x - 1][y - 1][z] != 1 ||
                            world[x][y - 1][z + 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                    else if (z == WORLDZ - 1) { //top edge
                        if (world[x + 1][y - 1][z] != 1 ||
                            world[x - 1][y - 1][z] != 1 ||
                            world[x][y - 1][z - 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                    else { //general case
                        if (world[x + 1][y - 1][z] != 1 ||
                            world[x - 1][y - 1][z] != 1 ||
                            world[x][y - 1][z + 1] != 1 ||
                            world[x][y - 1][z - 1] != 1) world[x][y - 1][z] = world[x][y][z];
                    }
                }
            }
        }
    }
}

int readGroundFile() {
    FILE *fp;
    fp = fopen("ground.pgm", "r");

    if (fp == NULL) {
        fprintf(stderr, "Unable to open ground file. Running default terrain.\n");
        return 1;
    }

    char c;
    int x, y, z, row, col, maxH;

    c = getc(fp);
    if (c != 'P') {
        fprintf(stderr, "Not a valid pgm file\n");
        return 1;
    }
    c = getc(fp);
    if (c != '2') {
        fprintf(stderr, "Not a valid pgm file\n");
        return 1;
    }

    while (getc(fp) != '\n'); //goes to EOL
    while (getc(fp) == '#'); //skip comments
    while (getc(fp) != '\n'); //goes to EOL following comments

    fscanf(fp, "%d", &row);
    fscanf(fp, "%d", &col);
    fscanf(fp, "%d", &maxH);
    for (x = 0; x < row - 1; x++) {
        for (z = col - 1; z >= 0; z--) {
            fscanf(fp, "%d", &y);
            world[x][(int)floor(y / 9)][z] = 3; //5.1 scales 255 to exactly 50.
        }
    }
    fclose(fp);
    fillGaps();
    return 0;
}

int main(int argc, char** argv)
{
    int i, j, k;
    /* initialize the graphics system */
    graphicsInit(&argc, argv);

    /* the first part of this if statement builds a sample */
    /* world which will be used for testing */
    /* DO NOT remove this code. */
    /* Put your code in the else statment below */
    /* The testworld is only guaranteed to work with a world of
        with dimensions of 100,50,100. */
    if (testWorld == 1) {
        /* initialize world to empty */
        for (i = 0; i < WORLDX; i++)
            for (j = 0; j < WORLDY; j++)
                for (k = 0; k < WORLDZ; k++)
                    world[i][j][k] = 0;

        /* some sample objects */
        /* build a red platform */
        for (i = 0; i < WORLDX; i++) {
            for (j = 0; j < WORLDZ; j++) {
                world[i][24][j] = 3;
            }
        }
        /* create some green and blue cubes */
        world[50][25][50] = 1;
        world[49][25][50] = 1;
        world[49][26][50] = 1;
        world[52][25][52] = 2;
        world[52][26][52] = 2;

        /* create user defined colour and draw cube */
        setUserColour(9, 0.7, 0.3, 0.7, 1.0, 0.3, 0.15, 0.3, 1.0);
        world[54][25][50] = 9;


        /* blue box shows xy bounds of the world */
        for (i = 0; i < WORLDX - 1; i++) {
            world[i][25][0] = 2;
            world[i][25][WORLDZ - 1] = 2;
        }
        for (i = 0; i < WORLDZ - 1; i++) {
            world[0][25][i] = 2;
            world[WORLDX - 1][25][i] = 2;
        }

        /* create two sample mobs */
        /* these are animated in the update() function */
        createMob(0, 50.0, 25.0, 52.0, 0.0);
        createMob(1, 50.0, 25.0, 52.0, 0.0);

        /* create sample player */
        createPlayer(0, 52.0, 27.0, 52.0, 0.0);
    }
    else {
        //TODO see about adding in code to change the curor to help the player stay right side up
        //i.e cursor points up when upside down
        glutSetCursor(GLUT_CURSOR_NONE);
        int x, y, z;
        if (readGroundFile() == 1) { //no valid world file. Make default world
            //init the world array
            for (x = 0; x < 100; x++) {
                for (y = 0; y < 50; y++) {
                    for (z = 0; z < 100; z++) {
                        world[x][y][z] = 0;
                    }
                }
            }
            //builds a 50x50 yellow platform at height zero and two multi coloured walls along the x-axis
            for (x = 0; x < WORLDX - 1; x++) {
                for (y = 2; y < WORLDY - 1; y++) {
                    world[x][y][0] = (x % 6) + 1;
                    world[x][y][WORLDZ - 1] = (x % 6) + 1;
                    for (z = 0; z < WORLDZ - 1; z++) {
                        world[x][2][z] = 8;
                    }
                }
            }
        }
    }
    /* starts the graphics processing loop */
    /* code after this will not run until the program exits */
    start = clock();
    glutMainLoop();
    return 0;
}

