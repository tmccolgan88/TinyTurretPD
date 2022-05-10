/*
  Particle Framework

  Author : Tim McColgan
  Date : 04/27/2022

  Purpose : Provde a set of structs and functions to create/update/draw particles

  A group of particles (burst), is stored as a linked list of individual particles.  Those lists are children
  to a ParticleBurst linked list.
*/

#include "particles.h"

PlaydateAPI *pd;

typedef struct Particle
{
	LCDBitmap *bmp;
	int x, y;
	int dx, dy;
	int deltaTime;
	int timer;

} Particle;

typedef struct ParticleListNode
{
	Particle *p;
	struct ParticleListNode *next;

} ParticleListNode;

typedef struct ParticleBurstNode
{
	ParticleListNode* burstHead;
	struct ParticleBurstNode* next;
} ParticleBurstNode;

ParticleBurstNode *particleBurstHead = NULL;
ParticleBurstNode *particleBurstCurrent = NULL;

void setPD(PlaydateAPI *playdate)
{
  pd = playdate;
}

void addParticleBurst(LCDBitmap *particleBMP, int x, int y)
{
	ParticleListNode *headParticle = NULL;
    ParticleListNode *currentParticle = NULL;

	int i = 0;
	int numParticles = (rand() % 14) + 6;

	for (i = 0; i < numParticles; i++)
	{
		Particle* particle = pd->system->realloc(NULL, sizeof(Particle));
		particle->bmp = particleBMP;
		particle->x = x;
		particle->y = y;
		particle->dx = (rand() % 10) - 5;
		particle->dy = (rand() % 10) - 5;
		particle->timer = 200;
		particle->deltaTime = 0;

		ParticleListNode* node;
		node = pd->system->realloc(NULL, sizeof(ParticleListNode));
		node->p = particle;
		node->next = NULL; 

		if (headParticle == NULL)
		{
			headParticle = node;
            headParticle->next = NULL;

			currentParticle = headParticle;
		}
		else
		{
			currentParticle->next = node;
			currentParticle = currentParticle->next;
		}
	}

    ParticleBurstNode* burstNode = pd->system->realloc(NULL, sizeof(ParticleBurstNode));
    burstNode->burstHead = headParticle;
    burstNode->next = NULL;

    if (particleBurstHead == NULL)
    {
        particleBurstHead = burstNode;
        particleBurstHead->next = NULL;

        particleBurstCurrent = particleBurstHead;
    }
    else
    {
        particleBurstCurrent->next = burstNode;
        particleBurstCurrent = particleBurstCurrent->next;
        particleBurstCurrent->next = NULL;
    }
}

void updateParticles(int deltaTime)
{
    ParticleBurstNode* headBurstNode = particleBurstHead;
    ParticleListNode* particleNode = NULL;

    while (headBurstNode != NULL)
    {
        particleNode = headBurstNode->burstHead;

        while (particleNode != NULL)
        {
            particleNode->p->x += particleNode->p->dx;
		    particleNode->p->y += particleNode->p->dy;
		    particleNode->p->deltaTime += deltaTime;
            particleNode = particleNode->next;
        }

        headBurstNode = headBurstNode->next;
    }

}

void removeAllParticles()
{
    ParticleBurstNode* headBurstNode = particleBurstHead;
    ParticleBurstNode* burstSaveNode = NULL;
    ParticleListNode* particleNode = NULL;
    ParticleListNode* savedNode = NULL;

    while (headBurstNode != NULL)
    {
        particleNode = headBurstNode->burstHead;

	    while (particleNode != NULL)
	    {
            savedNode = particleNode->next;

		    pd->system->realloc(particleNode, sizeof(ParticleBurstNode));
		
		    particleNode = savedNode;
	    }

        burstSaveNode = headBurstNode;
        headBurstNode = headBurstNode->next;
        pd->system->realloc(burstSaveNode, sizeof(ParticleBurstNode));

    }
    particleBurstHead = NULL;
}

int drawParticles()
{
	ParticleBurstNode* headBurstNode = particleBurstHead;
    ParticleListNode* particleNode = NULL;

    while (headBurstNode != NULL)
    {
        particleNode = headBurstNode->burstHead;

	    while (particleNode != NULL)
	    {
		    if (particleNode->p->deltaTime < particleNode->p->timer)
			    pd->graphics->drawBitmap(particleNode->p->bmp, particleNode->p->x, particleNode->p->y, kBitmapUnflipped);
		
		    particleNode = particleNode->next;
	    }

        headBurstNode = headBurstNode->next;
    }
	return 1;
}