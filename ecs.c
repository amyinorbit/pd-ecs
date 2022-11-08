/*===--------------------------------------------------------------------------------------------===
 * ecs.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2022 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "ecs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if TARGET_PLAYDATE==1
#include "pd_api.h"
extern PlaydateAPI *pd;
#endif

#define DECLARE_POOL(T, name, count)                        \
typedef struct {                                            \
    uint16_t freeCount;                                     \
    uint16_t freeList[count];                               \
    T data[count];                                          \
} name##Pool;                                               \
                                                            \
                                                            \
void init##name##Pool(name##Pool *pool) {                   \
    pool->freeCount = count;                                \
    memset(pool->data, 0, count * sizeof(T));               \
    for(uint16_t i = 0; i < count; ++i) {                   \
        pool->freeList[i] = count - (i+1);                  \
    }                                                       \
}                                                           \
                                                            \
                                                            \
                                                            \
uint16_t new##name##FromPool(name##Pool *pool) {            \
    ASSERT(pool->freeCount);                                \
    uint16_t id = pool->freeList[--pool->freeCount];        \
    return id;                                              \
}\
                                                            \
void return##name##ToPool(name##Pool *pool, uint16_t id) {  \
    ASSERT(pool->freeCount < count);                        \
    pool->freeList[pool->freeCount++] = id;                 \
}


typedef enum {
    kEntityUnused = 1 << 0,
} EntityFlags;

typedef struct {
    uint32_t        info;
    ComponentMask   components;
} EntityData;

typedef struct {
    size_t          size;
    char            id[64];
    uint8_t         data[];
} ComponentData;

typedef struct {
    ECSID           id;
    ComponentMask   mask;
    ECSIterator     *func;
    void            *userData;
} System;

DECLARE_POOL(EntityData, Entity, ECS_MAX_ENTITIES);
DECLARE_POOL(System, System, ECS_MAX_SYSTEMS);


struct ECS {
    EntityPool      entities;
    
    uint8_t         compDataCount;
    ComponentData   *compData[ECS_MAX_COMPS];
    
    uint8_t         nextSystemID;
    uint8_t         systemCount;
    System          systems[ECS_MAX_SYSTEMS];
//    SystemPool      systems;
};


void assertImpl(const char *file, int line, const char *exprStr, bool expr) {
#if TARGET_PLAYDATE==1
    if(expr) return;
    pd->system->error("%s:%d: assertion `%s' failed", file, line, exprStr);
#else
    if(expr) return;
    fprintf(stderr, "%s:%d: assertion `%s' failed", file, line, exprStr);
    abort();
#endif
}

// MARK: - Handle Utilities

static inline uint8_t generation(EntityData data) {
    return data.info & 0xffff;
}

static inline uint16_t flags(EntityData data) {
    return (data.info >> 16) & 0xffff;
}

static inline Entity createHandle(uint16_t index, uint8_t generation) {
    return ((Entity)generation << 16) | ((Entity)index);
}

static inline EntityData createEntityData(uint8_t generation) {
    return (EntityData) {
        .info = generation,
        .components = 0
    };
}

ComponentMask componentMask(unsigned count, ...) {
    ComponentMask mask = 0;
    
    va_list args;
    va_start(args, count);
    
    for(unsigned i = 0; i < count; ++i) {
        unsigned id = va_arg(args, unsigned);
        mask |= (1 << (ComponentMask)id);
    }
    
    va_end(args);
    return mask;
}

ECS *newECS(void) {
    ECS *ecs = malloc(sizeof(*ecs));
    ecs->entities.freeCount = ECS_MAX_ENTITIES;
    ecs->compDataCount = 0;
    
    ecs->systemCount = 0;
    ecs->nextSystemID = 0;
    initEntityPool(&ecs->entities);
    return ecs;
}

void destroyECS(ECS *ecs) {
    free(ecs);
}

// MARK: - Component Handling

uint8_t ecsComponentID(const ECS *ecs, const char *id) {
    for(uint8_t i = 0; i < ecs->compDataCount; ++i) {
        if(!strcmp(ecs->compData[i]->id, id)) return i;
    }
    return ECS_MAX_COMPS;
}

uint8_t ecsDeclareComponent(ECS *ecs, const char *compID, size_t size) {
    uint8_t id = ecsComponentID(ecs, compID);
    if(id != ECS_MAX_COMPS) return id;
    ASSERT(ecs->compDataCount < ECS_MAX_COMPS);
    
    ComponentData *data = malloc(sizeof(ComponentData) + ECS_MAX_ENTITIES * size);
    strcpy(data->id, compID);
    data->size = size;
    memset(data->data, 0, ECS_MAX_ENTITIES * size);
    ecs->compData[ecs->compDataCount] = data;
    return ecs->compDataCount++;
}

// MARK: - Entity Handling

Entity newEntity(ECS *ecs) {
    if(!ecs->entities.freeCount) {
#ifdef TARGET_PLAYDATE
        pd->system->error("No more free entities");
#else
        abort();
#endif
    }
    
    uint16_t id = newEntityFromPool(&ecs->entities);
    uint8_t gen = generation(ecs->entities.data[id]);
    ecs->entities.data[id] = createEntityData(gen);
    return createHandle(id, gen);
}

Entity newEntityWithArchetype(ECS *ecs, ComponentMask archetype) {
    Entity e = newEntity(ecs);
    ecs->entities.data[entityIndex(e)].components = archetype;
    return e;
}

bool isEntityValid(const ECS *ecs, Entity entity) {
    uint16_t gen = entityGen(entity);
    return generation(ecs->entities.data[entityIndex(entity)]) == gen;
}

void destroyEntity(ECS *ecs, Entity entity) {
    if(!isEntityValid(ecs, entity)) return;
    uint16_t id = entityIndex(entity);
    uint16_t gen = entityGen(entity);
    ecs->entities.data[id] = createEntityData(gen+1);
    returnEntityToPool(&ecs->entities, id);
}

void *addComponentID(ECS *ecs, Entity entity, uint8_t compID) {
    ASSERT(compID < ECS_MAX_COMPS);
    ASSERT(isEntityValid(ecs, entity));
    uint16_t id = entityIndex(entity);
    ecs->entities.data[id].components |= (1 << compID);
    return ecs->compData[compID]->data + compID * ecs->compData[compID]->size;
}

void *getComponentID(ECS *ecs, Entity entity, uint8_t compID) {
    ASSERT(compID < ECS_MAX_COMPS);
    ASSERT(isEntityValid(ecs, entity));
    uint16_t id = entityIndex(entity);
    if((ecs->entities.data[id].components & (1 << compID)) == 0) return NULL;
    return ecs->compData[compID]->data + compID * ecs->compData[compID]->size;
}

void removeComponentID(ECS *ecs, Entity entity, uint8_t compID) {
    ASSERT(compID < ECS_MAX_COMPS);
    ASSERT(isEntityValid(ecs, entity));
    uint16_t id = entityIndex(entity);
    ecs->entities.data[id].components &= ~(1 << compID);
}

// MARK: - Systems and matchers

ECSID newSystem(ECS *ecs, ComponentMask mask, ECSIterator it, void *data) {
    ASSERT(ecs->systemCount < ECS_MAX_SYSTEMS);
    ASSERT(it != NULL);
    
    ecs->systems[ecs->systemCount++] = (System) {
        .id = ecs->nextSystemID++,
        .mask = mask,
        .func = it,
        .userData = data
    };
    return 0;
}

static System *findSystem(ECS *ecs, ECSID id) {
    for(uint8_t i = 0; i < ecs->systemCount; ++i) {
        if(id == ecs->systems[i].id) return &ecs->systems[i];
    }
    return NULL;
}

void destroySystem(ECS *ecs, ECSID id) {
    ASSERT(ecs->systemCount > 0);
    
    System *sys = findSystem(ecs, id);
    if(!sys) return;
    
    System *begin = sys + 1;
    System *end = ecs->systems+ecs->systemCount;
    memmove(begin, sys, sizeof(System) * (end - begin));
    ecs->systemCount -= 1;
}

void ecsTick(ECS *ecs) {
    for(uint8_t i = 0; i < ecs->systemCount; ++i) {
        matchEntities(ecs,
                      ecs->systems[i].mask,
                      ecs->systems[i].func,
                      ecs->systems[i].userData);
    }
}


void matchEntities(ECS *ecs, ComponentMask mask, ECSIterator it, void *userData) {
    
    for(uint16_t id = 0; id < ECS_MAX_ENTITIES; ++id) {
        EntityData data = ecs->entities.data[id];
        if(flags(data) & kEntityUnused) continue;
        if((data.components & mask) != mask) continue;
        it(ecs, createHandle(id, generation(data)), userData);
    }
}
