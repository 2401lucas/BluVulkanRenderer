# BluRenderer
 
## ECS 
This was my first experience working with ECS, I have read about [Unity's DOTS implementation](https://unity.com/dots) and didn't appreciate it's design until I had a deep understanding of Game Engines and Renderers.
I opted to try and follow Unity's implementation. Instead of grouping similar components together, entities with similar components are grouped together. This has a great benefit when itererating over entities as there's very little checking to do and data is quite close together. A major drawback is when adding or removing components, all of the entities component data needs to be moved to another pool. My solution to work around this is to force components to be specified before the entity is created, ensuring that it only needs to be moved to a pool once. This design has it's drawbacks, however in my use case I know all of an entities required components before object creation.  
