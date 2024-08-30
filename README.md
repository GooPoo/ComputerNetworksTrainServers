# CITS3002-Project
Repository for Computer Networks Project Sem 1 2024

```
Student 1: Benjamin Lee 22252344
Student 2: Olivia Morrison 23176135
Student 3: Min Thit 23375069
Student 4: Johnson Che 23403302
``` 

## End mark for the project: 28/30 (HD)
Reason for deductions:
- Timetable data was not verified for updates.
- Occasionally failed to display the quickest route correctly.

## Python
To run all three station python servers at once (in one terminal) run: 
```bash 
    ./startpythonstations.sh 
```

Otherwise, if you want to run one station in one terminal at a time, the commands for each station is:


```bash
    python3 station-server.py TerminalA 2401 2408 localhost:2410 &
    python3 station-server.py JunctionB 2402 2409 localhost:2410 &
    python3 station-server.py BusportC 2403 2410 localhost:2408 localhost:2408 &
```

Note, these commands may not work due to 'python3', if that is the case just replace 'python3' with 'python'.

Once started, these three stations can be accessed in your browser via:

```bash
    http://localhost:2401/
    http://localhost:2402/
    http://localhost:2403/
```

To end the processes running the stations and close the sockets, click on the "Close Sockets" button on the HTML page. If the process does not end use the command: 
```bash
    sudo lsof -i -n -P | grep Python
``` 
This will tell you the process ID, then you can kill it with:
```bash
    kill {PID}
```


## C
To run all three station python servers at once (in one terminal) first run to compile: 
```bash
    make
```

Then run:
```bash
    ./startCstations.sh
```

Note: If you get the error `-bash: ./startCstations.sh: Permission denied`, this is due to the script file not having execute permissions. To fix this:
```bash
    chmod +x startCstations.sh
```

Otherwise, if you want to run one station in one terminal at a time, the commands for each station is:

```bash
    ./station-server TerminalA 4444 5555 127.0.0.1:6666 &
    ./station-server JunctionB 4466 4447 127.0.0.1:6666 &
    ./station-server BusportC 4446 6666 127.0.0.1:5555 127.0.0.1:4447 &
```

NOTE: These numbers are ports, if you are having issues with binding, change the number to a free port. Furthermore, if you want to communicate with different computers on the same network for your servers, change the ip to your IPV4.

In WSL if the ports are open and you are struggling to close them: 
```bash
    lsof -i :(port)
``` 
This will tell you the process ID, then you can kill it with:
```bash
    kill {PID}
```

Once started, these three stations can be accessed in your browser via:

```bash
    http://localhost:4444/
    http://localhost:4466/
    http://localhost:4446/
```