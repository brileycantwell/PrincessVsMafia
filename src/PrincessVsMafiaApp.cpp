// Briley Cantwell 9/12/2016
// Princess vs Mafia is a puzzle game where the player must guide a puppy and princess through a series of rooms while avoiding enemies.
// The executable (.exe) is available in the "Release" folder

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

#include "cinder/TriMesh.h"
#include "cinder/Camera.h"
#include "Resources.h"
#include <math.h>

using namespace ci;
using namespace ci::app;
using namespace std;

// return random integer, inclusive of low and high
int randomInt(int low, int high)
{
    return low + (rand() % (high - low + 1));
}

// int to string
string toString(int in)
{
	return to_string(static_cast<long double>(in));
}

// float to string
string toString(float in)
{
	return to_string(static_cast<long double>(in));
}

// string to int
int toInt(string str)
{
	return atoi(str.c_str());
}

// string to float
float toFloat(string str)
{
	return atof(str.c_str());
}

// Room represents one cell of the grid, arranged in a 2D matrix
struct Room
{
	Room() { for( int i = 0; i < 4; i++ ) doors[i] = -1; }

	int doors[4]; // integer representations of the types of the doors connecting this room to other rooms.
	// indices are 0:left, 1:up, 2:right, 3:down
	// -1 represents no door (for edge or corner rooms)
	// 0 through 5 represent different designs of doors
};

// Enemy represents all moving creatures other than the princess and the puppy
struct Enemy
{
	int type; // 0 robot, 1 mobster, 2 bomber, 3 bomb, 4 starchaser

	int x, y; // the indices into the 2D array of rooms, representing the room that this enemy is currently in
	int dx, dy; // the screen coordinates of where this enemy currently is
	int dir; // the direction that this enemy is going to move (0:left, 1:up, 2:right, 3:down)

	int ox, oy; // the original x and y indices into the 2D array of rooms, where this enemy first started. Stored in case of reset
};

// An element in the queue solver: for breadth-first searching for solutions
struct Solver
{
	vector< int > moves; // the list of moves that brought us to this state (0:left, 1:up, 2:right, 3:down)
	vector< Enemy* > ens; // the current arrangement of enemies in this state
	int princessx, princessy, puppyx, puppyy, puppydir; // the current positions of the princess and puppy
	bool puppyatstar; // whether or not the puppy has reached the star in this state
};


class PrincessVsMafiaApp : public AppNative {
  public:
	void prepareSettings( Settings* settings );
	void setup();
	void keyDown( KeyEvent event );
	void keyUp( KeyEvent event );
	void update();
	void draw();

	void loadTextures(); // load in the sprites

	void generateRooms(); // create a new arrangement of doors, enemies, star, princess, and puppy
	void reversePossible(); // helper function for door generation algorithm
	void resetRooms(); // returns the princess, puppy, and enemies to their original locations
	void decideEnemies(); // determines the upcoming actions of all enemies and stores them in internal variables
	void SOdecideEnemies( Solver* so ); // determines enemy actions for the solver algorithm's variables

	void initializeSolver( Solver* so, int dir ); // sets default values for the solver
	void updateSolving(); // takes the first solver's state in the queue, and spawns four new states from each move direction (left/up/right/down), placing the new states at the end of the queue
	void simulateLastMove( Solver* so ); // updates the positions of the princess, puppy, and enemies based on the most recent move added to this solver
	void cleanAndKillSolver( Solver* so ); // equivalent to a destructor for Solver
	Solver* makeACopy( Solver* so ); // equivalent to a copy constructor for Solver
	int closeness( Solver* so ); // returns how "close" this Solver is from finding a successful solution.
	bool identical( Solver* s1, Solver* s2 ); // returns true if the two solvers given as arguments represent identical states, false otherwise
	void purgeSolvers(); // deletes solvers that don't have much of a chance of reaching a solution, in order to speed up the solving algorithm
	bool isValid(Solver* so); // returns true if the solver argument represents a unique state where the princess and puppy have chances of reaching the star, false otherwise
	bool tryForSprint(int cl); // if the current solver is close to finishing, try sprinting straight to the star, ignoring enemies.


	int px, py; // the x and y coordinates of the princess in the 2D array of rooms
	int pox, poy; // the original x and y coordinates for the princess, stored in case of reset
	int pdx, pdy; // the screen coordinates of where the princess is drawn
	int pdir; // the direction the princess is moving in (0:left, 1:up, 2:right, 3:down)
	int puppyx, puppyy, puppyox, puppyoy; // the x and y coordinates of the puppy, and the original x and y coordinates in case of reset
	int puppydx, puppydy, puppydir; // the screen coordinates and move direction of the puppy
	bool puppyatstar; // stores true if the puppy has reached the star, false otherwise
	bool SOpuppyatstar; // stores true if the puppy has reached the star for any solver in the queue, false otherwise

	Room rooms[10][10]; // the 2D array of rooms, stores the arrangement of doors for all rooms
	int width, height; // the number of rooms horizontally and vertically for the current puzzle
	vector< int > possible; // used in the door generation algorithm so that no room has two of the same door
	int nodoor1, nodoor2; // integers representing the 2 of 6 door designs that do not appear in the current puzzle
	int opendoor, SOopendoor;
	int origxx, origyy;
	int moving;
	int starx, stary, stardx, stardy;
	bool showplayer;

	vector< Enemy* > enemies;
	
	Vec2f trans;
	int transitionphase;

	int exx, exy, extimer, extype;

	bool solving;
	vector< Solver* > solvers;
	vector< Solver* > zombieSolvers;
	vector< int > winmoves;
	int maxSolversNeeded;
	bool showingSolution;
	int showingSolutionIndex;
	int showingSolutionCounter;
	int circlersRemoved, duplicatesRemoved;
	bool puppyCleared;
	int bestProgress;
	bool purgedAnything;
	int timesSuperPurged;
	bool solvedBySprint;
	bool solvedDialogUp;

	bool keystates[30]; // stores true if key is down, false otherwise
	bool keypresses[30]; // stores true if key has just been hit, false otherwise

	gl::Texture tex[3];
	gl::Fbo fbo;

	int sx;
	int hsx;
	int realsx;
	int sy;
	int hsy;

	Font fo, fobig;
	Color black;
};

void PrincessVsMafiaApp::prepareSettings( Settings* settings )
{
	settings->setFullScreen();
	settings->setFrameRate(60.0f); // capped at 60 frames per second
	settings->setResizable(false);
}

void PrincessVsMafiaApp::setup()
{
	srand(static_cast<unsigned int>(time(0))); // random seed set to start time

	loadTextures(); // load in the sprites
	
	solvedDialogUp = false; // the solver algorithm has not begun

	sx = this->getWindowWidth() - 256; // the screen's width in pixels, minus the width of the instructions
	sy = this->getWindowHeight(); // the screen's height in pixels
	realsx = sx + 256; // the actual screen's width in pixels
	hsx = sx/2; // half screen's width, stored for convenient drawing
	hsy = sy/2; // half screen's height, stored for convenient drawing

	fo = Font("Lucida Console", 28); // initialize two font sizes for convenient drawing
	fobig = Font("Lucida Console", 72);
	black = Color(0,0,0);

	for( int i = 0; i < 30; i++ ) // keys are initially up (not pressed)
	{	keystates[i] = false; keypresses[i] = false; }

	generateRooms(); // create a new arrangement of doors, enemies, star, princess, and puppy

	this->hideCursor(); // hide the mouse cursor (it is not needed)
}

void PrincessVsMafiaApp::keyDown( KeyEvent event )
{
	// keystates stores true is key is down, false otherwise
	// keypresses stores true is key has just been hit, false otherwise

	if ( event.getCode() == event.KEY_LEFT )
	{
		if ( !keystates[0] )
		{	keypresses[0] = true; }
		keystates[0] = true;
	}
	if ( event.getCode() == event.KEY_RIGHT )
	{
		if ( !keystates[2] )
		{	keypresses[2] = true; }
		keystates[2] = true;
	}
	if ( event.getCode() == event.KEY_UP )
	{
		if ( !keystates[1] )
		{	keypresses[1] = true; }
		keystates[1] = true;
	}
	if ( event.getCode() == event.KEY_DOWN )
	{
		if ( !keystates[3] )
		{	keypresses[3] = true; }
		keystates[3] = true;
	}

	if ( event.getCode() == event.KEY_SPACE )
	{
		if ( !keystates[4] )
		{	keypresses[4] = true; }
		keystates[4] = true;
	}
	if ( event.getCode() == event.KEY_p )
	{
		if ( !keystates[5] )
		{	keypresses[5] = true; }
		keystates[5] = true;
	}

	if ( event.getCode() == event.KEY_s )
	{
		if ( !keystates[6] )
		{	keypresses[6] = true; }
		keystates[6] = true;
	}

	if ( event.getCode() == event.KEY_a )
	{
		//if ( !keystates[7] )
			keypresses[7] = true;
		keystates[7] = true;
	}
	if ( event.getCode() == event.KEY_d )
	{
		//if ( !keystates[8] )
			keypresses[8] = true;
		keystates[8] = true;
	}

	if ( event.getCode() == event.KEY_q )
	{
		if ( !keystates[9] )
		{	keypresses[9] = true; }
		keystates[9] = true;
	}

	if ( event.getCode() == event.KEY_ESCAPE )
		quit();
}

void PrincessVsMafiaApp::keyUp( KeyEvent event )
{
	// keystates stores true is key is down, false otherwise
	// keypresses stores true is key has just been hit, false otherwise

	if ( event.getCode() == event.KEY_LEFT )
		keystates[0] = false;
	if ( event.getCode() == event.KEY_RIGHT )
		keystates[2] = false;
	if ( event.getCode() == event.KEY_UP )
		keystates[1] = false;
	if ( event.getCode() == event.KEY_DOWN )
		keystates[3] = false;
	if ( event.getCode() == event.KEY_SPACE )
		keystates[4] = false;
	if ( event.getCode() == event.KEY_p )
		keystates[5] = false;
	if ( event.getCode() == event.KEY_s )
		keystates[6] = false;
	if ( event.getCode() == event.KEY_a )
		keystates[7] = false;
	if ( event.getCode() == event.KEY_d )
		keystates[8] = false;
	if ( event.getCode() == event.KEY_q )
		keystates[9] = false;
}

void PrincessVsMafiaApp::update()
{
	if ( solvedDialogUp ) // dialog box shows that the solver algorithm is complete
	{
		if ( keypresses[4] ) // SPACE: close dialog
			solvedDialogUp = false;
		if ( keypresses[6] && winmoves[0] > -1 ) // 'S': see solution, if a solution was possible
		{
			solvedDialogUp = false;
			resetRooms();
			showingSolution = true; // show the algorithm's solution
			showingSolutionIndex = 0;
			showingSolutionCounter = 1;
		}
		if ( keypresses[5] ) // 'P': new puzzle
		{
			solvedDialogUp = false;
			transitionphase = 1; // begin sliding the current screen downwards to generate a new puzzle
		}

		for( int i = 0; i < 30; i++ ) // key presses have already been registered, set them to false so they don't keep firing 
			keypresses[i] = false;
		return;
	}

	if ( transitionphase != 0 ) // Either a new screen is entering from above, or an old screen is leaving below
	{
		if ( transitionphase == 2 ) // new screen entering from above
		{
			trans.y += 45.0f;
			if ( trans.y >= 0 )
			{
				trans.y = 0.0f;
				transitionphase = 0; // done with transition
			}
		}
		else if ( transitionphase == 1 ) // old screen leaving below
		{
			trans.y += 45.0f;
			if ( trans.y >= sy )
			{
				generateRooms(); // create new screen to enter from above
			}
		}
	}
	else if ( moving == 0 && showingSolution && showingSolutionCounter > 0 ) // buffer time between moves showing the algorithm's solution. User input is blocked during this phase.
	{
		showingSolutionCounter--;
	}
	else if ( moving == 0 && ((keypresses[0] || keypresses[1] || keypresses[2] || keypresses[3]) || showingSolution ) ) // Either the user has pressed a movement key, or the algorithm's solution is ready to show next step
	{
		int which; // what move should be processed for the princess (0:left, 1:up, 2:right, 3:down)
		if ( showingSolution ) // show the next step of the algorithm's solution. User input is blocked during this phase
		{
			showingSolutionCounter = 40;
			if ( showingSolutionIndex >= winmoves.size() )
				quit();
			which = winmoves[showingSolutionIndex];
			showingSolutionIndex++;
		}
		else // the user inputs a direction for the princess to move in
		{
			if ( keypresses[0] ) // left
			{	which = 0; }
			else if ( keypresses[1] ) // up
			{	which = 1; }
			else if ( keypresses[2] ) // right
			{	which = 2; }
			else
			{	which = 3; } // down
		}

		int tx = px; // tx is the desired x position of the princess, px is the current x position
		int ty = py; // ty is the desired y position of the princess, py is the current y position
		if ( which == 0 )
		{	which = 0; tx--; } // left
		else if ( which == 1 )
		{	which = 1; ty--; } // up
		else if ( which == 2 )
		{	which = 2; tx++; } // right
		else
		{	which = 3; ty++; } // down

		if ( tx >= 0 && tx < width && ty >= 0 && ty < height ) // ensure that the user is not moving the princess out of bounds
		{
			bool enthere = false; // is there an enemy in the desired position?
			for( int i = 0; i < enemies.size(); i++ )
			{
				if ( enemies[i]->x == tx && enemies[i]->y == ty )
				{	enthere = true; break; }
			}
			if ( !enthere ) // there are no enemies in the desired position
			{
				pdir = which; // store the direction of movement
				opendoor = rooms[py][px].doors[which]; // set the opendoor integer to the type of door that is opened

				bool enpup = false; // is there an enemy in the puppy's path?
				puppydir = -1;
				if ( !puppyatstar ) // the puppy has not yet reached the star
				{
					for( int k = 0; k < 4; k++ )
					{
						if ( rooms[puppyy][puppyx].doors[k] == opendoor )
							puppydir = k; // set the puppy to move through any open door
					}
					int pupdesx = puppyx; // pupdesx is the desired x position of the princess, puppyx is the current x position
					int pupdesy = puppyy; // pupdesy is the desired y position of the princess, puppyy is the current y position
					if ( puppydir == 0 ) // left
						pupdesx--;
					else if ( puppydir == 1 ) // up
						pupdesy--;
					else if ( puppydir == 2 ) // right
						pupdesx++;
					else if ( puppydir == 3 ) // down
						pupdesy++;

					for( int i = 0; i < enemies.size(); i++ )
					{
						if ( enemies[i]->x == pupdesx && enemies[i]->y == pupdesy )
						{	enpup = true; break; } // there is an enemy in the puppy's path
					}
				}
				
				if ( enpup ) // there is an enemy in the puppy's path, so this move is illegal and will not occur
				{
					opendoor = -1; // close all doors
					exx = puppydx; // trigger a visual indication that the puppy cannot move this way
					exy = puppydy;
					extimer = 1;
					extype = 0;
				}
				else // if ( !enpup ) // there are no enemies in the puppy's path, proceed with the move
				{
					decideEnemies(); // determines the upcoming actions of all enemies and stores them in internal variables
					moving = 1; // start movement on screen
				}
			}
			else // there is an enemy in the desired position for the princess, so this move is illegal and will not occur
			{
				exx = pdx; // trigger a visual indication that the princess cannot move this way
				exy = pdy;
				extimer = 1;
				extype = 0;
			}
		}
		else // the user is trying to move the princess out of bounds, so this move is illegal and will not occur
		{
			exx = pdx; // trigger a visual indication that the princess cannot move this way
			exy = pdy;
			extimer = 1;
			extype = 0;
		}
	}
	else if ( moving > 0 ) // movement has begun. Update princess, puppy, and enemy movement until complete
	{
		if ( pdir == 0 ) // princess is moving left
			pdx -= 8;
		else if ( pdir == 1 ) // princess is moving up
			pdy -= 8;
		else if ( pdir == 2 ) // princess is moving right
			pdx += 8;
		else if ( pdir == 3 ) // princess is moving down
			pdy += 8;
		if ( !puppyatstar ) // if the puppy has not yet reached the star
		{
			if ( puppydir == 0 ) // puppy is moving left
				puppydx -= 8;
			else if ( puppydir == 1 ) // puppy is moving up
				puppydy -= 8;
			else if ( puppydir == 2 ) // puppy is moving right
				puppydx += 8;
			else if ( puppydir == 3 ) // puppy is moving down
				puppydy += 8;
		}

		for( int i = 0; i < enemies.size(); i++ ) // loop through all enemies
		{
			if ( enemies[i]->dir == 0 ) // enemy is moving left
				enemies[i]->dx -= 8;
			else if ( enemies[i]->dir == 1 ) // enemy is moving up
				enemies[i]->dy -= 8;
			else if ( enemies[i]->dir == 2 ) // enemy is moving right
				enemies[i]->dx += 8;
			else if ( enemies[i]->dir == 3 ) // enemy is moving down
				enemies[i]->dy += 8;
		}

		moving++;
		if ( moving == 13 ) // moving is complete
		{
			moving = 0;
			opendoor = -1; // close all doors
			if ( pdir == 0 ) // update the princess' indices in the 2D matrix
				px--;
			else if ( pdir == 1 )
				py--;
			else if ( pdir == 2 )
				px++;
			else if ( pdir == 3 )
				py++;
			if ( !puppyatstar )
			{
				if ( puppydir == 0 ) // update the puppy's indices in the 2D matrix
					puppyx--;
				else if ( puppydir == 1 )
					puppyy--;
				else if ( puppydir == 2 )
					puppyx++;
				else if ( puppydir == 3 )
					puppyy++;
			}

			for( int i = 0; i < enemies.size(); i++ ) // update all enemy's indices in the 2D matrix
			{
				if ( enemies[i]->dir == 0 )
					enemies[i]->x--;
				else if ( enemies[i]->dir == 1 )
					enemies[i]->y--;
				else if ( enemies[i]->dir == 2 )
					enemies[i]->x++;
				else if ( enemies[i]->dir == 3 )
					enemies[i]->y++;
			}

			if ( !puppyatstar && puppyx == starx && puppyy == stary ) // if the puppy has just reached the star
			{
				puppyatstar = true;
				exx = puppydx; // puppy disappears in a burst of stars
				exy = puppydy;
				extimer = 1;
				extype = 1;
			}

			if ( puppyatstar && px == starx && py == stary ) // if the princess has just reached the star after the puppy already did
			{
				exx = pdx; // the princess disappears in a burst of stars
				exy = pdy;
				extimer = 1;
				extype = 2;
				showplayer = false;
			}
		}
	}
	else if ( keypresses[6] && winmoves.size() > 0 && winmoves[0] > -1 && !showingSolution ) // pressed 'S' to begin showing the algorithm's solution
	{
		resetRooms();
		showingSolution = true;
		showingSolutionIndex = 0;
		showingSolutionCounter = 1;
	}



	if ( keypresses[4] && transitionphase == 0 && !showingSolution ) // SPACE: return the princess, puppy, and all enemies to their original positions
		resetRooms();
	if ( keypresses[5] && !showingSolution && transitionphase == 0 ) // P: generate a new puzzle, so start by removing the old screen downward
		transitionphase = 1;

	if ( keypresses[6] && !solving && winmoves.size() == 0 && !showingSolution ) // pressed 'S' to start running the solver algorithm
	{
		resetRooms();
		solving = true;
		for( int i = 0; i < 3; i++ ) // start by pushing three solver states onto the queue. These three states are the results of pressing left, up, and right from the start.
		{							// Since the princess always starts at the bottom of the screen, it is not necessary to consider a down movement from the start
			solvers.push_back( new Solver() );
			initializeSolver( solvers[solvers.size()-1], i );
			simulateLastMove(solvers[solvers.size()-1]);
			if ( !isValid(solvers[solvers.size()-1]) ) // if this solver's state is not valid (out-of-bounds, puppy or princess hitting enemy), remove it from the queue.
			{
				cleanAndKillSolver(solvers[solvers.size()-1]);
				solvers.pop_back();
			}
		}
		// initialize solving statistics:
		maxSolversNeeded = 3;
		circlersRemoved = 0;
		duplicatesRemoved = 0;
		puppyCleared = false;
		bestProgress = 10000;
		purgedAnything = false;
		timesSuperPurged = 0;
		solvedBySprint = false;
		disableFrameRate(); // disable the frame rate so the algorithm can run at max speed
	}
	else if ( solving ) // currently running the solver algorithm
	{
		if ( keypresses[4] ) // SPACE: cancel solver algorithm
		{
			for( int i = 0; i < solvers.size(); i++ )
				cleanAndKillSolver( solvers[i] );
			solvers.clear();
			for( int i = 0; i < zombieSolvers.size(); i++ )
				cleanAndKillSolver( zombieSolvers[i] );
			zombieSolvers.clear();
			solving = false;
			winmoves.clear();
			setFrameRate(60);
			return;
		}
		// if we don't cancel, continue with the solver algorithm:
		updateSolving();
	}

	if ( extimer > 0 ) // update the visual indications of a collision with an enemy, or the animated stars of successfully reaching the star
	{
		extimer++;
		if ( extimer >= 30 )
		{
			extimer = 0;
			if ( extype == 2 ) // if the princess and puppy already reached the star
			{
				if ( !showingSolution ) // the user, not the algorithm, solved this puzzle
					transitionphase = 1; // generate a new puzzle
				else
				{
					showingSolution = false; // the alorithm solved this puzzle, reset it
					showplayer = true;
					resetRooms();
				}
			}
		}
	}


	for( int i = 0; i < 30; i++ ) // key presses have already been registered, set them to false so they don't keep firing
		keypresses[i] = false;
}

void PrincessVsMafiaApp::draw()
{
	gl::clear( Color(1.0f,1.0f,0.8f) ); // set the background color

	int xx = origxx;
	int yy = origyy;

	for( int i = 0; i < height; i++ ) // draw the outlines of each room
	{
		for( int k = 0; k < width; k++ )
		{
			gl::color(0,0,0);
			gl::drawStrokedRect( Rectf( xx, yy, xx+96, yy+96 ) + trans );

			xx += 96;
		}
		xx = origxx;
		yy += 96;
	}
	
	xx = origxx;
	yy = origyy;

	for( int i = 0; i < height; i++ ) // draw all doors
	{
		for( int k = 0; k < width; k++ )
		{
			gl::color(1,1,1);
			// right door - 2 vertical
			if ( k != width-1 )
			{
				if ( rooms[i][k].doors[2] != opendoor )
					gl::draw( tex[1], Area(0,0,16,64) + Vec2i(16*rooms[i][k].doors[2],0), Rectf(xx+88, yy+16, xx+104, yy+80) + trans );
				else
				{
					gl::color(1.0f,1.0f,0.8f);
					gl::draw( tex[1], Area(0,64,16,128), Rectf(xx+88, yy+16, xx+104, yy+80) + trans );
				}
			}
			gl::color(1,1,1);
			// bottom door - 3 horizontal
			if ( i != height-1 )
			{
				if ( rooms[i][k].doors[3] != opendoor )
					gl::draw( tex[1], Area(96,0,160,16) + Vec2i(0,16*rooms[i][k].doors[3]), Rectf(xx+16, yy+88, xx+80, yy+104) + trans );
				else
				{
					gl::color(1.0f,1.0f,0.8f);
					gl::draw( tex[1], Area(0,64,64,80), Rectf(xx+16, yy+88, xx+80, yy+104) + trans );
				}
			}

			xx += 96;
		}
		xx = origxx;
		yy += 96;
	}


	gl::enableAlphaBlending();

	// draw star
	gl::color(1,1,1);
	gl::draw( tex[1], Area(160, 0, 256, 96), Rectf( stardx-48, stardy-48, stardx+48, stardy+48 ) + trans );

	// draw puppy
	if ( !puppyatstar )
		gl::draw( tex[0], Area(64,32,96,64), Rectf(puppydx-16, puppydy-16, puppydx+16, puppydy+16) + trans );

	// draw princess
	if ( showplayer )
		gl::draw( tex[0], Area(96,26,128,64), Rectf(pdx-16, pdy-22, pdx+16, pdy+16) + trans );

	// draw enemies
	for( int i = 0; i < enemies.size(); i++ )
	{
		if ( enemies[i]->type == 0 ) // robot
			gl::draw( tex[0], Area(64,0,96,32), Rectf(enemies[i]->dx-16, enemies[i]->dy-16, enemies[i]->dx+16, enemies[i]->dy+16) + trans );
		else if ( enemies[i]->type == 1 ) // mobster
			gl::draw( tex[0], Area(32,0,64,32), Rectf(enemies[i]->dx-16, enemies[i]->dy-16, enemies[i]->dx+16, enemies[i]->dy+16) + trans );
		else if ( enemies[i]->type == 2 ) // bomber
			gl::draw( tex[0], Area(32,32,64,64), Rectf(enemies[i]->dx-16, enemies[i]->dy-16, enemies[i]->dx+16, enemies[i]->dy+16) + trans );
		else if ( enemies[i]->type == 3 ) // bomb
			gl::draw( tex[0], Area(0,32,32,64), Rectf(enemies[i]->dx-16, enemies[i]->dy-16, enemies[i]->dx+16, enemies[i]->dy+16) + trans );
		else if ( enemies[i]->type == 4 ) // starchaser
			gl::draw( tex[0], Area(0,64,44,96), Rectf(enemies[i]->dx-16, enemies[i]->dy-16, enemies[i]->dx+28, enemies[i]->dy+16) + trans );
	}

	gl::disableAlphaBlending();


	if ( extimer > 0 ) // draw visual indications of collision with enemies, or the animated stars of successfully reaching the star
	{
		if ( extype == 0 )
		{
			gl::color(1,0,0);
			gl::drawSolidRect( Rectf( exx-8-extimer*5, exy-8, exx+8-extimer*5, exy+8 ) );
			gl::drawSolidRect( Rectf( exx-8+extimer*5, exy-8, exx+8+extimer*5, exy+8 ) );
			gl::drawSolidRect( Rectf( exx-8, exy-8+extimer*5, exx+8, exy+8+extimer*5 ) );
			gl::drawSolidRect( Rectf( exx-8, exy-8-extimer*5, exx+8, exy+8-extimer*5 ) );
		}
		else if ( extype == 1 )
		{
			gl::color(1.0f,0.18431f,1.0f);
			gl::drawSolidRect( Rectf( exx-8-extimer*5, exy-8, exx+8-extimer*5, exy+8 ) );
			gl::drawSolidRect( Rectf( exx-8+extimer*5, exy-8, exx+8+extimer*5, exy+8 ) );
			gl::drawSolidRect( Rectf( exx-8, exy-8+extimer*5, exx+8, exy+8+extimer*5 ) );
			gl::drawSolidRect( Rectf( exx-8, exy-8-extimer*5, exx+8, exy+8-extimer*5 ) );
			gl::drawSolidRect( Rectf( exx-8+extimer*4, exy-8-extimer*4, exx+8+extimer*4, exy+8-extimer*4 ) );
			gl::drawSolidRect( Rectf( exx-8+extimer*4, exy-8+extimer*4, exx+8+extimer*4, exy+8+extimer*4 ) );
			gl::drawSolidRect( Rectf( exx-8-extimer*4, exy-8+extimer*4, exx+8-extimer*4, exy+8+extimer*4 ) );
			gl::drawSolidRect( Rectf( exx-8-extimer*4, exy-8-extimer*4, exx+8-extimer*4, exy+8-extimer*4 ) );
		}
		else if ( extype == 2 )
		{
			gl::color(0.0f,0.502f,1.0f);
			gl::drawSolidRect( Rectf( exx-8-extimer*5, exy-8, exx+8-extimer*5, exy+8 ) );
			gl::drawSolidRect( Rectf( exx-8+extimer*5, exy-8, exx+8+extimer*5, exy+8 ) );
			gl::drawSolidRect( Rectf( exx-8, exy-8+extimer*5, exx+8, exy+8+extimer*5 ) );
			gl::drawSolidRect( Rectf( exx-8, exy-8-extimer*5, exx+8, exy+8-extimer*5 ) );
			gl::drawSolidRect( Rectf( exx-8+extimer*4, exy-8-extimer*4, exx+8+extimer*4, exy+8-extimer*4 ) );
			gl::drawSolidRect( Rectf( exx-8+extimer*4, exy-8+extimer*4, exx+8+extimer*4, exy+8+extimer*4 ) );
			gl::drawSolidRect( Rectf( exx-8-extimer*4, exy-8+extimer*4, exx+8-extimer*4, exy+8+extimer*4 ) );
			gl::drawSolidRect( Rectf( exx-8-extimer*4, exy-8-extimer*4, exx+8-extimer*4, exy+8-extimer*4 ) );
		}
	}

	
	gl::color(1,1,1);
	gl::draw(tex[2], Area(0,0,512,1400), Rectf(sx, 0, realsx, 700)); // draw instructions

	if ( solving || solvedDialogUp ) // draw the dialog boxes for the solver algorithm in-progess or completed
	{
		gl::color(.73333f, .866667f, 1.0f);
		gl::drawSolidRect( Rectf(hsx-256, hsy-120, hsx+256, hsy+120) );
		gl::color(0,0,0);
		gl::drawStrokedRect( Rectf(hsx-256, hsy-120, hsx+256, hsy+120) );

		if ( solving )
		{
			gl::enableAlphaBlending();
			gl::drawStringCentered("Breadth-First Queue Search In Progress", Vec2f(hsx, hsy-100), black, fo);
			gl::drawStringCentered("Queue Size: " + toString(solvers.size()), Vec2f(hsx, hsy-70), black, fo);
			gl::drawStringCentered("Progress:", Vec2f(hsx, hsy), black, fo);

			for( int i = 0; i < 10; i++ )
				gl::drawSolidRect(Rectf(hsx-150+30*i-5, hsy+50, hsx-150+30*i+5, hsy+60) );

			gl::color(1,1,1);
			// draw star
			gl::draw( tex[1], Area(160, 0, 256, 96), Rectf( hsx+150-32, hsy+55-32, hsx+150+32, hsy+55+32 ) );

			if ( !puppyCleared )
			{
				int puppyx = hsx+150 - 30*bestProgress;
				if ( bestProgress > 10 )
					puppyx = hsx-150;
				int prinprogx = hsx-150;
				gl::draw( tex[0], Area(64,32,96,64), Rectf(puppyx-16, hsy+55-16, puppyx+16, hsy+55+16) ); // puppy
				gl::draw( tex[0], Area(96,26,128,64), Rectf(prinprogx-16, hsy+55-16, prinprogx+16, hsy+55+16) ); // princess
			}
			else
			{
				int puppyx = hsx+150;
				int prinprogx = hsx+150-30*bestProgress;
				if ( bestProgress > 10 )
					prinprogx = hsx-150;
				gl::draw( tex[0], Area(64,32,96,64), Rectf(puppyx-16, hsy+55-16, puppyx+16, hsy+55+16) ); // puppy
				gl::draw( tex[0], Area(96,26,128,64), Rectf(prinprogx-16, hsy+55-16, prinprogx+16, hsy+55+16) ); // princess
			}

			gl::drawStringCentered("Press SPACE to Cancel", Vec2f(hsx, hsy+90), black, fo);

			gl::disableAlphaBlending();
		}
		else // already solved: solved dialog up
		{
			gl::enableAlphaBlending();
			if ( winmoves[0] > -1 )
			{
				gl::drawStringCentered("Breadth-First Queue Search Successful!", Vec2f(hsx, hsy-100), black, fo);
				gl::drawStringCentered("Number of Moves in Solution: " + toString(winmoves.size()), Vec2f(hsx, hsy-70), black, fo);
				gl::drawStringCentered("Press SPACE to close this dialog", Vec2f(hsx, hsy), black, fo);
				gl::drawStringCentered("Press 'S' to see the solution", Vec2f(hsx, hsy+30), black, fo);
			}
			else
			{
				gl::drawStringCentered("This particular puzzle is impossible", Vec2f(hsx, hsy-100), black, fo);
				gl::drawStringCentered("(Sorry!)" + toString(winmoves.size()), Vec2f(hsx, hsy-70), black, fo);
				gl::drawStringCentered("Press SPACE to close this dialog", Vec2f(hsx, hsy), black, fo);
				gl::drawStringCentered("Press 'P' to generate a new puzzle", Vec2f(hsx, hsy+30), black, fo);
			}
			gl::disableAlphaBlending();
		}
	}
	else if ( showingSolution )
	{
		gl::enableAlphaBlending();
		gl::drawStringCentered( "Showing solution", Vec2f( hsx, 5 ), black, fo );
		gl::disableAlphaBlending();
	}

}

CINDER_APP_NATIVE( PrincessVsMafiaApp, RendererGl )

void PrincessVsMafiaApp::loadTextures()
{
	tex[0] = gl::Texture( loadImage( loadResource( tex0_, "IMAGE" ) ) );
	tex[1] = gl::Texture( loadImage( loadResource( tex1_, "IMAGE" ) ) );
	tex[2] = gl::Texture( loadImage( loadResource( instr_, "IMAGE" ) ) );
}

void PrincessVsMafiaApp::generateRooms()
{
	// generateRooms() creates a new arrangement of doors, and sets the starting positions for the princess, puppy, and all enemies

	width = randomInt(5, 10); // randomly set the number of rooms horizontally
	while ( width > 5 && width*96 > sx ) // ensure these rooms do not exceed the edge of the screen
		width--;

	height = randomInt(5, 9); // randomly set the number of rooms vertically
	while ( height > 5 && height*96 > sy ) // ensure these rooms do not exceed the edge of the screen
		height--;

	nodoor1 = randomInt(0, 5); // only 4 of the 6 types of door designs are shown during a single puzzle, set the two that will not show this time.
	nodoor2 = randomInt(0, 5);
	if ( nodoor1 == nodoor2 )
		nodoor2 = (nodoor2+1) % 6;

	for( int i = 0; i < height; i++ ) // assign doors for each room, ensuring that no room has two of the same door connected to it
	{
		for( int k = 0; k < width; k++ )
		{
			if ( i == 0 ) // top row
				rooms[i][k].doors[1] = -1;
			else if ( i == height-1 ) // bottom row
				rooms[i][k].doors[3] = -1;
			if ( k == 0 ) // left column
				rooms[i][k].doors[0] = -1;
			else if ( k == width-1 ) // right column
				rooms[i][k].doors[2] = -1;
		
			if ( k != width-1 ) // not on right column
			{
				if ( k != 0 ) // not on left column
					possible.push_back( rooms[i][k].doors[0] );
				if ( i != 0 ) // not on top row
				{
					possible.push_back( rooms[i][k].doors[1] );
					possible.push_back( rooms[i-1][k+1].doors[3] );
				}
				reversePossible();
				rooms[i][k].doors[2] = possible[ randomInt(0, possible.size()-1) ];
				rooms[i][k+1].doors[0] = rooms[i][k].doors[2];
				possible.clear();
			}
			if ( i != height-1 ) // not on bottom row
			{
				if ( k != 0 ) // not on left column
					possible.push_back( rooms[i][k].doors[0] );
				if ( i != 0 ) // not on top row
					possible.push_back( rooms[i][k].doors[1] );
				if ( k != width-1 )
					possible.push_back( rooms[i][k].doors[2] );
				reversePossible();
				rooms[i][k].doors[3] = possible[ randomInt(0, possible.size()-1) ];
				rooms[i+1][k].doors[1] = rooms[i][k].doors[3];
				possible.clear();
			}
		}
	}


	// set the positioning of the rooms on the screen
	if ( (width % 2) == 0 ) // even
		origxx = hsx - (width/2)*96;
	else // odd
		origxx = hsx-48 - ((width-1)/2)*96;
	if ( (height % 2) == 0 ) // even
		origyy = hsy - (height/2)*96;
	else // odd
		origyy = hsy-48 - ((height-1)/2)*96;

	moving = 0; // nothing is moving
	opendoor = -1; // all doors are closed

	pox = randomInt(0, width-1); // set the princess's position
	poy = height-1;
	px = pox;
	py = poy;
	pdx = origxx+48 + 96*px;
	pdy = origyy+48 + 96*py;


	int area = width*height;

	int num = 2; // set the number of enemies based on the area of all the rooms
	if ( area > 38 )
		num++;
	if ( area > 51 )
		num++;
	if ( area > 64 )
		num++;
	if ( area > 77 )
		num++;

	for( int i = 0; i < enemies.size(); i++ )
		delete enemies[i];
	enemies.clear();
	
	while ( num > 0 ) // create enemies
	{
		enemies.push_back( new Enemy() );
		int z = enemies.size()-1;
		if ( z == 0 )
			enemies[z]->type = 2; // bomber
		else if ( z == 1 )
			enemies[z]->type = 4; // star chaser
		else if ( randomInt(0,1) == 1 )
			enemies[z]->type = 1; // mobster
		else
			enemies[z]->type = 0; // robot

		for(;;) // find a starting position that does not collide with the princess or other enemies
		{
			int tx = randomInt(0, width-1);
			int ty = randomInt(1, height-2);
			if ( (tx == px && ty == py) || (abs(tx-px) == 1 && ty == py)
				|| (tx == px && abs(ty-py) == 1 ) )
				continue;
			
			bool free = true;
			for( int k = 0; k < z; k++ )
			{
				if ( enemies[k]->ox == tx && enemies[k]->oy == ty )
				{	free = false; break; }
			}
			if ( !free )
				continue;

			enemies[z]->ox = tx;
			enemies[z]->oy = ty;
			enemies[z]->x = tx;
			enemies[z]->y = ty;

			enemies[z]->dx = origxx+48 + 96*enemies[z]->x;
			enemies[z]->dy = origyy+48 + 96*enemies[z]->y;
			break;
		}

		num--;
	}


	// set star location that does not collide with any enemies
	for(;;)
	{
		starx = randomInt(0, width-1);
		stary = 0;

		bool enhere = false;
		for( int i = 0; i < enemies.size(); i++ )
		{
			if ( enemies[i]->x == starx && enemies[i]->y == stary )
			{	enhere = true; break; }
		}
		if ( enhere )
			continue;

		stardx = origxx+48 + 96*starx;
		stardy = origyy+48 + 96*stary;
		break;
	}

	// set puppy location, again not colliding with princess or enemies
	puppyatstar = false;
	for(;;)
	{
		puppyx = randomInt(0, width-1);
		puppyy = height-1;
		
		if ( puppyx == px )
			continue;

		bool enhere = false;
		for( int i = 0; i < enemies.size(); i++ )
		{
			if ( enemies[i]->x == puppyx && enemies[i]->y == puppyy )
			{	enhere = true; break; }
		}
		if ( enhere )
			continue;

		puppyox = puppyx;
		puppyoy = puppyy;
		puppydx = origxx+48 + 96*puppyx;
		puppydy = origyy+48 + 96*puppyy;
		break;
	}

	trans = Vec2f(0, -sy);
	transitionphase = 2; // this new puzzle will transition down from above

	extimer = 0; // no visual indicators or stars

	showplayer = true;

	// set default values for solver algorithm:
	for( int i = 0; i < solvers.size(); i++ )
		cleanAndKillSolver( solvers[i] );
	for( int i = 0; i < zombieSolvers.size(); i++ )
		cleanAndKillSolver( zombieSolvers[i] );
	solvers.clear();
	zombieSolvers.clear();
	winmoves.clear();
	solving = false;

	showingSolution = false;
	showingSolutionIndex = 0;
	showingSolutionCounter = 0;
}

void PrincessVsMafiaApp::reversePossible()
{
	// helper function for generateRooms(). This helps ensure that a room does not have two of the same door type connected to it

	bool cont[6];

	for( int k = 0; k < 6; k++ )
	{
		bool c = false;
		for( int i = 0; i < possible.size(); i++ )
		{
			if ( possible[i] == k )
			{	c = true; break; }
		}

		cont[k] = c;
	}

	possible.clear();

	for( int k = 0; k < 6; k++ )
	{
		if ( cont[k] == false && k != nodoor1 && k != nodoor2 )
			possible.push_back( k );
	}

}

void PrincessVsMafiaApp::resetRooms()
{
	// resetRooms() returns the princess, puppy, and all enemies to their original positions

	px = pox; // reset princess to original position
	py = poy;
	pdx = origxx+48 + 96*px;
	pdy = origyy+48 + 96*py;

	puppyatstar = false; // reset puppy to original position
	puppyx = puppyox;
	puppyy = puppyoy;
	puppydx = origxx+48 + 96*puppyx;
	puppydy = origyy+48 + 96*puppyy;

	moving = 0; // nothing is moving
	opendoor = -1; // all doors are closed

	for( int i = 0; i < enemies.size(); i++ ) // delete all bombs
	{
		if ( enemies[i]->type == 3 ) // bomb
		{
			delete enemies[i];
			for( int j = i; j < enemies.size()-1; j++ )
				enemies[j] = enemies[j+1];
			enemies.pop_back();
			i--;
			continue;
		}
	}

	for( int i = 0; i < enemies.size(); i++ ) // reset all enemies to their original positions
	{
		enemies[i]->x = enemies[i]->ox;
		enemies[i]->y = enemies[i]->oy;
		enemies[i]->dx = origxx+48 + 96*enemies[i]->x;
		enemies[i]->dy = origyy+48 + 96*enemies[i]->y;
	}

}

void PrincessVsMafiaApp::decideEnemies()
{
	// decideEnemies() determines the upcoming actions of all enemies and stores them in internal variables

	for( int i = 0; i < enemies.size(); i++ ) // loop through all enemies
	{
		enemies[i]->dir = -1;
		if ( enemies[i]->type == 3 ) // bombs do not have any actions
			continue;

		for( int k = 0; k < 4; k++ ) // is there an open door to another room for this enemy?
		{
			if ( rooms[enemies[i]->y][enemies[i]->x].doors[k] == opendoor )
				enemies[i]->dir = k;
		}
		if ( enemies[i]->dir == -1 ) // if there are no open doors around this enemy, then there is nothing to do
			continue;

		// there is an open door:
		int desx = enemies[i]->x; // desired x position
		int desy = enemies[i]->y; // desired y position
		if ( enemies[i]->dir == 0 ) // open door is left
			desx--;
		else if ( enemies[i]->dir == 1 ) // open door is up
			desy--;
		else if ( enemies[i]->dir == 2 ) // open door is right
			desx++;
		else if ( enemies[i]->dir == 3 ) // open door is down
			desy++;
		for( int k = 0; k < enemies.size(); k++ ) // is there another enemy blocking your path to the next room?
		{
			if ( k != i && enemies[k]->x == desx && enemies[k]->y == desy )
			{	enemies[i]->dir = -1; break; }
		}
		if ( enemies[i]->dir == -1 ) // if there is another enemy in the way, then there is nothing to do
			continue;
		// there are no other enemies in the way:

		if ( enemies[i]->type == 2 ) // bombers always move through an open door and place a bomb behind them
		{
			enemies.push_back( new Enemy() );
			int z = enemies.size()-1;
			enemies[z]->x = enemies[i]->x;
			enemies[z]->y = enemies[i]->y;
			enemies[z]->dx = origxx+48 + 96*enemies[z]->x;
			enemies[z]->dy = origyy+48 + 96*enemies[z]->y;
			enemies[z]->ox = 0;
			enemies[z]->oy = 0;
			enemies[z]->type = 3;
			enemies[z]->dir = -1;
		}

		if ( enemies[i]->type == 1 ) // mobsters only move through open doors if they get closer to the puppy, or if the puppy is already cleared, closer to the princess
		{
			int antx; // the future position of puppy or princess to chase
			int anty;

			if ( puppyatstar ) // chase princess
			{
				antx = px;
				anty = py;
				if ( pdir == 0 )
					antx--;
				else if ( pdir == 1 )
					anty--;
				else if ( pdir == 2 )
					antx++;
				else if ( pdir == 3 )
					anty++;
			}
			else // chase puppy
			{
				antx = puppyx;
				anty = puppyy;
				if ( puppydir == 0 )
					antx--;
				else if ( puppydir == 1 )
					anty--;
				else if ( puppydir == 2 )
					antx++;
				else if ( puppydir == 3 )
					anty++;
			}

			if ( enemies[i]->dir == 0 && enemies[i]->x <= antx )
				enemies[i]->dir = -1;
			else if ( enemies[i]->dir == 1 && enemies[i]->y <= anty )
				enemies[i]->dir = -1;
			else if ( enemies[i]->dir == 2 && enemies[i]->x >= antx )
				enemies[i]->dir = -1;
			else if ( enemies[i]->dir == 3 && enemies[i]->y >= anty )
				enemies[i]->dir = -1;
		}

		if ( enemies[i]->type == 4 ) // starchasers only move through open doors if they get closer to the star
		{
			if ( enemies[i]->dir == 0 && enemies[i]->x <= starx )
				enemies[i]->dir = -1;
			else if ( enemies[i]->dir == 1 && enemies[i]->y <= stary )
				enemies[i]->dir = -1;
			else if ( enemies[i]->dir == 2 && enemies[i]->x >= starx )
				enemies[i]->dir = -1;
			else if ( enemies[i]->dir == 3 && enemies[i]->y >= stary )
				enemies[i]->dir = -1;
		}
	}
}

void PrincessVsMafiaApp::SOdecideEnemies( Solver* so )
{
	// SOdecideEnemies() is the same as decide enemies (above), except for the solver's internal variables.
	// The code is duplicated to reduce complexity, even though there may be better ways of doing this.

	for( int i = 0; i < so->ens.size(); i++ ) // loop through all enemies
	{
		so->ens[i]->dir = -1;
		if ( so->ens[i]->type == 3 ) // bombs do not have any actions
			continue;

		for( int k = 0; k < 4; k++ ) // is there an open door to another room for this enemy?
		{
			if ( rooms[so->ens[i]->y][so->ens[i]->x].doors[k] == SOopendoor )
				so->ens[i]->dir = k;
		}
		if ( so->ens[i]->dir == -1 ) // if there are no open doors around this enemy, then there is nothing to do
			continue;
		// there is an open door:
		int desx = so->ens[i]->x; // desired x position
		int desy = so->ens[i]->y; // desired y position
		if ( so->ens[i]->dir == 0 ) // open door is left
			desx--;
		else if ( so->ens[i]->dir == 1 ) // open door is up
			desy--;
		else if ( so->ens[i]->dir == 2 ) // open door is right
			desx++;
		else if ( so->ens[i]->dir == 3 ) // open door is down
			desy++;
		for( int k = 0; k < so->ens.size(); k++ ) // is there another enemy blocking your path to the next room?
		{
			if ( k != i && so->ens[k]->x == desx && so->ens[k]->y == desy )
			{	so->ens[i]->dir = -1; break; }
		}
		if ( so->ens[i]->dir == -1 ) // if there is another enemy in the way, then there is nothing to do
			continue;
		// there are no other enemies in the way:

		if ( so->ens[i]->type == 2 ) // bombers always move through an open door and place a bomb behind them
		{
			so->ens.push_back( new Enemy() );
			int z = so->ens.size()-1;
			so->ens[z]->x = so->ens[i]->x;
			so->ens[z]->y = so->ens[i]->y;
			so->ens[z]->dx = origxx+48 + 96*so->ens[z]->x;
			so->ens[z]->dy = origyy+48 + 96*so->ens[z]->y;
			so->ens[z]->ox = 0;
			so->ens[z]->oy = 0;
			so->ens[z]->type = 3;
			so->ens[z]->dir = -1;
		}

		if ( so->ens[i]->type == 1 ) // mobsters only move through open doors if they get closer to the puppy, or if the puppy is already cleared, closer to the princess
		{
			int antx; // the future position of puppy or princess to chase
			int anty;

			if ( so->puppyatstar ) // chase princess
			{
				antx = so->princessx;
				anty = so->princessy;
				if ( so->moves[ so->moves.size()-1 ] == 0 )
					antx--;
				else if ( so->moves[ so->moves.size()-1 ] == 1 )
					anty--;
				else if ( so->moves[ so->moves.size()-1 ] == 2 )
					antx++;
				else if ( so->moves[ so->moves.size()-1 ] == 3 )
					anty++;
			}
			else // chase puppy
			{
				antx = so->puppyx;
				anty = so->puppyy;
				if ( so->puppydir == 0 )
					antx--;
				else if ( so->puppydir == 1 )
					anty--;
				else if ( so->puppydir == 2 )
					antx++;
				else if ( so->puppydir == 3 )
					anty++;
			}

			if ( so->ens[i]->dir == 0 && so->ens[i]->x <= antx )
				so->ens[i]->dir = -1;
			else if ( so->ens[i]->dir == 1 && so->ens[i]->y <= anty )
				so->ens[i]->dir = -1;
			else if ( so->ens[i]->dir == 2 && so->ens[i]->x >= antx )
				so->ens[i]->dir = -1;
			else if ( so->ens[i]->dir == 3 && so->ens[i]->y >= anty )
				so->ens[i]->dir = -1;
		}

		if ( so->ens[i]->type == 4 )  // starchasers only move through open doors if they get closer to the star
		{
			if ( so->ens[i]->dir == 0 && so->ens[i]->x <= starx )
				so->ens[i]->dir = -1;
			else if ( so->ens[i]->dir == 1 && so->ens[i]->y <= stary )
				so->ens[i]->dir = -1;
			else if ( so->ens[i]->dir == 2 && so->ens[i]->x >= starx )
				so->ens[i]->dir = -1;
			else if ( so->ens[i]->dir == 3 && so->ens[i]->y >= stary )
				so->ens[i]->dir = -1;
		}
	}
}

void PrincessVsMafiaApp::initializeSolver( Solver* so, int dir )
{
	// initializeSolver sets default values for the solver's variables

	for( int i = 0; i < enemies.size(); i++ )
		so->ens.push_back( new Enemy( *enemies[i] ) );
	so->princessx = px;
	so->princessy = py;
	so->puppyx = puppyx;
	so->puppyy = puppyy;
	so->moves.push_back( dir );
	so->puppyatstar = false;
}

void PrincessVsMafiaApp::updateSolving()
{
	// updateSolving() takes the first solver's state in the queue, and spawns four new states from each move direction (left/up/right/down), placing the new states at the end of the queue.
	// This continues until a solution is found or there are no solvers left in the queue, indicating no solution.

	if ( solvers.size() > 6000 ) // if the queue has grown too large and slow, purge states that are lagging behind other states in terms of solution prospects.
	{
		purgedAnything = true;
		purgeSolvers();
		return;
	}


	int togo = 300; // run through 300 states before updating the screen graphics again
	while( togo > 0 )
	{
		if ( solvers.empty() ) // if there are no valid solver states left, this means there is no solution. Give up the algorithm.
		{
			solving = false;
			winmoves.clear();
			winmoves.push_back( -1 );
			for( int i = 0; i < solvers.size(); i++ )
				cleanAndKillSolver( solvers[i] );
			solvers.clear();
			for( int i = 0; i < zombieSolvers.size(); i++ )
				cleanAndKillSolver( zombieSolvers[i] );
			zombieSolvers.clear();
			setFrameRate(60);
			break;
		}

		if ( solvers.size() > maxSolversNeeded )
			maxSolversNeeded = solvers.size();

		// track the progress of the most promising solver:
		int cl = closeness( solvers[0] ); // the progress of a solver is measured by how close the puppy and princess are to the star
		if ( !puppyCleared )
		{
			if ( solvers[0]->puppyatstar )
			{
				puppyCleared = true;
				bestProgress = cl;
			}
			else if ( cl < bestProgress )
				bestProgress = cl;
		}
		else
		{
			if ( solvers[0]->puppyatstar && cl < bestProgress )
				bestProgress = cl;
		}

		// if the current solver is close to finishing, try sprinting straight to the star, ignoring enemies.
		if ( solvers[0]->puppyatstar && cl == bestProgress && cl <= 2 ) // try for sprint
		{
			if ( tryForSprint(cl) ) // indicates a winning solver if true
				break;
		}

		bool foundsuccess = false; // were any of these solvers successful in finding a solution?
		if ( solvers[0]->princessx > 0 ) // if there is a room to the left, add a solver for a left move to the end of the queue
		{
			solvers.push_back( makeACopy( solvers[0] ) );
			solvers[ solvers.size()-1 ]->moves.push_back( 0 );
			simulateLastMove(solvers[solvers.size()-1]);
			if ( !isValid(solvers[solvers.size()-1]) )
			{
				cleanAndKillSolver(solvers[solvers.size()-1]);
				solvers.pop_back();
			}
			else if ( solvers[solvers.size()-1]->moves[ solvers[solvers.size()-1]->moves.size()-1] == 100 )
				foundsuccess = true;
		}
		if ( solvers[0]->princessx < width-1 && !foundsuccess ) // if there is a room to the right, add a solver for a right move to the end of the queue
		{
			solvers.push_back( makeACopy( solvers[0] ) );
			solvers[ solvers.size()-1 ]->moves.push_back( 2 );
			simulateLastMove(solvers[solvers.size()-1]);
			if ( !isValid(solvers[solvers.size()-1]) )
			{
				cleanAndKillSolver(solvers[solvers.size()-1]);
				solvers.pop_back();
			}
			else if ( solvers[solvers.size()-1]->moves[ solvers[solvers.size()-1]->moves.size()-1] == 100 )
				foundsuccess = true;
		}
		if ( solvers[0]->princessy > 0 && !foundsuccess ) // if there is a room up, add a solver for an up move to the end of the queue
		{
			solvers.push_back( makeACopy( solvers[0] ) );
			solvers[ solvers.size()-1 ]->moves.push_back( 1 );
			simulateLastMove(solvers[solvers.size()-1]);
			if ( !isValid(solvers[solvers.size()-1]) )
			{
				cleanAndKillSolver(solvers[solvers.size()-1]);
				solvers.pop_back();
			}
			else if ( solvers[solvers.size()-1]->moves[ solvers[solvers.size()-1]->moves.size()-1] == 100 )
				foundsuccess = true;
		}
		if ( solvers[0]->princessy < height-1 && !foundsuccess ) // if there is a room down, add a solver for a down move to the end of the queue
		{
			solvers.push_back( makeACopy( solvers[0] ) );
			solvers[ solvers.size()-1 ]->moves.push_back( 3 );
			simulateLastMove(solvers[solvers.size()-1]);
			if ( !isValid(solvers[solvers.size()-1]) )
			{
				cleanAndKillSolver(solvers[solvers.size()-1]);
				solvers.pop_back();
			}
			else if ( solvers[solvers.size()-1]->moves[ solvers[solvers.size()-1]->moves.size()-1] == 100 )
				foundsuccess = true;
		}

		if ( foundsuccess ) // Success! The most recent solver on the queue found a solution.
		{
			solvers[solvers.size()-1]->moves.pop_back();
			winmoves.clear();
			for( int i = 0; i < solvers[solvers.size()-1]->moves.size(); i++ )
				winmoves.push_back( solvers[solvers.size()-1]->moves[i] );
			for( int i = 0; i < solvers.size(); i++ )
				cleanAndKillSolver( solvers[i] );
			solvers.clear();
			for( int i = 0; i < zombieSolvers.size(); i++ )
				cleanAndKillSolver( zombieSolvers[i] );
			zombieSolvers.clear();
			solving = false;
			solvedDialogUp = true;
			setFrameRate(60);
			break;
		}
		
		
		if ( zombieSolvers.size() > 6000 ) // too many zombie solvers, clear all zombie solvers
		{
			for( int i = 0; i < zombieSolvers.size(); i++ )
				cleanAndKillSolver( zombieSolvers[i] );
			zombieSolvers.clear();
		}

		zombieSolvers.push_back( solvers[0] ); // since this solver was just processed, add it to zombie solvers and remove it from the queue
		for( int j = 0; j < solvers.size()-1; j++ )
			solvers[j] = solvers[j+1];
		solvers.pop_back();
		
		togo--;
	}
	
}

void PrincessVsMafiaApp::simulateLastMove( Solver* so )
{
	// simulateLastMove updates the positions of the princess, puppy, and enemies based on the most recent move added to this solver

	// determine the princess' next position:
	int tx = so->princessx;
	int ty = so->princessy;
	int which = so->moves[ so->moves.size()-1 ];
	if ( which == 0 )
	{	tx--; }
	else if ( which == 1 )
	{	ty--; }
	else if ( which == 2 )
	{	tx++; }
	else
	{	ty++; }

	if ( tx >= 0 && tx < width && ty >= 0 && ty < height ) // ensure princess is not out-of-bounds
	{
		bool enthere = false;
		for( int i = 0; i < so->ens.size(); i++ )
		{
			if ( so->ens[i]->x == tx && so->ens[i]->y == ty )
			{	enthere = true; break; }
		}
		if ( !enthere ) // ensure the princess is not moving into an enemy
		{
			SOopendoor = rooms[so->princessy][so->princessx].doors[which];

			// ensure that the puppy is not moving into an enemy
			bool enpup = false;
			int pupwhich = -1;
			if ( !so->puppyatstar )
			{
				for( int k = 0; k < 4; k++ )
				{
					if ( rooms[so->puppyy][so->puppyx].doors[k] == SOopendoor )
						pupwhich = k;
				}
				int pupdesx = so->puppyx;
				int pupdesy = so->puppyy;
				if ( pupwhich == 0 )
					pupdesx--;
				else if ( pupwhich == 1 )
					pupdesy--;
				else if ( pupwhich == 2 )
					pupdesx++;
				else if ( pupwhich == 3 )
					pupdesy++;

				so->puppydir = pupwhich;

				for( int i = 0; i < so->ens.size(); i++ )
				{
					if ( so->ens[i]->x == pupdesx && so->ens[i]->y == pupdesy )
					{	enpup = true; break; }
				}
			}
				
			if ( enpup )
			{
				so->moves.push_back( -1 ); // invalid state, the puppy tried to move into an enemy
			}
			else // if ( !enpup ) // there are no enemies in the puppy's path
			{
				SOdecideEnemies( so ); // determine all enemy movements
				
				SOopendoor = -1;
				if ( which == 0 )
					so->princessx--;
				else if ( which == 1 )
					so->princessy--;
				else if ( which == 2 )
					so->princessx++;
				else if ( which == 3 )
					so->princessy++;
				if ( !so->puppyatstar )
				{
					if ( so->puppydir == 0 )
						so->puppyx--;
					else if ( so->puppydir == 1 )
						so->puppyy--;
					else if ( so->puppydir == 2 )
						so->puppyx++;
					else if ( so->puppydir == 3 )
						so->puppyy++;
				}

				for( int i = 0; i < so->ens.size(); i++ )
				{
					if ( so->ens[i]->dir == 0 )
						so->ens[i]->x--;
					else if ( so->ens[i]->dir == 1 )
						so->ens[i]->y--;
					else if ( so->ens[i]->dir == 2 )
						so->ens[i]->x++;
					else if ( so->ens[i]->dir == 3 )
						so->ens[i]->y++;
				}

				if ( !so->puppyatstar && so->puppyx == starx && so->puppyy == stary )
				{
					so->puppyatstar = true;
				}

				if ( so->puppyatstar && so->princessx == starx && so->princessy == stary )
				{
					so->moves.push_back( 100 ); // success!
				}
			}
		}
		else
		{
			so->moves.push_back( -1 ); // invalid state, the princess tried moving into an enemy
		}
	}
	else
	{
		so->moves.push_back( -1 ); // invalid state, the princess tried to move out-of-bounds
	}
}

void PrincessVsMafiaApp::cleanAndKillSolver( Solver* so )
{
	// equivalent to a destructor for Solver
	for( int i = 0; i < so->ens.size(); i++ )
		delete so->ens[i];
	so->ens.clear();
	delete so;
}

Solver* PrincessVsMafiaApp::makeACopy( Solver* so )
{
	// equivalent to a copy constructor for Solver
	Solver* r = new Solver();
	r->princessx = so->princessx;
	r->princessy = so->princessy;
	for( int i = 0; i < so->moves.size(); i++ )
		r->moves.push_back( so->moves[i] );
	r->puppydir = so->puppydir;
	r->puppyatstar = so->puppyatstar;
	r->puppyx = so->puppyx;
	r->puppyy = so->puppyy;
	for( int i = 0; i < so->ens.size(); i++ )
	{
		r->ens.push_back( new Enemy() );
		(*r->ens[i]) = (*so->ens[i]);
	}
	return r;
}

int PrincessVsMafiaApp::closeness( Solver* so )
{
	// determines how "close" this Solver is from finding a successful solution.
	// Returns how far away the princess and puppy are from the star.
	int acc = 0;
	if ( !so->puppyatstar )
	{
		acc += abs( so->puppyx - starx );
		acc += abs( so->puppyy - stary );
	}
	else
	{
		acc += abs( so->princessx - starx );
		acc += abs( so->princessy - stary );
	}
	return acc;
}

bool PrincessVsMafiaApp::identical( Solver* s1, Solver* s2 )
{
	// returns true if the two solvers given as arguments represent identical states, false otherwise
	if ( s1->princessx != s2->princessx || s1->princessy != s2->princessy )
		return false;
	if ( s1->puppyatstar != s2->puppyatstar )
		return false;
	if ( !s1->puppyatstar && (s1->puppyx != s2->puppyx || s1->puppyy != s2->puppyy) )
		return false;
	if ( s1->ens.size() != s2->ens.size() )
		return false;
	for( int i = 0; i < s1->ens.size(); i++ )
	{
		if ( s1->ens[i]->x != s2->ens[i]->x || s1->ens[i]->y != s2->ens[i]->y )
			return false;
	}
	return true;
}

void PrincessVsMafiaApp::purgeSolvers()
{
	// In order to speed up the solving algorithm, purgeSolvers() deletes solvers that don't have much of a chance of reaching a solution.
	int numPrincesses = 0;
	int numGuys = 0;
	int prclosest = 10000;
	int prfarthest = 0;
	int guyclosest = 10000;
	int guyfarthest = 0;
	for( int i = 0; i < solvers.size(); i++ )
	{
		if ( solvers[i]->puppyatstar )
		{
			numGuys++;
			int cl = closeness( solvers[i] );
			if ( cl < guyclosest )
				guyclosest = cl;
			if ( cl > guyfarthest )
				guyfarthest = cl;
		}
		else
		{
			numPrincesses++;
			int cl = closeness( solvers[i] );
			if ( cl < prclosest )
				prclosest = cl;
			if ( cl > prfarthest )
				prfarthest = cl;
		}
	}
	if ( numPrincesses > 0 && (prclosest != prfarthest || numGuys > 0) )
	{
		for( int i = 0; i < solvers.size(); i++ )
		{
			if ( !solvers[i]->puppyatstar && closeness( solvers[i] ) >= prfarthest )
			{
				cleanAndKillSolver( solvers[i] );
				for( int j = i; j < solvers.size()-1; j++ )
					solvers[j] = solvers[j+1];
				solvers.pop_back();
				i--;
				continue;
			}
		}
	}
	else if ( numPrincesses == 0 && numGuys > 0 && guyclosest != guyfarthest )// princesses all cleared
	{
		for( int i = 0; i < solvers.size(); i++ )
		{
			if ( closeness( solvers[i] ) >= guyfarthest )
			{
				cleanAndKillSolver( solvers[i] );
				for( int j = i; j < solvers.size()-1; j++ )
					solvers[j] = solvers[j+1];
				solvers.pop_back();
				i--;
				continue;
			}
		}
	}
	else // kill every fourth
	{
		timesSuperPurged++;
		int numkilled = 0;
		for( int i = 0; i < solvers.size(); i++ )
		{
			if ( randomInt(0, 4) == 0 )
			{
				cleanAndKillSolver( solvers[i] );
				for( int j = i; j < solvers.size()-1; j++ )
					solvers[j] = solvers[j+1];
				solvers.pop_back();
				i--;
				numkilled++;
				if ( numkilled >= 200 )
					break;
				continue;
			}
		}
	}
}

bool PrincessVsMafiaApp::isValid(Solver* so)
{
	// returns true if the solver argument represents a unique state where the princess and puppy have chances of reaching the star, false otherwise

	if ( so->moves[ so->moves.size()-1 ] == -1 ) // bad state
		return false;

	bool badstate = false;
	for( int i = 0; i < so->ens.size(); i++ )
	{
		if ( so->ens[i]->x == starx && so->ens[i]->y == stary
			&& (so->ens[i]->type == 2 || so->ens[i]->type == 3
			|| so->ens[i]->type == 4 ) )
		{	badstate = true; break; }
	}

	if ( badstate )
		return false;

	bool circler = false;
	if ( so->moves.size() > 2 )
	{
		int z = so->moves.size()-1;
		if ( so->moves[z] == so->moves[z-2] &&
			(so->moves[z]-so->moves[z-1] == -2 ||
			so->moves[z]-so->moves[z-1] == 2 ) )
		{
			circler = true;
		}
	}
	if ( circler )
	{
		circlersRemoved++;
		return false;
	}

	bool duplicate = false;
	for( int i = 0; i < zombieSolvers.size(); i++ )
	{
		if ( identical(so, zombieSolvers[i]) )
		{	duplicate = true; break; }
	}
	if ( duplicate )
	{
		duplicatesRemoved++;
		return false;
	}

	return true;
}

bool PrincessVsMafiaApp::tryForSprint(int cl)
{
	// if the current solver is close to finishing, try sprinting straight to the star, ignoring enemies.

	bool sprintsuccess = false;

	Solver* so = solvers[0];

	if ( cl == 1 )
	{
		if ( so->princessx < starx )
		{
			so->moves.push_back(2);
			so->moves.push_back(100);
			sprintsuccess = true;
		}
		else if ( so->princessx > starx )
		{
			so->moves.push_back(0);
			so->moves.push_back(100);
			sprintsuccess = true;
		}
		else
		{
			so->moves.push_back(1);
			so->moves.push_back(100);
			sprintsuccess = true;
		}
	}
	else if ( cl == 2 )
	{
		if ( so->princessx == starx - 2 || so->princessx == starx - 1 )
		{
			Solver* copy = makeACopy(so);
			copy->moves.push_back(2);
			simulateLastMove(copy);
			if ( isValid(copy) )
			{
				so->moves.push_back(2);
				if ( so->princessx == starx - 2 )
					so->moves.push_back(2);
				else
					so->moves.push_back(1);
				so->moves.push_back(100);
				sprintsuccess = true;
			}
		}
		if ( (so->princessx == starx + 2 || so->princessx == starx + 1) && !sprintsuccess )
		{
			Solver* copy = makeACopy(so);
			copy->moves.push_back(0);
			simulateLastMove(copy);
			if ( isValid(copy) )
			{
				so->moves.push_back(0);
				if ( so->princessx == starx + 2 )
					so->moves.push_back(0);
				else
					so->moves.push_back(1);
				so->moves.push_back(100);
				sprintsuccess = true;
			}
		}
		if ( (so->princessy == stary + 2 || so->princessy == stary + 1) && !sprintsuccess )
		{
			Solver* copy = makeACopy(so);
			copy->moves.push_back(1);
			simulateLastMove(copy);
			if ( isValid(copy) )
			{
				so->moves.push_back(1);
				if ( so->princessy == stary + 2 )
					so->moves.push_back(1);
				else if ( so->princessx < starx )
					so->moves.push_back(2);
				else
					so->moves.push_back(0);
				so->moves.push_back(100);
				sprintsuccess = true;
			}
		}

	}




	if ( sprintsuccess ) // success!
	{
		so->moves.pop_back();
		winmoves.clear();
		for( int i = 0; i < so->moves.size(); i++ )
			winmoves.push_back( so->moves[i] );
		for( int i = 0; i < solvers.size(); i++ )
			cleanAndKillSolver( solvers[i] );
		solvers.clear();
		for( int i = 0; i < zombieSolvers.size(); i++ )
			cleanAndKillSolver( zombieSolvers[i] );
		zombieSolvers.clear();
		solving = false;
		solvedBySprint = true;
		solvedDialogUp = true;
		setFrameRate(60);
		return true;
	}

	return false;
}
