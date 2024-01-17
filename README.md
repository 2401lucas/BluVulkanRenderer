# BluRenderer
 
## ECS 
This was my first experience working with ECS, I have read about [Unity's DOTS implementation](https://unity.com/dots) and didn't appreciate it's design until I had a deep understanding of Game Engines and Renderers.
I opted to try and follow Unity's implementation. While of grouping similar components together, entities with similar components are also grouped together. This has a great benefit when itererating over entities as there's very little checking for the cpu to do and data is quite close together, greatly improving read speed from the ram to the cpu. A major drawback is when adding or removing components, all of the entities component data needs to be moved to another pool. 
