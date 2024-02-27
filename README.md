# Train_multithreading
This program, coded in C, emulates a train crossing between an East and a west Station, given an input file.

Each train is represented by a thread using mutexes and condition variables. 

Each train is given a priority, a direction, a load time and a cross time.

The crossing priority is given these rules.
- Only one train can cross at a time.
- A train needs to be loaded to cross the track
- A train with high priority will cross before low priority
- If trains are travelling in the same direction with the same priority, the one that finished loading first will cross first
- If trains are travelling in the opposite direction with same priority, pick the direction that last crossed or if non crossed, the westbound train
- If three trains cross back to back from one direction, send a train from the other direction to avoid starvation.
