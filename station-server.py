# Student 1: Benjamin Lee 22252344
# Student 2: Olivia Morrison 23176135
# Student 3: Min Thit 23375069
# Student 4: Johnson Che 23403302

import socket
import select
import sys
import time
import queue

MAX_NEIGHBOURS = 20
CHUNK_SIZE = 1024

neighbours = []    # list of neighbours
num_neighbours = 0

# -------------------------------------------------------------------------------------------------------
def read_bus_schedule(filename):
    schedules = []
    with open(filename, 'r') as file:
        for line in file:
            line = line.strip()
            if not line.startswith("#"):  # Ignore comments
                data = line.split(',')
                if len(data) == 5:    # Assuming it's schedule data
                    departure_time, route_name, departing_from, arrival_time, arrival_station = data
                    schedules.append({'departure_time': departure_time, 'route_name': route_name, 'departing_from': departing_from, 'arrival_time': arrival_time, 'arrival_station': arrival_station})
    return schedules

# -------------------------------------------------------------------------------------------------------
def send_UDP_datagram():

    # Send to all neighbours - FOR TESTING
    for n in neighbours:
        # Prepare destination address
        dest_addr = (n.get("ip"), int(n.get("port")))

        # Send UDP datagram
        udp_message = "Hello UDP Server!"
        udp_server_socket.sendto(udp_message.encode('utf-8'), dest_addr)

# -------------------------------------------------------------------------------------------------------
def handle_TCP_request(client_socket, buffer):

    # Parse HTTP request
    request_lines = buffer.split('\r\n')
    request_line = request_lines[0].split(' ')
    if len(request_line) != 3:
        print("Invalid HTTP request")
        return
    method, path, _ = request_line

    # Get HTTP Payload
    payload = request_lines[-1]

    html_content = ""

    # Check requested path
    if path == "/":
        # Send HTML page
        filename = "index.html"

        with open(filename, 'r') as file:
            # read HTML file into content buffer
            content = file.read()
            # Close file
            file.close
        
        # Find where "<span id=\"station_name\"></span>" is in HTML content
        name_index = content.find("<span id=\"station_name\"></span>")
        span_length = len("<span id=\"station_name\"></span>")
        end_of_span = name_index + span_length

        # Place station name in span tag
        html_content = content[0:name_index]
        html_content += station_name
        html_content += content[end_of_span:]

    elif path == "/about":
        # About page
        filename = "about.html"

    elif path == "/send-udp":
        # Send UDP datagram
        send_UDP_datagram()
        print("sending UDP")
        return
    
    # FOR CONVIENIENCE - DELETE LATER
    elif path == "/close-sockets": 
        print("Request: close sockets")
        close_sockets()
        sys.exit(0)

    elif path == "/info-neighbours":

        # Check for unknown neighbours names
        unknown_neighbours = get_unknown_name()

        if unknown_neighbours > 0:
            time.sleep(1)

        # Display neighbours data on HTML page
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"

        # Get info of each neighbour
        for n in neighbours:
            html_content += "Name: %s, IP: %s, Port: %s <br>" % (n.get("name"), n.get("ip"), n.get("port"))
        
    elif path == "/findJourneyResult":
        # Find a route to a given destination after a given time

        # Check for unknown neighbours names
        unknown_neighbours = get_unknown_name()

        # If there are unknown neighbours, wait for their response
        if unknown_neighbours > 0:
            time.sleep(1)

        # Get user input values from payload
        payload_data = payload.split('&')
        payload_data_name = payload_data[0].split('=')
        payload_data_time = payload_data[1].split('=')

        # Assuming destination and time values are both valid
        destination = payload_data_name[1]
        t_time = payload_data_time[1] 
        print(f"destination: {destination}, time: {t_time}")

        # Check if valid station and time has been entered by the user
        if check_valid_time(t_time) == -1:
            html_content += "Invalid time entered"

        elif destination == station_name:
            html_content += "You're already at %s please enter a valid destination" % (station_name)

        else:
            # Find route
            msg = find_journey(destination, t_time, []) 

            if msg is None:
                # If no direct route, wait for UDP responses from neighbours
                print("waiting ...")
                fds_waiting.append(
                    {
                        "socket": client_socket,
                        "destination": destination,
                        "time": t_time, 
                        "waiting": True,
                        "wait_count": 0
                    }
                )
                queue_msg = "waiting&%s&" % (destination)
                message_queues[client_socket].put(queue_msg)
                return
            
            else:
                # Found a direct route
                route_dt, route_name, route_df, route_at, route_as = msg.split('|')
                html_content += "Departure Time: %s, Route: %s, Departing From: %s, Arrival Time: %s, Arrival Station: %s <br>" % (route_dt, route_name, route_df, route_at, route_as)
    
    elif path == "/timetable-data":
        # Display timetable data on HTML page
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"

        filename = "tt-" + station_name
        schedule = read_bus_schedule(filename)

        for x in schedule:
            html_content += "Departure Time: %s, Route: %s, Departing From: %s, Arrival Time: %s, Arrival Station: %s <br>" % (x.get("departure_time"), x.get("route_name"), x.get("departing_from"), x.get("arrival_time"), x.get("arrival_station"))
        
    else:
        # Path not found
        response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><head><title> 404 Not Found </title></head><body><h1> 404 Not Found</h1><p> The requested URL was not found on this server. </p></body></html>"
        client_socket.send(response.encode('utf-8'))
        client_socket.close()
        return

    # Create HTTP response
    response = f"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n{html_content}"

    if len(response) == 44:
        print("waiting on UDP response")

    # Enqueue response to be sent
    message_queues[client_socket].put(response)

    html_content = ""

# -------------------------------------------------------------------------------------------------------
def handle_UDP_request():
    buffer_size = 1024

    # Receive UDP packet
    buffer, client_address = udp_server_socket.recvfrom(buffer_size)
    if not buffer:
        print("Error in receiving UDP data")
        return

    # Convert client IP address to string
    client_ip, client_port = client_address

    # Print the received UDP packet along with the source address
    print(f"{station_name} Received UDP packet from {client_ip}:{client_port}: {buffer.decode('utf-8')}")

    # Decode message
    msg = buffer.decode('utf-8')

    if msg[:11] == "My name is ":
        # find addr in neighbours + add name
        # check if msg received w/o errors?
        for n in neighbours:
            if n.get("port") == str(client_port):
                n["name"] = msg[11:]

    elif msg == "Unknown name":
        print(f"{station_name} received unknown name udp request")
        udp_msg = "My name is " + station_name
        bytes_sent = udp_server_socket.sendto(udp_msg.encode('utf-8'), client_address)

    elif msg[:4] == "GOAL":
        # Get info from msg - might need to change variable names (datagram)
        split_datagram = msg.split('&')

        header = split_datagram[0]
        destination = split_datagram[1]

        routes_taken = []
        visited = []
        for i in range(2,len(split_datagram)-1):
            # Get info of last route (to this current station)
            r_departure_time, r_route_name, r_departing_from, r_arrival_time, r_arrival_station = split_datagram[i].split('|')
            route_info = {
                "r_departure_time": r_departure_time,
                "r_route_name": r_route_name, 
                "r_departing_from": r_departing_from, 
                "r_arrival_time": r_arrival_time, 
                "r_arrival_station": r_arrival_station
            }
            routes_taken.append(route_info)
            visited.append(r_departing_from)
        
        # Check if destination can be reached after arrival time
        direct = find_journey(destination, r_arrival_time, visited)

        # If no direct route, udp's have been sent on to neighbours
        if direct == -1 or direct is None:
            print(station_name + "sent onto neighbours")
            return
        else: 
            # Else, direct route to goal destination has been found!
            # Construct a RESTURNVALID UDP Datagram
            udp_datagram = "RETURNVALID&%s&" % (destination)

            for i in range(2,len(split_datagram)-1):
                udp_datagram += split_datagram[i]
                udp_datagram += "&"

            udp_datagram += direct
            udp_datagram += "&"

            print(udp_datagram)
            bytes_sent = udp_server_socket.sendto(udp_datagram.encode('utf-8'), client_address)
            print(f"Sent {bytes_sent} bytes to {client_address[0]}:{client_address[1]}")

    elif msg[:11] == "RETURNVALID":
        split_datagram = msg.split('&')
        destination = split_datagram[1]

        final_route = ""

        for i in range(2,len(split_datagram)-1):
            route_info = split_datagram[i].split('|')
            final_route += "Departure Time: %s, Route: %s, Departing From: %s, Arrival Time: %s, Arrival Station: %s <br>" % (route_info[0], route_info[1], route_info[2], route_info[3], route_info[4])
        
        print(station_name + " FINAL ROUTE: " + final_route)

        # loop through message queues to find which socket is waiting for this response
        for conn in message_queues:

            for w in fds_waiting:
                if conn == w.get("socket") and destination == w.get("destination"):
                    # enqueue HTTP response
                    response = f"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n{final_route}"
                    message_queues[conn].put(response)

# -------------------------------------------------------------------------------------------------------
def send_name_to_neighbours():
    # sleep for 5 sec 
    time.sleep(5)

    for n in neighbours:
        # TO DO --> check w Ben if "\0" is needed
        # udp_msg = "My name is " + station_name + "\0"
        udp_msg = "My name is " + station_name
        dest_addr = (n.get("ip"), int(n.get("port")))
        bytes_sent = udp_server_socket.sendto(udp_msg.encode('utf-8'), dest_addr)
        print(f"Sent {bytes_sent} bytes to {dest_addr[0]}:{dest_addr[1]}")

# -------------------------------------------------------------------------------------------------------
def get_unknown_name():

    print("in unknown names")

    unknown_count = 0

    for n in neighbours:
        if n.get("name") == "Unknown":
            unknown_count+=1
            udp_msg = "Unknown name"
            dest_addr = (n.get("ip"), int(n.get("port")))
            bytes_sent = udp_server_socket.sendto(udp_msg.encode('utf-8'), dest_addr)
            print(f"Sent {bytes_sent} bytes to {dest_addr[0]}:{dest_addr[1]}")
    
    return unknown_count

# -------------------------------------------------------------------------------------------------------
def close_sockets():
    # Close sockets
    tcp_server_socket.shutdown(socket.SHUT_RDWR)
    tcp_server_socket.close()
    udp_server_socket.shutdown(socket.SHUT_RDWR)
    udp_server_socket.close()
    print("Sockets have been closed")

# -------------------------------------------------------------------------------------------------------
# Returns -1 if time provided by the user is invalid, otherwise returns 0
def check_valid_time(t_time):

    if len(t_time) != 5:
        return -1
    
    colon_count = 0

    for t in t_time:
        if t == ":":
            colon_count += 1
        if t not in "0123456789:":
            return -1
    
    if colon_count != 1:
        return -1
    
    hour = int(t_time[0:2]) 
    min = int(t_time[3:5])

    if hour > 12 or hour < 0 or min > 59 or min < 0:
        return -1
    
    return 0

# -------------------------------------------------------------------------------------------------------
# Function to compare time strings in HH:MM format
# Returns 1 if time1 > time2, -1 if time1 < time2, 0 if equal
def compare_times(time1, time2):

    hours1 = int(time1[0:2])
    hours2 = int(time2[0:2])
    mins1 = int(time1[3:5])
    mins2 = int(time2[3:5])

    if hours1 > hours2:
        return 1
    elif hours1 < hours2:
        return -1
    else:
        if mins1 > mins2:
            return 1
        elif mins1 < mins2:
            return -1
        else:
            return 0

# -------------------------------------------------------------------------------------------------------
def find_journey(destination, t_time, visited):

    # First check if destination is a direct neighbour and if there is a direct route after set time
    direct = get_direct_route(destination, t_time)
    
    # Check if file has changed?

    if direct == -1:
        # If destination is not a direct neighbour, send each neighbour UDP 
        get_route_from_neighbour(destination, t_time, visited)
        return
    else:
        # Else found a direct route to destination
        msg = "%s|%s|%s|%s|%s" % (direct.get("departure_time"), direct.get("route_name"), station_name, direct.get("arrival_time"), direct.get("arrival_station"))
        return msg

# -------------------------------------------------------------------------------------------------------
# Returns first avaliable route to a destination after a specified time (if a direction route exists), else returns -1
def get_direct_route(destination, t_time):

    # Get current schedule information - can change to get once and pass as param
    filename = "tt-" + station_name
    schedule = read_bus_schedule(filename)

    # First check if destination is a direct neighbour and if there is a direct route after set time
    for i in range(len(schedule)):
        # Check if departure time is after inputed time
        if compare_times(schedule[i].get("departure_time"), t_time) == 1: 
            # Return first avaliable route to destination if there is a direct route
            if schedule[i].get("arrival_station") == destination:
                return schedule[i]
    return -1

# -------------------------------------------------------------------------------------------------------
# Sends UDP's to neighbours to find route to destination
def get_route_from_neighbour(destination, t_time, visited):
    # Get current schedule information - can change to get once and pass as param
    filename = "tt-" + station_name
    schedule = read_bus_schedule(filename)

    for n in neighbours:

        # Don't send neighbour UDP request if already visited
        if n.get("name") in visited:
            continue
        else:
            # Once this loop finishes, each neighbour that can be travelled to from the current station after the inputed time will have been sent a UDP request
            for i in range(len(schedule)):
                if schedule[i].get("arrival_station") == n.get("name") and compare_times(schedule[i].get("departure_time"), t_time) == 1:
                    # Send UDP
                    st = schedule[i].get("arrival_station")
                    print(f"{station_name} send udp to {st}")

                    udp_msg = "GOAL&%s&%s|%s|%s|%s|%s&" % (destination, schedule[i].get("departure_time"), schedule[i].get("route_name"), station_name, schedule[i].get("arrival_time"), schedule[i].get("arrival_station"))
                    print(station_name + " sent:" + udp_msg)

                    dest_addr = (n.get("ip"), int(n.get("port")))
                    bytes_sent = udp_server_socket.sendto(udp_msg.encode('utf-8'), dest_addr)
                    print(f"Sent {bytes_sent} bytes to {dest_addr[0]}:{dest_addr[1]}")
                    break
            


# -------------------------------------------------------------------------------------------------------
if __name__ == "__main__":

    # Print error message when command line arguments are invalid
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} station-name browser-port query-port neighbour1 [neighbour2 ...]")
        sys.exit(1)

    global station_name
    global TCP_PORT
    global UDP_PORT

    # Retrieving arguments from command line
    station_name = sys.argv[1]
    TCP_PORT = int(sys.argv[2])
    UDP_PORT = int(sys.argv[3])

    print("\n-------------------------------------")
    print("Station Name:", station_name)
    print("TCP Port:", TCP_PORT)
    print("UDP Port:", UDP_PORT)

    # Process neighbours in loop for infinitely many neightbours (in reality won't have that many)
    if len(sys.argv) >= 5:
        for neighbour in sys.argv[4:]:
            if num_neighbours <= MAX_NEIGHBOURS:
                # Add to list of neighbours
                info = neighbour.split(':') # assuming correct format
                neighbours.append({
                        'name': "Unknown",
                        'ip': info[0],
                        'port': info[1]
                    })
                
                num_neighbours += 1
            else:
                print("Exceeded maximum number of neighours (maximum 20 neighbours)")
                sys.exit(1)

    else:
        print("No neighbours provided in command line. Please provide neighbours.")
    
    print("-------------------------------------")

    global tcp_server_socket
    global udp_server_socket
    global output_fds
    global message_queues
    global fds_waiting

    # Create & Bind TCP socket
    tcp_server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if tcp_server_socket.fileno() < 0:
        print("error in TCP socket creation")
        sys.exit(1)

    tcp_server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # tcp_server_socket.setblocking(False)
    tcp_server_socket.bind(("localhost", TCP_PORT))
    tcp_server_socket.listen(5)
    print(f"TCP server now listening on port {TCP_PORT}")

    # Create & Bind UDP socket
    udp_server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if udp_server_socket.fileno() < 0:
        print("error in UDP socket creation")
        tcp_server_socket.close()
        sys.exit(1)

    udp_server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # udp_server_socket.setblocking(False)
    udp_server_socket.bind(("localhost", UDP_PORT))
    print(f"UDP Server bound to port {UDP_PORT}")

    # Send station name to all neighbours 
    send_name_to_neighbours()

    # Add sockets to list for select call
    read_fds = [tcp_server_socket, udp_server_socket]

    # List of sockets to output to
    output_fds = []

    # Dictionary of queued messages for each tcp connection to send
    message_queues = {}

    # List of sockets waiting for UDP responses
    fds_waiting = []
    
    # Use select to handle both TCP and UDP sockets
    while True:

        try:
            readable, writable, exceptional = select.select(read_fds, output_fds, read_fds)
        except ValueError as e:
            print("Error with select: ", e)

            print("read fds: ")
            print(read_fds)
            print("output fds: ")
            print(output_fds)

            # Close sockets
            close_sockets()
            print("Sockets have been closed")

        # Select call populates lists with 'ready' sockets
        readable, writable, exceptional = select.select(read_fds, output_fds, read_fds)

        # For each readable socket that is 'ready'
        for sock in readable:
            # If TCP socket is readable
            if sock == tcp_server_socket:
                # Accept connection
                tcp_client_socket, tcp_client_addr = tcp_server_socket.accept()
                # print(f"TCP connection established from {tcp_client_addr}")

                # Set client socket to unblocking
                # tcp_client_socket.setblocking(0)

                # Append to read_fds
                read_fds.append(tcp_client_socket)

                # Create a message queue for this tcp connection
                message_queues[tcp_client_socket] = queue.Queue()

            # If UDP socket is readable
            elif sock == udp_server_socket:
                # Handle UDP request
                handle_UDP_request()
            
            # An already established TCP connection is ready
            else:
                # Recieve data from connection
                buffer = sock.recv(4096).decode('utf-8')
                
                if buffer:
                    print("data received")

                    # Append to output_fds
                    output_fds.append(sock)

                     # Handle request and enqueue data to the connection's message queue
                    handle_TCP_request(sock, buffer)

                else:
                    # No data received - the client has disconnected
                    read_fds.remove(sock)
                    if sock in output_fds:
                            output_fds.remove(sock)
                    # sock.shutdown(socket.SHUT_RDWR)
                    # sock.close()
                    # message_queues.pop(sock)

                
        # For each TCP connection socket ready to send 
        for sock in writable:
            # If there is a message in the queue, send it
            if not message_queues[sock].empty():

                # Get first message off the queue
                msg = message_queues[sock].get()

                if msg[:7] == "waiting":
                    for f in fds_waiting:
                        if f.get("socket") == sock:
                            # if f.get("wait_count") >= 5:
                            count = f.get("wait_count") + 1
                            f["wait_count"] = count
                    message_queues[sock].put(msg)
                    break 

                else:
                    # There is output ready to send
                    bytes_sent = sock.send(msg.encode('utf-8'))

                    if bytes_sent:
                        # Close connection 
                        read_fds.remove(sock)
                        if sock in output_fds:
                            output_fds.remove(sock)
                        sock.shutdown(socket.SHUT_RDWR)
                        sock.close()

            else:
                # TCP connection has nothing else to send so remove it from output list
                output_fds.remove(sock)

        
        # For sockets with pending error conditions
        for sock in exceptional:
            # remove and close
            read_fds.remove(sock)
            if sock in output_fds:
                output_fds.remove(sock)
            sock.shutdown(socket.SHUT_RDWR)
            sock.close()
            del message_queues[sock]

    
    # Close sockets
    tcp_server_socket.shutdown()
    tcp_server_socket.close()
    udp_server_socket.shutdown()
    udp_server_socket.close()
    print("Sockets have been closed")

