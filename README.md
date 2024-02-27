# Train_multithreading
This program, coded in C, emulates a train crossing between an East and a west Station, given an input file.

Each train is represented by a thread using mutexes and condition variables. 

Each train is given a priority, a direction, a load time and a cross time.

The crossing priority is given these rules.
- Only one train can cross at a time.
- A train needs to be loaded to cross the track
- A train with high priority will cross before low priority
- If trains are travelling in the same direction with the same priority, the one that finished loading first will cross first
- If trains are travelling in the opposite direction with the same priority, pick the direction that last crossed or if non-crossed, the westbound train
- If three trains cross back to back from one direction, send a train from the other direction to avoid starvation.

--Example input file--
w 15 9
E 8 6
E 9 5
E 9 3
w 10 4
e 11 5

- w is for the west, and e is for the east.
- Capitalisation denotes if it is a high-priority or low-priority train.
- the first number is load time in 10ths of a second,
- The second number is crossing time in the 10ths of a second.
