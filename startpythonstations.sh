# python3 py-station-server.py TerminalA 2401 2408 localhost:2410 &
# python3 py-station-server.py JunctionB 2402 2409 localhost:2410 &
# python3 py-station-server.py BusportC 2403 2410 localhost:2408 localhost:2409 &

# python3 py-station-server-2.py TerminalA 2401 2408 localhost:2410 &
# # ./station-server TerminalA 2401 2408 localhost:2410 &
# python3 py-station-server-2.py JunctionB 2402 2409 localhost:2410 &
# python3 py-station-server-2.py BusportC 2403 2410 localhost:2408 localhost:2409 &

#./station-server TerminalA 2401 2408 localhost:2410 &
py station-server.py JunctionB 2402 2409 localhost:2410 &
py station-server.py BusportC 2403 2410 localhost:2408 localhost:2409 &
py station-server.py TerminalA 2401 2408 localhost:2410 &
# ./station-server JunctionB 2402 2409 localhost:2410 &
# ./station-server BusportC 2403 2410 localhost:2408 localhost:2409 &