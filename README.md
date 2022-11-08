PD-ECS - A simple ECS for playdate-sized games
==============================================

PD-ECS is a very small, very basic ECS intened for use in small games. I'm writing it to support
small games I'm making for Panic's Playdate. Be warned it's extremeley bare bones, very much not
thoroughly tested, and might blow up!

How do I install PD-ECS?
------------------------

- Add `ecs.h` and `ecs.c` to your project.
- Optionally, you can use your build system to define some variables:
    - `ECS_MAX_ENTITIES`: the maximum number of entities supported by the ECS;
    - `ECS_MAX_COMPS`: the maximum number of components that can be registered on an entity;
    - `ECS_MAX_SYSTEMS`: the maximum number of systems that can operate.
- That's it!

For an example of how to actually use it in your code, have a look at [`example.c`](example.c).
It's extremely basic, but should give you an idea of how to get started!
