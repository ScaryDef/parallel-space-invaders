/* Space Invaders
   Timothy Pettit

   This program is modeled after the 80's arcade classic Space Invaders. A "tank" moves across
   the bottom of the screen and shoots at "aliens" that move across and down the screen while
   dropping bombs. The game is lost if the tank is hit by a bomb or an alien reaches
   the bottom of the screen. The game is won by shooting down all of the aliens. This game
   also features a menu which allows the user to adjust the overall speed of the game as well
   as the relative speeds of the individual elements.

   Original source code: https://github.com/Chaser324/invaders/blob/master/invaders.c
 */

 
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <time.h>
#include "invaderstructs.h"
#include "mpi.h"

#define MAX_BOMBS 1000

////////////////////////////////////////////////////////////////////////////////
// Added three constants to make the number of aliens flexible, making it easier
// to test with different alien amounts.
//
// Original code had a hardcoded number of aliens (30 in total, 3 rows)
#define ALIENS 55
#define ALIEN_ROWS 5
#define ALIEN_COLUMNS (ALIENS / ALIEN_ROWS)
////////////////////////////////////////////////////////////////////////////////

/* Function prototypes */
void menu(struct options *settings);
void gameover(int win);

////////////////////////////////////////////////////////////////////////////////
// Refactored some code by moving it from the main method to separate functions.
void init(struct options* settings, struct player* tank, struct alien* aliens,
        struct shoot* shot, struct bomb* bomb);
void move_bombs(struct bomb* bomb, int loops, struct options* settings, unsigned int* currentbombs);
void move_shots(struct shoot* shot, struct alien* aliens, int loops, struct options* settings,
        unsigned int* currentshots, unsigned int* currentaliens, int* score);
void move_alien(struct alien* alien, int* shoot_bomb, int loops,
        struct options* settings, struct player* tank, struct shoot* shot);

void mpi_move_aliens(struct alien* aliens, struct mpi_alien* mpi_aliens,
        struct bomb* bomb, unsigned int* currentbombs, MPI_Datatype type);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// MPI CODE
//
// cpu holds my processor number, cpu=0 is master, rest are slaves
// numcpus is the total number of processors
int cpu, numcpus;
////////////////////////////////////////////////////////////////////////////////

/* The main function handles user input, the game visuals, and checks for win/loss conditions */
int main(int argc, char** argv)
{
    ///////////////////////////////////////////////////////////
    // MPI Starter Code
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &cpu);
    MPI_Comm_size(MPI_COMM_WORLD, &numcpus);
    //////////////////////////////////////////////////////////
   
    struct player tank;
    struct alien aliens[ALIENS];
    struct shoot shot[3];
    struct bomb bomb[MAX_BOMBS];
    struct options settings;
    unsigned int input, loops = 0, i = 0, j = 0, currentshots = 0, currentbombs = 0, currentaliens = ALIENS;
    int random = 0, score = 0, win = -1;
    struct timespec tim, tim2;
    char tellscore[30];
    srand(time(NULL));
    
    
    ////////////////////////////////////////////////////////////////////////////
    // Used for passing required data to each cpu
    struct mpi_alien mpi_aliens[ALIENS];
 
    /* Need to create MPI data type for the struct mpi_alien type
       https://computing.llnl.gov/tutorials/mpi/#Derived_Data_Types */
    MPI_Aint lb;
    MPI_Aint extent;
    
    // MPI_Datatype for struct alien
    MPI_Datatype alien_type, alien_old_types[2];
    int alien_block_counts[2];
    MPI_Aint alien_offsets[2];
    alien_offsets[0] = 0;
    alien_old_types[0] = MPI_INT;
    alien_block_counts[0] = 7;
    MPI_Type_get_extent(MPI_INT, &lb, &extent);
    alien_offsets[1] = 7 * extent;
    alien_old_types[1] = MPI_CHAR;
    alien_block_counts[1] = 2;
    MPI_Type_create_struct(2, alien_block_counts, alien_offsets, alien_old_types, &alien_type);
    MPI_Type_commit(&alien_type);
    
    // MPI_Datatype for struct options
    MPI_Datatype options_type, options_old_types[1];
    int options_block_counts[1];
    MPI_Aint options_offsets[1];
    options_offsets[0] = 0;
    options_old_types[0] = MPI_INT;
    options_block_counts[0] = 5;
    MPI_Type_create_struct(1, options_block_counts, options_offsets, options_old_types, &options_type);
    MPI_Type_commit(&options_type);
    
    // MPI_Datatype for struct player
    MPI_Datatype player_type, player_old_types[2];
    int player_block_counts[2];
    MPI_Aint player_offsets[2];
    player_offsets[0] = 0;
    player_old_types[0] = MPI_INT;
    player_block_counts[0] = 2;
    MPI_Type_get_extent(MPI_INT, &lb, &extent);
    player_offsets[1] = 2 * extent;
    player_old_types[1] = MPI_CHAR;
    player_block_counts[1] = 1;
    MPI_Type_create_struct(2, player_block_counts, player_offsets, player_old_types, &player_type);
    MPI_Type_commit(&player_type);
    
    // MPI_Datatype for struct shoot
    MPI_Datatype shoot_type, shoot_old_types[2];
    int shoot_block_counts[2];
    MPI_Aint shoot_offsets[2];
    shoot_offsets[0] = 0;
    shoot_old_types[0] = MPI_INT;
    shoot_block_counts[0] = 3;
    MPI_Type_get_extent(MPI_INT, &lb, &extent);
    shoot_offsets[1] = 3 * extent;
    shoot_old_types[1] = MPI_CHAR;
    shoot_block_counts[1] = 1;
    MPI_Type_create_struct(2, shoot_block_counts, shoot_offsets, shoot_old_types, &shoot_type);
    MPI_Type_commit(&shoot_type);
    
    // MPI_Datatype for array of struct shoot
    MPI_Datatype shoot_array_type;
    MPI_Type_contiguous(3, shoot_type, &shoot_array_type);
    MPI_Type_commit(&shoot_array_type);
    
    // MPI_Datatype for struct mpi_alien
    MPI_Datatype mpi_alien_type, mpi_alien_old_types[5];
    int mpi_alien_block_counts[5];
    MPI_Aint mpi_alien_offsets[5];
    mpi_alien_offsets[0] = 0;
    mpi_alien_old_types[0] = alien_type;
    mpi_alien_block_counts[0] = 1;
    MPI_Type_get_extent(alien_type, &lb, &extent);
    mpi_alien_offsets[1] = 1 * extent;
    mpi_alien_old_types[1] = MPI_INT;
    mpi_alien_block_counts[1] = 2;
    MPI_Type_get_extent(MPI_INT, &lb, &extent);
    mpi_alien_offsets[2] = 2 * extent + mpi_alien_offsets[1];
    mpi_alien_old_types[2] = options_type;
    mpi_alien_block_counts[2] = 1;
    MPI_Type_get_extent(options_type, &lb, &extent);
    mpi_alien_offsets[3] = 1 * extent + mpi_alien_offsets[2];
    mpi_alien_old_types[3] = player_type;
    mpi_alien_block_counts[3] = 1;
    MPI_Type_get_extent(player_type, &lb, &extent);
    mpi_alien_offsets[4] = 1 * extent + mpi_alien_offsets[3];
    mpi_alien_old_types[4] = shoot_array_type;
    mpi_alien_block_counts[4] = 1;
    MPI_Type_create_struct(5, mpi_alien_block_counts, mpi_alien_offsets, mpi_alien_old_types, &mpi_alien_type);
    MPI_Type_commit(&mpi_alien_type);
    ////////////////////////////////////////////////////////////////////////////

    if (cpu == 0)
    {
        init(&settings, &tank, aliens, shot, bomb);
    }

    while (1)
    {
        if (cpu == 0)
        {
            /* Show score */
            sprintf(tellscore, "%d   ", score);
            move(0, 8);
            addstr(tellscore);

            /* Show number of aliens left */
            sprintf(tellscore, "Aliens left: %d   ", currentaliens);
            move(0, 15);
            addstr(tellscore);

            /* Move tank */
            move(tank.r, tank.c);
            addch(tank.ch);
        
            move_bombs(bomb, loops, &settings, &currentbombs);
            move_shots(shot, aliens, loops, &settings, &currentshots, &currentaliens, &score);
        
            ////////////////////////////////////////////////////////////////////
            // Draw the aliens onto the screen.
            // Originally the aliens were drawn inside the loop where the alien
            // positions are changed. We had to separate this process for the mpi
            // modifications, otherwise each core would be attempting to print
            // to the screen.
            for (i = 0; i < ALIENS; i++)
            {
                if (aliens[i].alive == 1)
                {
                    move(aliens[i].pr, aliens[i].pc);
                    addch(' ');

                    move(aliens[i].r, aliens[i].c);
                    addch(aliens[i].ch);

                    aliens[i].pr = aliens[i].r;
                    aliens[i].pc = aliens[i].c;
                }
            }
            ////////////////////////////////////////////////////////////////////
        }

        // prepare the data structure for the other cpus
        if (cpu == 0)
        {
            for (i = 0; i < ALIENS; i++)
            {
                mpi_aliens[i].alien = aliens[i];
                mpi_aliens[i].shoot_bomb = 0;
                mpi_aliens[i].loops = loops;
                mpi_aliens[i].settings = settings;
                mpi_aliens[i].tank = tank;
                mpi_aliens[i].shot[0] = shot[0];
                mpi_aliens[i].shot[1] = shot[1];
                mpi_aliens[i].shot[2] = shot[2];
            }
        }
        
        /* Move aliens in parallel */
        mpi_move_aliens(aliens, mpi_aliens, bomb, &currentbombs, mpi_alien_type);
        
        if (cpu == 0) {
        /* See if game has been won or lost*/
        if (currentaliens == 0)
        {
            win = 1;
            MPI_Bcast(&win, 1, MPI_INT, 0, MPI_COMM_WORLD);
            break;
        }
        for (i = 0; i < ALIENS; ++i)
        {
            if (aliens[i].r == LINES - 1)
            {
                win = 0;
                break;
            }
        }
        for (i = 0; i < MAX_BOMBS; ++i)
        {
            if (bomb[i].r == tank.r && bomb[i].c == tank.c)
            {
                win = 0;
                break;
            }
        }

        move(0, COLS - 1);
        refresh();
        tim.tv_sec = 0;
        tim.tv_nsec = settings.overall * 1000;
        nanosleep(&tim, &tim2);
        ++loops;

        input = getch();
        move(tank.r, tank.c);
        addch(' ');

        /* Check input */
        if (input == 'q')
        {
            win = 2;
        }
        else if (input == KEY_LEFT)
        {
            --tank.c;
            
            /* Check for valid tank position */
            if (tank.c < 0)
            {
                tank.c = 0;
            }
        }
        else if (input == KEY_RIGHT)
        {
            ++tank.c;
            
            /* Check for valid tank position */
            if (tank.c > COLS - 2)
            {
                tank.c = COLS - 2;
            }
        }
        else if (input == ' ' && currentshots < 3)
        {
            for (i = 0; i < 3; ++i)
            {
                if (shot[i].active == 0)
                {
                    shot[i].active = 1;
                    ++currentshots;
                    --score;
                    shot[i].r = LINES - 2;
                    shot[i].c = tank.c;
                    break;
                }
            }
        }
        else if (input == 'm')
        {
            menu(&settings);
        }

        if (win != -1)
        {
            MPI_Bcast(&win, 1, MPI_INT, 0, MPI_COMM_WORLD);
            break;
        }
        
        MPI_Bcast(&win, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        }
        else
        {
            int x = 0;
            MPI_Bcast(&x, 1, MPI_INT, 0, MPI_COMM_WORLD);
            
            if (x != -1) break;
        }
    }

    if (cpu == 0)
    {
        gameover(win);
        endwin();
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    // MPI Finish Code
    MPI_Type_free(&alien_type);
    MPI_Type_free(&player_type);
    MPI_Type_free(&options_type);
    MPI_Type_free(&shoot_type);
    MPI_Type_free(&shoot_array_type);
    MPI_Type_free(&mpi_alien_type);
    MPI_Finalize();
    ////////////////////////////////////////////////////////////////////////////////
   
    return 0;
}

/* This function handles the menu options available to the user. Unchanged from original code. */
void menu(struct options *settings)
{
    char option, buf[30];
    int newchoice;
    clear();
    echo();
    nocbreak();
    nodelay(stdscr, 0);
    move(0, 0);
    addstr("1. Change overall game speed");
    move(1, 0);
    addstr("2. Change alien motion speed");
    move(2, 0);
    addstr("3. Change tank shot speed");
    move(3, 0);
    addstr("4. Change alien bomb speed");
    move(4, 0);
    addstr("5. Change alien bomb dropping frequency");
    move(5, 0);
    addstr("6. Return to the game");
    move(6, 0);
    addstr("7. Exit the game");
    move(8, 0);
    addstr("Enter your option: ");
    refresh();

    while (1)
    {
        move(8, 19);
        option = getch();
        move(9, 0);
        deleteln();
        move(10, 0);
        deleteln();
        move(11, 0);
        deleteln();

        if (option == '1')
        {
            sprintf(buf, "Current value: %d\n", settings->overall);

            move(9, 0);
            addstr(buf);
            move(10, 0);
            addstr("Enter new value: ");
            move(10, 17);
            refresh();
            getch();
            getstr(buf);

            newchoice = atoi(buf);

            /* Check for valid new value */
            if (newchoice < 0)
            {
                move(11, 0);
                addstr("ERROR: Inalid value");
            }
            else
            {
                settings->overall = newchoice;
            }
        }
        else if (option == '2')
        {
            sprintf(buf, "Current value: %d\n", settings->alien);

            move(9, 0);
            addstr(buf);
            move(10, 0);
            addstr("Enter new value: ");
            move(10, 17);
            refresh();
            getch();
            getstr(buf);

            newchoice = atoi(buf);

            /* Check for valid new value */
            if (newchoice <= 0)
            {
                move(11, 0);
                addstr("ERROR: Inalid value");
            }
            else
            {
                settings->alien = newchoice;
            }
        }
        else if (option == '3')
        {
            sprintf(buf, "Current value: %d\n", settings->shots);

            move(9, 0);
            addstr(buf);
            move(10, 0);
            addstr("Enter new value: ");
            move(10, 17);
            refresh();
            getch();
            getstr(buf);

            newchoice = atoi(buf);

            /* Check for valid newchoice value */
            if (newchoice <= 0)
            {
                move(11, 0);
                addstr("ERROR: Inalid value");
            }
            else
            {
                settings->shots = newchoice;
            }
        }
        else if (option == '4')
        {
            sprintf(buf, "Current value: %d\n", settings->bombs);

            move(9, 0);
            addstr(buf);
            move(10, 0);
            addstr("Enter new value: ");
            move(10, 17);
            refresh();
            getch();
            getstr(buf);

            newchoice = atoi(buf);

            /* Check for valid new value */
            if (newchoice <= 0)
            {
                move(11, 0);
                addstr("ERROR: Inalid value");
            }
            else
            {
                settings->bombs = newchoice;
            }
        }
        else if (option == '5')
        {
            sprintf(buf, "Current value: %d\n", settings->bombchance);

            move(9, 0);
            addstr(buf);
            move(10, 0);
            addstr("Enter new value: ");
            move(10, 17);
            refresh();
            getch();
            getstr(buf);

            newchoice = atoi(buf);

            /* Check for valid new value */
            if (newchoice > 100 || newchoice < 0)
            {
                move(11, 0);
                addstr("ERROR: Inalid value");
            }
            else
            {
                settings->bombchance = newchoice;
            }
        }
        else if (option == '6')
        {
            break;
        }
        else if (option == '7')
        {
            endwin();
            exit(0);
        }
        else
        {
            move(9, 0);
            addstr("ERROR: Invalid selection");
            move(8, 19);
            addstr(" ");
            refresh();
        }
    }

    clear();
    noecho();
    cbreak();
    nodelay(stdscr, 1);

    move(0, (COLS / 2) - 9);
    addstr("--SPACE INVADERS--");
    move(0, 1);
    addstr("SCORE: ");
    move(0, COLS - 19);
    addstr("m = menu q = quit");
}

/* This function handles displaying the win/lose screen. Unchanged from original code. */
void gameover(int win)
{
    nodelay(stdscr, 0);

    if (win == 0)
    {
        clear();
        move((LINES / 2) - 1, (COLS / 2) - 5);
        addstr("YOU LOSE!");
        move((LINES / 2), (COLS / 2) - 11);
        addstr("PRESS ANY KEY TO EXIT");
        move(0, COLS - 1);
        refresh();
        getch();
    }
    else if (win == 1)
    {
        clear();
        move((LINES / 2) - 1, (COLS / 2) - 5);
        addstr("YOU WIN!");
        move((LINES / 2), (COLS / 2) - 11);
        addstr("PRESS ANY KEY TO EXIT");
        move(0, COLS - 1);
        refresh();
        getch();
    }
}


////////////////////////////////////////////////////////////////////////////////
// Initialization procedures.
//
// The only changes we made are to allow for a variable number of aliens
// to be initialized. The rest of the initializations remain unchanged.
////////////////////////////////////////////////////////////////////////////////
void init(struct options* settings, struct player* tank, struct alien* aliens,
        struct shoot* shot, struct bomb* bomb)
{
    unsigned int i = 0, j = 0, a = 0;
    
    /* ncurses initialization */
    initscr();
    clear();
    noecho();
    cbreak();
    nodelay(stdscr, 1);
    keypad(stdscr, 1);

    /* Set default settings */
    settings->overall = 15000;
    settings->alien = 12;
    settings->shots = 3;
    settings->bombs = 7;
    settings->bombchance = 5;

    /* Set tank settings */
    tank->r = LINES - 1;
    tank->c = COLS / 2;
    tank->ch = '^';

    /* Set aliens settings */
    for (i = 0; i < ALIEN_ROWS; i++)
    {
        for (j = 0; j < ALIEN_COLUMNS; j++)
        {
            a = i * ALIEN_COLUMNS + j;

            aliens[a].r = (i + 1) * 2;
            aliens[a].c = j * 5;
            aliens[a].pr = 0;
            aliens[a].pc = 0;
            aliens[a].alive = 1;
            aliens[a].behavior = WANDER;
            aliens[a].time = 0;
            aliens[a].direction = 'r';
            aliens[a].ch = ALIEN_SYMBOLS[aliens[a].behavior];
        }
    }

    /* Set shot settings */
    for (i = 0; i < 3; ++i)
    {
        shot[i].active = 0;
        shot[i].ch = '*';
    }

    /* Set bomb settings */
    for (i = 0; i < MAX_BOMBS; ++i)
    {
        bomb[i].active = 0;
        bomb[i].ch = 'o';
        bomb[i].loop = 0;
    }

    /* Display game title,score header,options */
    move(0, (COLS / 2) - 9);
    addstr("--SPACE INVADERS--");
    move(0, 1);
    addstr("SCORE: ");
    move(0, COLS - 19);
    addstr("m = menu q = quit");
}

/* Move bombs dropped by aliens. Unchanged from original code. */
void move_bombs(struct bomb* bomb, int loops, struct options* settings, unsigned int* currentbombs)
{
    unsigned int i = 0;
    
    if (loops % settings->bombs == 0)
    {
        for (i = 0; i < MAX_BOMBS; ++i)
        {
            if (bomb[i].active == 1)
            {
                if (bomb[i].r < LINES)
                {
                    if (bomb[i].loop != 0)
                    {
                        move(bomb[i].r - 1, bomb[i].c);
                        addch(' ');
                    }
                    else
                    {
                        ++bomb[i].loop;
                    }

                    move(bomb[i].r, bomb[i].c);
                    addch(bomb[i].ch);

                    ++bomb[i].r;
                }
                else
                {
                    bomb[i].active = 0;
                    bomb[i].loop = 0;
                    --(*currentbombs);
                    move(bomb[i].r - 1, bomb[i].c);
                    addch(' ');
                }
            }
        }
    }
}

/* Move the shots from the player. Unchanged from original code. */
void move_shots(struct shoot* shot, struct alien* aliens, int loops, struct options* settings,
        unsigned int* currentshots, unsigned int* currentaliens, int* score)
{
    unsigned int i = 0, j = 0;
    
    if (loops % settings->shots == 0)
    {
        for (i = 0; i < 3; ++i)
        {
            if (shot[i].active == 1)
            {
                if (shot[i].r > 0)
                {
                    if (shot[i].r < LINES - 2)
                    {
                        move(shot[i].r + 1, shot[i].c);
                        addch(' ');
                    }

                    for (j = 0; j < ALIENS; ++j)
                    {
                        if (aliens[j].alive == 1 && aliens[j].r == shot[i].r && aliens[j].pc == shot[i].c)
                        {
                            *score += 20;
                            aliens[j].alive = 0;
                            shot[i].active = 0;
                            --(*currentshots);
                            --(*currentaliens);
                            move(aliens[j].pr, aliens[j].pc);
                            addch(' ');
                            break;
                        }
                    }

                    if (shot[i].active == 1)
                    {
                        move(shot[i].r, shot[i].c);
                        addch(shot[i].ch);
                        --shot[i].r;
                    }
                }
                else
                {
                    shot[i].active = 0;
                    --(*currentshots);
                    move(shot[i].r + 1, shot[i].c);
                    addch(' ');
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// MPI function to move aliens
void mpi_move_aliens(struct alien* aliens, struct mpi_alien* mpi_aliens,
        struct bomb* bomb, unsigned int* currentbombs, MPI_Datatype type)
{
    int i = 0, j = 0, k = 0, slave;
    MPI_Status status;

    int numeach = ALIENS / numcpus;

    // I AM THE MASTER
    if (cpu == 0)
    {
        // send data to the slaves
        for (slave = 1; slave < numcpus; slave++)
        {
            MPI_Send(&mpi_aliens[numeach * slave], numeach, type, slave, 1, MPI_COMM_WORLD);
        }

        // master cpu does some operations too
        for (i = 0; i < numeach; i++)
        {
            move_alien(&mpi_aliens[i].alien,
                       &mpi_aliens[i].shoot_bomb,
                       mpi_aliens[i].loops,
                       &mpi_aliens[i].settings,
                       &mpi_aliens[i].tank,
                       mpi_aliens[i].shot);
        }

        //receive data from the slaves
        for (slave = 1; slave < numcpus; slave++)
        {
            MPI_Recv(&mpi_aliens[numeach * slave], numeach, type, slave, 2, MPI_COMM_WORLD, &status);
        }
        
        for (i = 0; i < ALIENS; i++)
        {
            // copy alien data back into the original array
            aliens[i] = mpi_aliens[i].alien;
            
            if (mpi_aliens[i].shoot_bomb == 1)
            {
                // from original code: alien shoots a bomb
                for (j = k; j < MAX_BOMBS; ++j)
                {
                    if (bomb[j].active == 0)
                    {
                        bomb[j].active = 1;
                        ++(*currentbombs);
                        bomb[j].r = aliens[i].r + 1;
                        bomb[j].c = aliens[i].c;
                        break;
                    }
                }
                k = j;
            }
        }
    }
    // I AM A SLAVE
    else
    {
        struct mpi_alien data[numeach];
        // receive data from the master
        MPI_Recv(&data[0], numeach, type, 0, 1, MPI_COMM_WORLD, &status);
        
        for (i = 0; i < numeach; i++)
        {
            move_alien(&data[i].alien,
                       &data[i].shoot_bomb,
                       data[i].loops,
                       &data[i].settings,
                       &data[i].tank,
                       data[i].shot);
        }
        
        // send the new data back to the master
        MPI_Send(&data[0], numeach, type, 0, 2, MPI_COMM_WORLD);
    }
}
////////////////////////////////////////////////////////////////////////////////

/* Calculate next position of an alien and whether it shot a bomb. */
void move_alien(struct alien* alien, int* shoot_bomb, int loops,
        struct options* settings, struct player* tank, struct shoot* shot)
{
    if (loops % settings->alien == 0)
    {
        if (alien->alive == 1)
        {
            ////////////////////////////////////////////////////////////////////
            // This is the start of our main modifications to allow
            // aliens to have different behaviors.
            //
            // In the original code, the alien would simply move 1 column left
            // or right in every iteration, and move down 1 row when it touches
            // one of the screen boundaries.
            //
            
            int random = rand() % 2500;
            
            /* Aliens change behavior randomly. The greater the length
               of the current behavior, the greater the chance to
               change behavior */
            if (random < alien->time)
            {
                random = -1;
                
                while (random == -1)
                {
                    // choose a new behavior by random
                    random = rand() % TOTAL_BEHAVIORS;
                    
                    // make it have a less chance of choosing WALL
                    if (random == WALL)
                    {
                        if (rand() % 10 != 0) random = -1;
                    }
                }
                
                alien->behavior = random;
                alien->time = 0;
                alien->ch = ALIEN_SYMBOLS[alien->behavior];
            }
            
            
            if (alien->behavior == WANDER)
            {
                random = rand() % 5;
                
                // on every 15 loops in this behavior, the alien has a 20% chance of switching direction to left or right
                if (alien->time % 15 == 0 && random == 0)
                {
                    if (alien->direction == 'l')
                    {
                        alien->direction = 'r';
                    }
                    else
                    {
                        alien->direction = 'l';
                    }
                }
                
                /*set alien next position*/
                if (alien->direction == 'l')
                {
                    if (alien->c == 0)  // if at the leftmost column
                    {
                        ++alien->c;
                        alien->direction = 'r';
                    }
                    else
                    {
                        --alien->c;
                    }
                }
                else if (alien->direction == 'r')
                {
                    if (alien->c == COLS - 2)  // if at the rightmost column
                    {
                        --alien->c;
                        alien->direction = 'l';
                    }
                    else
                    {
                        ++alien->c;
                    }
                }
                
                // see if alien shoots a bomb
                if (settings->bombchance - (1 + rand() % 100) >= 0)
                {
                    *shoot_bomb = 1;
                }
            }
            else if (alien->behavior == FOLLOW)
            {
                
                if (alien->c < tank->c)
                    ++alien->c;
                else if (alien->c > tank->c)
                    --alien->c;
                
                // drops bombs when the player and the alien are in close proximity (nearby columns)
                if (settings->bombchance - (1 + rand() % 100) >= 0 && abs(alien->c - tank->c) <= 5)
                {
                    *shoot_bomb = 1;
                }
            }
            else if (alien->behavior == DODGE)
            {
                
                int dodged = 0; // did the alien make a dodge move?
                int j = 0;
                
                for (j = 0; j < 3; ++j)
                {
                    // if the player's shot is active and in an adjacent column, and in a nearby row
                    if (shot[j].active == 1 && abs(shot[j].c - alien->c) <= 1 && shot[j].r > alien->r && shot[j].r - alien->r <= 5 && dodged == 0)
                    {
                        if (alien->c >= COLS - 3)  // right-boundary check
                        {
                            alien->c -= 2;
                            alien->direction = 'l';
                        }
                        else if (alien->c <= 1)  // left-boundary check
                        {
                            alien->c += 2;
                            alien->direction = 'r';
                        }
                        // alien is somewhere in the middle (not at a screen boundary)
                        else
                        {
                            random = rand() % 2;
                            
                            // dodge to a random direction
                            if (random == 0)
                            {
                                alien->c += 2;
                                alien->direction = 'r';
                            }
                            else
                            {
                                alien->c -= 2;
                                alien->direction = 'l';
                            }
                        }
                        
                        dodged = 1;
                    }
                }
                
                // if alien did not need to dodge a bullet, move normally
                if (dodged == 0)
                {
                    if (alien->direction == 'l')
                    {
                        if (alien->c == 0)  // if at the leftmost column
                        {
                            ++alien->c;
                            alien->direction = 'r';
                        }
                        else
                        {
                            --alien->c;
                        }
                    }
                    else if (alien->direction == 'r')
                    {
                        if (alien->c == COLS - 2)  // if at the rightmost column
                        {
                            --alien->c;
                            alien->direction = 'l';
                        }
                        else
                        {
                            ++alien->c;
                        }
                    }
                }
            }
            else if (alien->behavior == WALL)
            {
                
                // just constantly shoots bombs
                *shoot_bomb = 1;
            }
            
            /* Timer for all aliens to move down a row. Don't move down on the first loop of the game */
            if (loops % 100 == 0 && loops != 0)
            {
                ++alien->r;
            }
            
            /* Increase number of loops the alien has had their behavior for */
            alien->time++;
            ////////////////////////////////////////////////////////////////////
        }
    }
}




