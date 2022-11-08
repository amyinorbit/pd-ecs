/*===--------------------------------------------------------------------------------------------===
 * ecs.h
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2022 Amy Parent
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#ifndef _ECS_H_
#define _ECS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ECS_MAX_ENTITIES
#define ECS_MAX_ENTITIES    (128)
#endif

#ifndef ECS_MAX_COMPS
#define ECS_MAX_COMPS       (8)
#endif

#ifndef ECS_MAX_SYSTEMS
#define ECS_MAX_SYSTEMS     (32)
#endif

#define ECS_COMPMASK_BYTES  (1 + (ECS_MAX_COMPS-1)/8)
#define ECS_ALL_COMP_MASK   ((1 << ECS_MAX_COMPS) - 1)

#if ECS_COMPMASK_BYTES == 1
typedef uint8_t ComponentMask;
#elif ECS_COMPMASK_BYTES == 2
typedef uint16_t ComponentMask;
#else
typedef uint32_t ComponentMask;
#endif

typedef uint8_t     ECSID;
typedef uint16_t    Index;
typedef uint32_t    Entity;
typedef struct ECS  ECS;

typedef void ECSIterator(ECS *, Entity, void *);

#ifdef NDEBUG
#define ASSERT(expr)
#else
#define ASSERT(expr) assertImpl(__FILE__, __LINE__, #expr, expr)
#endif

#define ECS_COMPONENT(ecs, T) ecsDeclareComponent(ecs, #T, sizeof(T))
#define ECS_ID(ecs, T) ecsComponentID(ecs, #T)
#define ECS_MASK(ecs, T) (1 << ECS_ID(ecs, T))


void assertImpl(const char *file, int line, const char *exprStr, bool expr);

/**
 * Creates a new ECS registry.
 * @return A newly allocated ECS registry.
 */
ECS *newECS(void);

/**
 * Destroys an ECS registry.
 * @param ecs The ECS registry to destroy.
 */
void destroyECS(ECS *ecs);

/**
 * Returns a component mask from a set of component type IDs.
 * @param count The number of type IDs.
 * @param ... The type IDs.
 * @return A mask that represent the set of component types.
 */
ComponentMask componentMask(unsigned count, ...);

/**
 * Registers a new type of component that can be attached to entities.
 * @param ecs The ECS regsitry in which to register the component type.
 * @param id The string identifying the component type.
 * @param size The size of the type's components.
 * @return A unique identifier for the component type.
 */
ECSID ecsDeclareComponent(ECS *ecs, const char *id, size_t size);

/**
 * Returns the unique identifier for a component type.
 * @param ecs The registry in which the component type is registered.
 * @param id The string identifiying the component type.
 * @return A unique identifier for the component type.
 */
ECSID ecsComponentID(const ECS *ecs, const char *id);

/**
 * Returns the index mapped to an entity.
 * @param handle The entity handle.
 * @return The index into the component tables associated with the entity.
 */
static inline uint16_t entityIndex(Entity handle) {
    return handle & 0xffff;
}

/**
 * Returns the generation of an entity. If the generation is lower than the one in the
 * entity table, the handle is stale and does not point to a valid entity anymore.
 * @param handle The entity handle.
 * @return The index into the component tables associated with the entity.
 */
static inline uint16_t entityGen(Entity handle) {
    return (handle >> 16) & 0xffff;
}

/**
 * Creates a new entity in `ecs`, and returns a handle to it
 * @param ecs the ecs "registry" to create the entity in.
 * @return The handle to the new entity.
 */
Entity newEntity(ECS *ecs);

/**
 * Creates a new entity with a list of components in `ecs`, and returns a handle to it
 * @param ecs The ecs "registry" to create the entity in.
 * @param archetype A bitmask of all the component types that the entity has.
 * @return The handle to the new entity.
 */
Entity newEntityWithArchetype(ECS *ecs, ComponentMask archetype);

/**
 * Removes an entity from `ecs` and marks it for reuse.
 * @param ecs The ECS registry that `entity` belongs to.
 * @param entity The handle of the entity to remove.
 */
void destroyEntity(ECS *ecs, Entity entity);

/**
 * Adds a component identified by `id` to an entity.
 * @param ecs The ECS registry in which the entity and compoennt type are registered.
 * @param entity The entity to which the component is to be added.
 * @param id The unique ID of the new component's type.
 * @return A pointer to the new component's data.
 */
void *addComponentID(ECS *ecs, Entity entity, ECSID id);

/**
 * Adds a component identified by `id` to an entity.
 * @param ecs The ECS registry in which the entity and compoennt type are registered.
 * @param entity The entity to which the component is to be added.
 * @param T The new component's type.
 * @return A pointer to the new component's data.
 */
#define addComponent(ecs, entity, T) ((T *)addComponentID((ecs), (entity), ECS_COMPONENT(ecs, T)))

/**
 * Returns a pointer to a entity's given component's data.
 * @param ecs The ECS registry in which the entity and compoennt type are registered.
 * @param entity The entity for which to get the component's data.
 * @param id The unique ID of the component's type.
 * @return A pointer to the component's data.
 */
void *getComponentID(ECS *ecs, Entity entity, ECSID id);

/**
 * Returns a pointer to a entity's given component's data.
 * @param ecs The ECS registry in which the entity and compoennt type are registered.
 * @param entity The entity for which to get the component's data.
 * @param T  The component's type.
 * @return A pointer to the component's data.
 */
#define getComponent(ecs, entity, T) ((T *)addComponentID((ecs), (entity), ECS_COMPONENT(ecs, T)))

/**
 * Removes a component from a given entity.
 * @param ecs The ECS registry in which the entity and component type are registered.
 * @param entity The entity for which to remove the component.
 * @param id The unique ID of the component's type.
 */
void removeComponentID(ECS *ecs, Entity entity, ECSID id);

/**
 * Removes a component from a given entity.
 * @param ecs The ECS registry in which the entity and component type are registered.
 * @param entity The entity for which to remove the component.
 * @param T  The component's type.
 */
#define removeComponent(ecs, entity, T) removeComponentID((ecs), (entity), ECS_COMPONENT(ecs, T))

/**
 * Returns whether a handle points to an valid, active entity.
 * @param ecs The ECS registry in which the entity is is registered.
 * @param entity  The entity handle to check.
 * @return Whether `entity` is a handle pointing to a valid entity in `ecs`.
 */
bool isEntityValid(const ECS *ecs, Entity entity);

/**
 * Calls a function for each entity in a registry that contains the given components.
 * @param ecs The ECS registry in which to match entities.
 * @param mask A set of component types that entities must contain to match.
 * @param func A function to call for each entity matching `mask`.
 * @param data An arbitrary pointer passed to `func`.
 */
void matchEntities(ECS *ecs, ComponentMask mask, ECSIterator func, void *data);


/**
 * Creates a new system in an ECS registry that works over entities with a given set of component Types.
 * @param ecs The ECS registry in which the system will be created.
 * @param mask A set of component types that the new system will operate on.
 * @param func A function called for each entity matching the system's component set.
 * @param data An arbitrary pointer passed to the system's function.
 * @return A unique identifier that refers to the system.
 */
ECSID newSystem(ECS *ecs, ComponentMask mask, ECSIterator func, void *data);

/**
 * Destroys a given system in the given ECS registry.
 * @param ecs The ECS registry in which the system exists.
 * @param id The unique identifier of the system to destroy.
 */
void destroySystem(ECS *ecs, ECSID id);

/**
 * Run all system in a given ECS registry once.
 * @param ecs The ECS to advance.
 */
void ecsTick(ECS *ecs);


#ifdef __cplusplus
}
#endif

#endif /* ifndef _ECS_H_ */
