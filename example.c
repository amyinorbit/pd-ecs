/*===--------------------------------------------------------------------------------------------===
 * example.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2022 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "ecs.h"
#include <stdio.h>

typedef struct {
    float x, y;
} Speed;

typedef struct {
    float x, y;
} Position;

ECSID kSpeed = ECS_MAX_COMPS;
ECSID kPosition = ECS_MAX_COMPS;

void moveEntities(ECS *world, Entity e, void *userData) {
    (void)userData;
    Position *pos = addComponentID(world, e, kPosition);
    Speed *speed = addComponentID(world, e, kSpeed);
    
    pos->x += speed->x;
    pos->y += speed->y;
}

int main() {
    
    // Create a "world"
    ECS *world = newECS();
    
    // Register our component types.
    kSpeed = ECS_COMPONENT(world, Speed);
    kPosition = ECS_COMPONENT(world, Position);
    
    // Create some entities!
    Entity e = newEntity(world);
    
    // Add some components, either with a known ID or using ID retrieval
    Position *pos = addComponentID(world, e, kPosition);
    pos->x = 0;
    pos->y = 0;

    Speed *speed = addComponent(world, e, Speed);
    speed->x = 0.2;
    speed->y = 0.2;
    
    // Create systems that operate on component sets
    ECSID physics = newSystem(world, componentMask(2, kPosition, kSpeed), moveEntities, NULL);
    
    // If needed, you can remove systems
    destroySystem(ecs, physics);
    
    // Every frame, advance your ECS world
    ecsTick(world);
    
    destroyECS(world);
}
