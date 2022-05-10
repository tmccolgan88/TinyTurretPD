#include "pd_api.h"

void setPD(PlaydateAPI *pd);
void addParticleBurst(LCDBitmap *particleBMP, int x, int y);
void updateParticles(int deltaTime);
void removeAllParticles(void);
int drawParticles(void);