# Memory 

## Storage class decorator

The storage class decoration describes the lifetime, access scope and initialization behaviour. Of an object. Following values are supported.

| Class    | Lifetime | Resides      | Init         | Free           | C++ equivalent      |
|----------|----------|--------------|--------------|----------------|---------------------|
| local    | Scope    | Stack        | enter scope  | Leave scope    | auto                |
| static   | Process  | Data-section | image start  | at exit        | static              |
| register | Scope    | CPU register | enter scope  | Leave scope    |                     |
| dynamic  | Dynamic  | Heap         | enter scope  | no longer used | new/delete          |
| thread   | Process  | Heap         | thread spawn | at thread exit | thread_local static |

## Read/Write or ReadOnly

An object can be defined mutable or read-only after initialization. By default read-only is used and *mut* allows to change the behaviour to read and write.
Inmutable variables as default behaviour is intented to reduce the accidental overwrite which introduce bugs.

## Initialization and freeing

C++ use constructor and destructor for objects to customize the initialization and freeing of them. But tau don't support classes that way. It use structs as model like C and functions can be attached to a struct to feel like a classes with methods.
The solution to allow implicit initialization and freeing the operator mechanic is used.

Operator | C++ equivalent      | Example
-|-|-
init     | default constructor | operator init(mem self){ self.data = 0 self.bytes = 0}
free     | default destructor  | operator free(mem self){ memory.free(self) }

You can have also free/init named functions which don't conflict with the operator because the syntax don't allow to call a operator directly and the compiler only consider the operator functions for the default init and free. This functions would be usefull for early freeing or lazy construction.

The equivalent to a custom C++ constructor with several parameters is a tau function of the same name as the struct with the struct as result. Because tau don't know constructor a function with the name of the struct can't be ambigous.

```
// Overwrite the default free operator of the mem struct.
operator free(mem self){ 
  // Free the system memory.
  memory.free(self)
}

// Factory function for the mem struct.
mem(size bytes) -> mem {
  // Allocate the specified amount of bytes as system memory.
  mem result = memory.alloc(bytes)
  return result
}

// Initialize the block with the factory function.
mem block = mem(32)
// End of scope. A automatic generated call of the free operator happens now.
```