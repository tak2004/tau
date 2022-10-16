# FLUCDV or Fragmentability, Locality, Utilization, Contention, Density and Variation

## Fragmentability

You can assign each structure a fragmentability factor.
A struct with no additional external data you can assign a factor of 0.
```
struct RGB{// F=0
    u8 r
    u8 g
    u8 b
}
struct RGBA{// F=0
    embed RGB
    u8 a
}
```
A structure with external data get a higher factor for each external information.
```
struct RGBA{// F=1
    ptr<RGB> rgb //+1
    u8 a
}
struct User{// F=4
    size id
    u8 nameBytes
    u8[32] name    
    u256 passwordHash
    size billingAccountId//+1 Because billing data are somewhere else.
    size socialAccountId//+1 Because social service data are somewhere else.
    vec<size> unlockedLevelIds//+2 Because vec is a memory block and the id's in there are pointing to other data.
}
```
It's important to keep F as low as possible to improve locality.

## Locality

You should design your data in a way to fit their processing of them. If you build a phone book with each entry containing a name, phone number, etc. and you just want to pick all people containing a specific surname then you pick for each entry also the other data and reduce data locality.
```
struct PhoneBookEntry{
    str<32> surname
    str<32> forname
    u8[16] number
    ...
}

// Alphabetical sorted map of vector of PhoneBookEntry
getBobs(map<u8,vec<PhoneBookEntry>> phoneBook)->vec<size>{
    vec<size> result
    size i = 0
    for letter,entries in phoneBook{
        for e in entries{
          if e.forname == "Bob"{
            result << i
          }
          ++i
        }
    }
    return result
}
```
Instead you should think about locality and split the data.
```
struct PhoneBook{
    vec<str<32>> surname
    vec<str<32>> forname
    vec<u8[16]> number
    ...
}
getBobs(PhoneBook phoneBook)->vec<size>{
    vec<size> result
    size i = 0
    for e in phoneBook.forname{
        if e == "Bob"{
          result << i
        }
        ++i
    }
    return result
}
```
It's likely to be better to keep information multiple times instead of one time but strongly spread.

## Utilization

## Contention

## Density

Data density means using as less bytes as possible to get all data fitting into it. This improves caching but could additional penalties to processing. A very common case is a SIMD operation needing a specific alignment of the data.

## Variation

# Memory allocator

The major goal is the best performance for the specific use case. There are many general purpose allocators available but they are general purpose but don't fit well for applications like stateless servers or game engines. Both of them run a hot loop which mostly does the same pattern again and again.
The general purpose allocator will diffuse and fragment over time which restricts the run time of the application to a finite amount.

## Arena

Arena-based allocators will prevent fragmentation by design and can run infinitely. They also allow avoiding freeing objects/memory by a simple reset routine. Arenas are describing how you organize your data and the lifetime of each object.