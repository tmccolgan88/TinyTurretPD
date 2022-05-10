//
//  main.c
//  Extension
//
// By   : Tim McColgan
// Date : 04/25/2022

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pd_api.h"

#include "headers/global_data.h"
#include "particles/particles.h"

#define HIGH_SCORE_FILE "highscore.txt";

#define RIGHT_BOUNDARY 385
#define LEFT_BOUNDARY 190

#define TURRET_SPEED 5
#define BULLET_SPEED -4

#define SHOOT_TIME 500
#define MOVE_TIME 60

#define MAX_MISSILES 10

#ifdef _WINDLL
__declspec(dllexport)
#endif

//prototypes
int update(void* userdata);
int drawParticles(void);
int drawHUD(void);
void setupGame(void);
void updateTurret(void);
void loadAssets(void);
void buildGround(void); //name?
LCDSprite *shoot(void);
LCDSprite* launchMissile(void);
LCDSprite* createTurret(void);

typedef enum {
	Menu,
	Pause,
	Play,
	GameOver
} GameState;

typedef enum {
	kTurret = 0,
	kBullet = 1,
	kMissile = 2,
	kGround1 = 3,
	kGround2 = 4,
	kGround3 = 5
} SpriteType;

typedef struct VelocitySprite
{
	LCDSprite* sprite;
	int vx;
	int vy;

} VelocitySprite;

GameState gameState = Play;

PlaydateAPI* pd = NULL;
LCDSprite* turret;
LCDBitmap* bulletBMP;
LCDBitmap* missileBMP;
LCDBitmap* ground1BMP;
LCDBitmap* ground3BMP;
LCDBitmap* particleBMP;
LCDBitmap* hudBorderBMP;
LCDBitmap* gameFieldBorderBMP;
LCDBitmap* hudSpacerBMP;
LCDBitmap* gameoverBMP;

LCDFont* font;

//variables
int lastTime = 0;
int deltaTime = 0;

int canMove = 0;
int moveTimer = 0;
int shootTimer = 0;
int bulletHeight = 0;
int missileCounter = 0;
int missileTimer = 0;
int missileHeight = 0;

//HUD variables
int score = 0;
int highScore = 0;
int multiplier = 1;
int threat = 1;

int writeHighScore(int finalGameScore)
{
	char scoreString[6];
	int bytesWritten = 0;

	sprintf(scoreString, "%d", finalGameScore);
	SDFile* file = pd->file->open("highscore.txt", kFileWrite);
	bytesWritten = pd->file->write(file, scoreString, strlen(scoreString));
	pd->file->close(file);

	return bytesWritten;
}

int readHighScore()
{
	int err = 0;
	char highScoreBuf[5];

	SDFile* file = pd->file->open("highscore.txt", kFileReadData);
	pd->file->read(file, highScoreBuf, strlen(highScoreBuf));
	highScore = atoi(highScoreBuf);
	
	return 1;
}

int eventHandler(PlaydateAPI* p, PDSystemEvent event, uint32_t arg)
{
	(void)arg; // arg is currently only used for event = kEventKeyPressed

	if (event == kEventInit)
	{
		p->system->setUpdateCallback(update, p);
		pd = p;
		readHighScore();
		loadAssets();
		setupGame();
	}

	return 0;
}


LCDBitmap *loadImageAtPath(const char* path)
{
	const char* outErr = NULL;
	LCDBitmap* img = pd->graphics->loadBitmap(path, &outErr);
	if (outErr != NULL) 
	{
		pd->system->logToConsole("Error loading image at path '%s': %s", path, outErr);
	}
	return img;
}


void updateMissile(LCDSprite *s)
{
	float x, y;
    int targetY = 0;
	int collisionLen = 0;

	pd->sprite->getPosition(s, &x, &y);
	targetY = y + 2;
	SpriteCollisionInfo *collisions = pd->sprite->moveWithCollisions(s, x, targetY, NULL, NULL, &collisionLen);

	if (collisionLen > 0)
	{
		SpriteCollisionInfo info;

		//probably don't need to do a loop here, could
		//speed it up by just grabbing element one of the array
		int i;
		for (i = 0; i < collisionLen; i++)
		{
			info = collisions[i];

			if (pd->sprite->getTag(info.other) == kBullet)
			{
				pd->sprite->removeSprite(s);
				pd->sprite->freeSprite(s);
				
				addParticleBurst(particleBMP, x, y);
				score += 5 * multiplier;
				multiplier++;
				pd->sprite->removeSprite(info.other);
				pd->sprite->freeSprite(info.other);

			}
			else 
			{
				pd->sprite->removeSprite(s);
				pd->sprite->freeSprite(s);

				multiplier = 1;
				pd->sprite->removeSprite(info.other);
				pd->sprite->freeSprite(info.other);

				if (pd->sprite->getTag(info.other) == kGround2)
				{
					if (threat < 2)
						threat = 2;
				}
				else if (pd->sprite->getTag(info.other) == kGround3)
				{
					if (threat < 3)
						threat = 3;

					gameState = GameOver;
				}

			}

			missileCounter--;
		}

		pd->system->realloc(collisions, 0);
	}

	if (y > 250)
	{
		pd->sprite->removeSprite(s);
		pd->sprite->freeSprite(s);
		missileCounter--;
	}
}

LCDSprite* launchMissile()
{
	LCDSprite* missile = pd->sprite->newSprite();
	pd->sprite->setUpdateFunction(missile, updateMissile);
	pd->sprite->setImage(missile, missileBMP, kBitmapUnflipped);

	int w;
	pd->graphics->getBitmapData(missileBMP, &w, &missileHeight, NULL, NULL, NULL);
	PDRect cr = PDRectMake(0.0f, 0.0f, w, missileHeight);
	pd->sprite->setCollideRect(missile, cr);
	pd->sprite->moveTo(missile, (rand() % 200) + LEFT_BOUNDARY, 0);
	pd->sprite->setZIndex(missile, 999);
	pd->sprite->setTag(missile, kMissile);
	pd->sprite->addSprite(missile);

	missileCounter++;

	return missile;

}

void updateBullet(LCDSprite* s)
{
	pd->sprite->moveBy(s, 0, BULLET_SPEED);

	float x, y;
	pd->sprite->getPosition(s, &x, &y);
	if (y < 0)
	{
		pd->sprite->removeSprite(s);
		pd->sprite->freeSprite(s);
	}
	
}

LCDSprite* shoot()
{
	LCDSprite* bullet = pd->sprite->newSprite();
	pd->sprite->setUpdateFunction(bullet, updateBullet);
	
	pd->sprite->setImage(bullet, bulletBMP, kBitmapUnflipped);

	int w;
	pd->graphics->getBitmapData(bulletBMP, &w, &bulletHeight, NULL, NULL, NULL);
	PDRect cr = PDRectMake(0, 0, w, bulletHeight);
	pd->sprite->setCollideRect(bullet, cr);

	float x, y;
	pd->sprite->getPosition(turret, &x, &y);

	pd->sprite->moveTo(bullet, x, y - 2);
	pd->sprite->setTag(bullet, kBullet);
	pd->sprite->addSprite(bullet);
	
	return bullet;
}

void updateTurret(LCDSprite* s)
{
	float crankChange = 0;

	PDButtons current;
	pd->system->getButtonState(&current, NULL, NULL);

	if (shootTimer < SHOOT_TIME)
		shootTimer += deltaTime;

	if ((current & kButtonUp) && shootTimer > SHOOT_TIME)
	{
		shoot();
		shootTimer = 0;
	}

	if (moveTimer < 90)
	  moveTimer += deltaTime;

	crankChange = pd->system->getCrankChange();

	if (crankChange)
	{
	    float x, y;

		pd->sprite->getPosition(s, &x, &y);
		//move right
	    if (crankChange > 0) 
	    {
			if ((x + TURRET_SPEED) > RIGHT_BOUNDARY)
				pd->sprite->moveTo(s, RIGHT_BOUNDARY, y);
			else
	    	    pd->sprite->moveBy(s, TURRET_SPEED, 0);
	    }
		//move left
	    else
	    {
			if ((x - TURRET_SPEED) < LEFT_BOUNDARY)
				pd->sprite->moveTo(s, LEFT_BOUNDARY, y);
			else
				pd->sprite->moveBy(s, -TURRET_SPEED, 0);
	    }

		moveTimer = 0;
    }
	return;
}

/*
  TODO : ripe for #defines
*/
LCDSprite* createTurret()
{
	LCDSprite *player = pd->sprite->newSprite();
	
	pd->sprite->setUpdateFunction(player, updateTurret);

	LCDBitmap *turretImage = loadImageAtPath("images/turret_sprite");

	pd->sprite->setImage(player, turretImage, kBitmapUnflipped);
	pd->sprite->moveTo(player, 293, 178);
	pd->sprite->setZIndex(player, 1000);
	pd->sprite->addSprite(player);
	pd->sprite->setTag(player, kTurret);
	return player;
}

void buildGround()
{
	int i, j;
	int groundHeight = 0;

	for (i = 0; i < 16; i++)    //COLS
		for (j = 0; j < 3; j++) //ROWS
		{
			LCDSprite* ground = pd->sprite->newSprite();

			if (j < 2)
				pd->sprite->setImage(ground, ground1BMP, kBitmapUnflipped);
			else
				pd->sprite->setImage(ground, ground3BMP, kBitmapUnflipped);
			
			//pd->sprite->setUpdateFunction(ground, updateGround);
			int w;
			pd->graphics->getBitmapData(ground1BMP, &w, &groundHeight, NULL, NULL, NULL);
			PDRect cr = PDRectMake(0, 0, w, groundHeight);
			pd->sprite->setCollideRect(ground, cr);

			pd->sprite->moveTo(ground, (i * 10) + (i * 2)  + 198, (j * 10) + (j * 2) + 200);
			
			switch (j) 
			{
				case 0:
					pd->sprite->setTag(ground, kGround1);
					break;
				case 1:
					pd->sprite->setTag(ground, kGround2);
					break;
				case 2:
					pd->sprite->setTag(ground, kGround3);
					break;
				default:
					pd->sprite->setTag(ground, kGround1);
			}
			
			pd->sprite->addSprite(ground);
		}
}

void setupGame()
{
	setPD(pd);
	lastTime = pd->system->getCurrentTimeMilliseconds();
	turret = createTurret();
	buildGround();

	//must be after loadAssets()
	pd->graphics->setFont(font);
}

void loadAssets()
{
	const char* outErr = NULL;

	bulletBMP = loadImageAtPath("images/bullet");
	missileBMP = loadImageAtPath("images/missile");
	ground1BMP = loadImageAtPath("images/ground1");
	ground3BMP = loadImageAtPath("images/ground3");
	particleBMP = loadImageAtPath("images/explosion_particle");
	hudBorderBMP = loadImageAtPath("images/HUD_border");
	gameFieldBorderBMP = loadImageAtPath("images/gamefield_border");
	hudSpacerBMP = loadImageAtPath("images/HUD_spacer");
	gameoverBMP = loadImageAtPath("images/gameoverscreen");

	font = pd->graphics->loadFont("fonts/copsnrobbersauto", &outErr);
	if (outErr != NULL) 
	{
		pd->system->logToConsole("Error loading font : %s", outErr);
	}
}

int drawHUD()
{
	char scoreText[20];
	char multiplierText[20];
	char threatText[20];
	char highScoreText[20];

	sprintf(scoreText, "SCORE  %d", score);
	sprintf(multiplierText, "MULTIPLIER  %d", multiplier);
	sprintf(threatText, "THREAT LEV  %d", threat);
	sprintf(highScoreText, "HIGH SCORE  %d", highScore);

	pd->graphics->drawText(scoreText, strlen(scoreText), kASCIIEncoding, 20, 20);
	pd->graphics->drawText(multiplierText, strlen(multiplierText), kASCIIEncoding, 20, 50);
	pd->graphics->drawText(threatText, strlen(threatText), kASCIIEncoding, 20, 80);

	//high score logic
	pd->graphics->drawBitmap(hudSpacerBMP, 10, 120, kBitmapUnflipped);
	pd->graphics->drawText(highScoreText, strlen(highScoreText), kASCIIEncoding, 20, 140);
}

int updateMenu(void* userdata)
{
	PDButtons current;
	pd->system->getButtonState(&current, NULL, NULL);

	if (current & kButtonA)
	{
		gameState = Play;
		setupGame();
	}
}

int updatePlay(void* userdata)
{
	int i = 0;
	int saveTime = pd->system->getCurrentTimeMilliseconds();

	deltaTime = saveTime - lastTime;
	lastTime = saveTime;

	if (missileTimer < 850)
	{
		missileTimer += deltaTime;
	}
	if (missileTimer >= 850)
	{
		missileTimer = 0;
		if (missileCounter < MAX_MISSILES)
		    launchMissile();
	}

	//draw sprites, calls clear(), so custom draws
	//need to appear after this call
	pd->sprite->updateAndDrawSprites();

	//draw borders
	pd->graphics->drawBitmap(hudBorderBMP, 0, 2, kBitmapUnflipped);
	pd->graphics->drawBitmap(gameFieldBorderBMP, 174, 0, kBitmapUnflipped);

	pd->system->drawFPS(200, 5);
	updateParticles(deltaTime);
	drawParticles();
	drawHUD();
	
	//1 lets playdate know to continue updating
	return 1;
}

void newGame()
{
	if (score > highScore)
	{
		writeHighScore(score);
		highScore = score;
	}

	canMove = 0;
	moveTimer = 0;
	shootTimer = 0;
	missileCounter = 0;
	missileTimer = 0;
	score = 0;
	multiplier = 1;
	threat = 1;

	pd->sprite->removeAllSprites();
	
	removeAllParticles();
	setupGame();
}

int updateGameOver(void* userdata)
{
	char* gameOverText = "GAME OVER";
	char* newGameText = "PRESS A TO CONTINUE";
	char* newHighScoreText = "NEW HIGH SCORE";
	char finalScore [20];
	
	drawHUD;
	pd->graphics->drawBitmap(gameoverBMP, 100, 50, kBitmapUnflipped);

	pd->graphics->drawText(gameOverText, strlen(gameOverText), kASCIIEncoding, 120, 65);

	sprintf(finalScore, "FINAL SCORE  %d", score);
	pd->graphics->drawText(finalScore, strlen(finalScore), kASCIIEncoding, 120, 85);

	if (score > highScore)
		pd->graphics->drawText(newHighScoreText, strlen(newHighScoreText), kASCIIEncoding, 120, 105);

	pd->graphics->drawText(newGameText, strlen(newGameText), kASCIIEncoding, 120, 125);

	PDButtons current;
	pd->system->getButtonState(&current, NULL, NULL);

	if (current & kButtonA)
	{
		newGame();
		gameState = Play;
	}
}

int update(void* userdata)
{
	if (gameState == Play)
	{
		updatePlay(userdata);
	}
	else if (gameState == GameOver)
	{
		updateGameOver(userdata);
	}
	else if (gameState == Menu)
	{
		updateMenu(userdata);
	}
}