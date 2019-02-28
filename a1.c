
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
extern void setOldViewPosition(float, float, float);
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

extern float tubeData[TUBE_COUNT][6];
extern int tubeColour[TUBE_COUNT];
extern short tubeVisible[TUBE_COUNT];

//used for timing animation
clock_t gravityTimer, momentumTimer, rayAnimationTimer, end;
clock_t tubeTimer[TUBE_COUNT];

double decelMod = 0.9, accelMod = 1.0;
float lastX, lastY, lastZ;
int numTubes = 0;

int numHumans = 0; //used for error checking and indexing of the below array
Human humans[MAX_HUMANS];

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

extern void createTube(int, float, float, float, float, float, float, int);

extern void hideTube(int);

extern void showTube(int);

/********* end of extern variable declarations **************/

//x, y, z are the values that the human is moving in that direction. These are added to the humans current coords.
//They can be negative to allow movement in all directions
//Also can be used to draw humans for the first time by "moving" them to their current position
void moveHuman(int x, int y, int z, int index) {
    if (index >= numHumans || index < 0 || index > MAX_HUMANS) {
        printf("Invalid index for humans\n");
    }
    int newX, newY, newZ;
    newX = x + humans[index].x;
    newY = y + humans[index].y;
    newZ = z + humans[index].z;

    //remove the blocks from where the person was
    world[humans[index].x][humans[index].y][humans[index].z] = 0;
    world[humans[index].x][humans[index].y - 1][humans[index].z] = 0;
    world[humans[index].x][humans[index].y - 2][humans[index].z] = 0;

    //checks if there is space for the torso and legs and if there are any other blocks in those spaces
    if (world[newX][newY][newZ] != 0 || world[newX][newY - 1][newZ] != 0 || world[newX][newY - 2][newZ] != 0 ||
        newY < 2) {
        //printf("no space\n");
        world[humans[index].x][humans[index].y][humans[index].z] = ORANGE;
        world[humans[index].x][humans[index].y - 1][humans[index].z] = BLACK;
        world[humans[index].x][humans[index].y - 2][humans[index].z] = WHITE;
    }
    else {
        world[newX][newY][newZ] = ORANGE; //head
        world[newX][newY - 1][newZ] = BLACK; //torso
        world[newX][newY - 2][newZ] = WHITE; //legs



        //update the stuct to the new position
        humans[index].x = newX;
        humans[index].y = newY;
        humans[index].z = newZ;
    }

}

void makeHuman(int x, int y, int z) {
    if (numHumans < MAX_HUMANS) {
        humans[numHumans].x = x;
        humans[numHumans].y = y;
        humans[numHumans].z = z;
        world[humans[numHumans].x][humans[numHumans].y][humans[numHumans].z] = ORANGE;
        world[humans[numHumans].x][humans[numHumans].y - 1][humans[numHumans].z] = BLACK;
        world[humans[numHumans].x][humans[numHumans].y - 2][humans[numHumans].z] = WHITE;
        numHumans++;
    }
    else {
        printf("Max number of humans reached.\n");
    }
}

void moveTube(int tubeNum) {
    float bx, by, bz, ex, ey, ez;
    int x, y, z;

    bx = tubeData[tubeNum][3]; //ex
    by = tubeData[tubeNum][4]; //ey
    bz = tubeData[tubeNum][5]; //ez
    ex = bx + (bx - tubeData[tubeNum][0]);
    ey = by + (by - tubeData[tubeNum][1]);
    ez = bz + (bz - tubeData[tubeNum][2]);

    x = (int)floor(bx);
    y = (int)floor(by);
    z = (int)floor(bz);

    if (x < 0 || x > 99 || y < 0 || y > 49 || z < 0 || z > 99) { //prevents OOB errors in world array
        tubeVisible[tubeNum] = 0;
    }
    else {
        if (world[x][y][z] == ORANGE || world[x][y][z] == BLACK || world[x][y][z] == WHITE) {
            printf("A tube has hit a person!\n");
            tubeVisible[tubeNum] = 0;
        }
        else {
            tubeData[tubeNum][0] = bx;
            tubeData[tubeNum][1] = by;
            tubeData[tubeNum][2] = bz;
            tubeData[tubeNum][3] = ex;
            tubeData[tubeNum][4] = ey;
            tubeData[tubeNum][5] = ez;
        }
    }
}

/*** collisionResponse() ***/
/* -performs collision detection and response */
/*  sets new xyz  to position of the viewpoint after collision */
/* -can also be used to implement gravity by updating y position of vp*/
/* note that the world coordinates returned from getViewPosition()
   will be the negative value of the array indices */
void collisionResponse() {
    int i;
    float nextX, nextY, nextZ, currX, currY, currZ, deltaX, deltaY, deltaZ;

    getViewPosition(&nextX, &nextY, &nextZ);
    getOldViewPosition(&currX, &currY, &currZ);
    nextX *= -1;
    nextY *= -1;
    nextZ *= -1;

    deltaX = fabs(nextX - currX);
    deltaY = fabs(nextY - currY);
    deltaZ = fabs(nextZ - currZ);

    //for use when checking the contents of the world array
    int nextXi, nextYi, nextZi; //floored integers of next positions
    nextXi = (int)floor(nextX);
    nextYi = (int)floor(nextY);
    nextZi = (int)floor(nextZ);


    //detects out of bounds
    if (nextX < 0 || nextY < 0 || nextZ < 0 || nextX > 100 || nextY > 50 || nextZ > 100) {
        setViewPosition(currX, currY, currZ);
    }

    //block collision
    if (world[nextXi][nextYi][nextZi] != 0 &&
        (nextX < nextXi + 1.5 && nextY < nextYi + 1.5 && nextZ < nextZi + 1.5)) { //buffers to try and prevent clipping
        setViewPosition(currX, currY, currZ); //don't let us move
    }

    ////checking for skipping blocks
    //if (deltaX > 1) {
    //    for (i = 0; i < deltaX; i++) {
    //        if (world[nextXi + i][nextYi][nextZi] != 0) {
    //            setViewPosition(currX, currY, currZ);
    //        }
    //    }
    //}

    //if (deltaY > 1) {
    //    for (i = 0; i < deltaY; i++) {
    //        if (world[nextXi][nextYi+i][nextZi] != 0) {
    //            setViewPosition(currX, currY, currZ);
    //        }
    //    }
    //}

    //if (deltaZ > 1) {
    //    for (i = 0; i < deltaZ; i++) {
    //        if (world[nextXi][nextYi][nextZi + i] != 0) {
    //            setViewPosition(currX, currY, currZ);
    //        }
    //    }
    //}



}

//just calls glutBitmapCharacter for the length of the string. Not an actual glut function
void glutBitmapString(void *fontID, const unsigned char *string) {
    int len, i;
    len = strlen(string);
    for (i = 0; i < len; i++) {
        glutBitmapCharacter(fontID, string[i]);
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
        if (displayMap != 0) {
            int i, mapSize, mapScale, xOrigin, yOrigin, playerScale, humanScale, rayScale;
            float x, y, z, bx,by,ex,ey;
            char coords[100];
            GLfloat blue[] = { 0.0, 0.0, 1.0, 1.0 };
            GLfloat red[] = { 1.0, 0.0, 0.0, 1.0 };
            GLfloat green[] = { 0.0, 1.0, 0.0, 1.0 };
            GLfloat yellow[] = { 1.0, 1.0, 0.0, 1.0 };
            GLfloat purple[] = { 1.0, 0.0, 1.0, 1.0 };
            GLfloat orange[] = { 1.0, 0.64, 0.0, 1.0 };
            GLfloat white[] = { 1.0, 1.0, 1.0, 1.0 };
            GLfloat black[] = { 0.0, 0.0, 0.0, 1.0 };
            GLfloat grey[] = { 0.0, 0.0, 0.0, 0.5 };
            
            if (displayMap == 1) {
                playerScale = 6;
                mapScale = 2;
                humanScale = 4;
                rayScale = 2;
            }
            else if (displayMap == 2) {
                playerScale = 8;
                mapScale = 8;
                humanScale = 8;
                rayScale = 4;
            }
            mapSize = mapScale * 100;
            
            xOrigin = screenWidth - mapSize;
            yOrigin = screenHeight - mapSize;

            glDisable(GL_DEPTH_TEST);

            set2Dcolour(grey);
            draw2Dbox(screenWidth, screenHeight, xOrigin, yOrigin);
            set2Dcolour(red);
            draw2Dline(screenWidth, screenHeight, xOrigin, screenHeight, 1); // top line
            draw2Dline(screenWidth, yOrigin, xOrigin, yOrigin, 1); // bottom line
            draw2Dline(screenWidth, screenHeight, screenWidth, yOrigin, 1); //right line
            draw2Dline(xOrigin, screenHeight, xOrigin, yOrigin, 1); //left line

            for (i = 0; i < numHumans; i++) {
                set2Dcolour(white);
                draw2Dpoint( xOrigin + (humans[i].x * mapScale), (yOrigin + humans[i].z * mapScale), 4, 0);
            }
            getOldViewPosition(&x, &y, &z);

            //print coordinates to the screen
            snprintf(coords, sizeof(coords), "X: %2.2f Y: %2.2f Z: %2.2f", x*-1, y*-1, z*-1);

            set2Dcolour(white);
            draw2Dbox(0, 9, 215, 25); //draws a white box so you can see the text

            glDisable(GL_LIGHTING); //lets us add colour to the bitmap
            glRasterPos2i(0, 10); //position of the text
            glColor3f(0.0f, 0.0f, 0.0f); //colour of the text
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, coords); //draws the string to the screen
            glEnable(GL_LIGHTING); //reenables lighting

            x = (int)floor(x) * -1;
            y = (int)floor(z) * -1;
            draw2Dtriangle(xOrigin + (x * mapScale), yOrigin + (y * mapScale), 
                xOrigin + (x * mapScale) + playerScale, yOrigin + (y * mapScale), 
                xOrigin + (x * mapScale) + playerScale/2, yOrigin + (y * mapScale) + playerScale);
            
            set2Dcolour(red);
            for (i = 0; i < TUBE_COUNT; i++) {
                if (tubeVisible[i] == 1) {
                    bx = tubeData[i][0] * mapScale;
                    by = tubeData[i][2] * mapScale;
                    ex = tubeData[i][3] * mapScale;
                    ey = tubeData[i][5] * mapScale;
                    draw2Dline(xOrigin + bx, yOrigin + by, xOrigin + ex, yOrigin + ey, rayScale);
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
        int i;
        float currX, currY, currZ, nextX, nextY, nextZ, deltaX, deltaY, deltaZ;
        end = clock();

        //gravity things here
        if ((double)(end - gravityTimer) / CLOCKS_PER_SEC > .2) {

            if (numHumans > 0) { //moves all humans down 1 space every cycle
                //printf("gravity\n");
                for (i = 0; i < numHumans; i++) {
                    moveHuman(0, -1, 0, i);
                }
            }


            gravityTimer = end; //reset clock cycle
        }

        //fast animation but it smooth tho
        if ((double)(end - rayAnimationTimer) / CLOCKS_PER_SEC > .03) {
            //ray movement here
            for (i = 0; i < TUBE_COUNT; i++) {
                if (tubeVisible[i] == 1) {
                    moveTube(i);
                }
            }
            rayAnimationTimer = end;
        }

        //do momentum things here 
        if ((double)(end - momentumTimer) / CLOCKS_PER_SEC > .01) {
            getViewPosition(&nextX, &nextY, &nextZ);
            getOldViewPosition(&currX, &currY, &currZ);

            deltaX = (nextX - currX); //calculate the total distance moved
            deltaY = (nextY - currY);
            deltaZ = (nextZ - currZ);

            if (lastX == nextX && lastY == nextY && lastZ == nextZ) { //check last and new. If the same, no key press. Need to decelerate 
                setOldViewPosition(nextX, nextY, nextZ); //move the current position to the next position
                nextX += deltaX * decelMod;
                nextY += deltaY * decelMod;
                nextZ += deltaZ * decelMod;
                accelMod -= .5;
                if (accelMod < 1.2) accelMod = 1.0;
                setViewPosition(nextX, nextY, nextZ); //add the momentum to the next position
                collisionResponse();
            }
            else if (deltaX >= .1 || deltaY >= .1 || deltaZ >= .1) { //acceleration. Thershold of .1 so it ignores the movement of the deceleration causes
                setOldViewPosition(nextX, nextY, nextZ);
                nextX += deltaX * accelMod;
                nextY += deltaY * accelMod;
                nextZ += deltaZ * accelMod;
                accelMod += .1;
                if (accelMod > 3) accelMod = 3;
                setViewPosition(nextX, nextY, nextZ);
                collisionResponse();
            }
            lastX = nextX; //store the last position so we have something to compare to next step
            lastY = nextY;
            lastZ = nextZ;
            momentumTimer = end; //reset clock cycle
        }
    }
}


/* called by GLUT when a mouse button is pressed or released
   -button indicates which button was pressed or released
   -state indicates a button down or button up event
   -x,y are the screen coordinates when the mouse is pressed or
    released */
void mouse(int button, int state, int x, int y) {

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_UP) {

            //printf("left button - ");
            float x, y, z, xOr, yOr, zOr, xRot, yRot, bx, by, bz, ex, ey, ez;
            getOldViewPosition(&x, &y, &z);
            getViewOrientation(&xOr, &yOr, &zOr);


            xRot = xOr / 180 * 3.141592;
            yRot = yOr / 180 * 3.141592;

            //printf("sin(yRot) %2.2f, sin(xRot) %2.2f, cos(xRot) %2.2f\n", sin(yRot), sin(xRot), cos(xRot));

            ex = bx = fabs(x);
            ey = by = fabs(y);
            ez = bz = fabs(z);

            ex += sin(yRot);
            ey -= sin(xRot);
            ez -= cos(yRot);

            createTube(numTubes, bx, by, bz, ex, ey, ez, 3);
            tubeTimer[numTubes] = clock();
            numTubes++;
            if (numTubes >= TUBE_COUNT) numTubes = 0;
        }
    }
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
    int x, y, z, row, col, maxH, colour;

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
    //printf("%d %d %d\n", row, col, maxH);o
    for (x = 0; x < row - 1; x++) {
        for (z = col - 1; z >= 0; z--) {
            fscanf(fp, "%d", &y);
            y = (int)floor(y / 20);
            if (y < 5) {
                colour = 2;
            }
            else if (y > 30) {
                colour = 5;
            }
            else {
                colour = 1;
            }
            world[x][y][z] = colour;
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
        //glutSetCursor(GLUT_CURSOR_NONE);
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
        else {
            makeHuman(10, 40, 10);
            makeHuman(25, 35, 25);
            makeHuman(35, 49, 35);
            makeHuman(50, 43, 50);
        }
    }
    float newX, newY, newZ;
    getViewPosition(&newX, &newY, &newZ);
    setOldViewPosition(newX, newY, newZ); //make sure the old view position has valid values at the start
    lastX = newX;
    lastY = newY;
    lastZ = newZ;
    gravityTimer = clock();
    momentumTimer = clock();
    rayAnimationTimer = clock();
    /* starts the graphics processing loop */
    /* code after this will not run until the program exits */
    glutMainLoop();
    return 0;
}

