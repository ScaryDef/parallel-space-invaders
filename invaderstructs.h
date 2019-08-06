/* Space Invaders
   
   Original source code: https://github.com/Chaser324/invaders/blob/master/invaderstructs.h
*/

/* header file for invaders.c */

struct player
{
    int r, c;
    char ch;
};

////////////////////////////////////////////////////////////////////////////////
// Modifications to this struct - added 'behavior' and 'time' fields
////////////////////////////////////////////////////////////////////////////////
struct alien
{
    int r, c;       /* row and column */
    int pr, pc;     /* previous row and column */
    int alive;      /* 1=alive 0=destroyed */
    int behavior;   /* a value from enum behavior */
    int time;       /* number of loops this alien has had the behavior for */
    char direction; /* direction of general movement, 'l'=left 'r'=right */
    char ch;        /* graphical representation for the alien */
};

struct shoot
{
    int r, c;
    int active; /* 1=active 0=inactive */
    char ch;
};

struct bomb
{
    int r, c;
    int active; /* 1=active 0=inactive */
    int loop;   /* used to prevent alien from flashing when bomb is dropped */
    char ch;
};

struct options
{
    int overall, alien, shots, bombs, bombchance;
};


////////////////////////////////////////////////////////////////////////////////
// Structure to hold values that will be required for each cpu to operate on an alien
struct mpi_alien
{
    struct alien alien;
    int shoot_bomb;
    int loops;
    struct options settings;
    struct player tank;
    struct shoot shot[3];
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Types of behaviors (movements) an alien can have
//
// WANDER - The alien will appear to wander the screen left and right
// FOLLOW - The alien will follow the player until it is in the same column as the player
// DODGE - The alien will attempt to dodge player bullets that are nearby
// WALL - The alien will form a wall of bombs which will be difficult for the player to move across
enum behavior
{
    WANDER = 0,
    FOLLOW = 1,
    DODGE = 2,
    WALL = 3
};

/* Total number of possible alien behaviors */
const int TOTAL_BEHAVIORS = 4;

/* Use the behavior enum as an index to get the character for the alien */
const char ALIEN_SYMBOLS[] = {'#', 'V', '+', 'T'};
////////////////////////////////////////////////////////////////////////////////
