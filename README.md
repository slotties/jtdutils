# Compile

    ./build_all.sh

All binaries are found in the `bin` directory.

# Tools

## tdls

Lists all threads with their states:

    > jstack [some pid] | tdls
    http-bio-8080-exec-106              P  94987
    http-bio-8080-exec-107              P  90895
    http-bio-8080-exec-108              P  93511
    [...]

## tdstats

Prints basic statistics for every dump:

    > jstack [some pid] | tdstats
                                       |  RUN | WAIT | TIMED_WAIT | PARK | BLOCK
    -----------------------------------|------|------|------------|------|-------
    2015-03-09 20:15:13                    22      3           18     92       0
    
## tdlocks

Lists all locks and the threads that either hold or wait on them:

    > jstack [some pid] | tdlocks
    Dump: 2015-03-09 20:16:15
    "MultiThreadedHttpConnectionManager cleanup" holds 0x00000007d5898c10 (java.lang.ref.ReferenceQueue$Lock)
    - MultiThreadedHttpConnectionManager cleanup
    
## tdgrep

Filters threads by specific criteria:

    > jstack [some pid] | tdgrep -n "Gang.*"
    "Gang worker#0 (Parallel GC Threads)" prio=5 tid=0x00007fb6ca804000 nid=0x2103 runnable 

    "Gang worker#1 (Parallel GC Threads)" prio=5 tid=0x00007fb6ca804800 nid=0x2303 runnable 

    "Gang worker#2 (Parallel GC Threads)" prio=5 tid=0x00007fb6c9801800 nid=0x2503 runnable 

    "Gang worker#3 (Parallel GC Threads)" prio=5 tid=0x00007fb6c9802000 nid=0x2703 runnable 
    
    
