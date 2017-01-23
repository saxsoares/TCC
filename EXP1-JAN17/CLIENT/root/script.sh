sleep 3
ifconfig eth0 down
ifconfig eth1 up
route add default gw 192.168.11.1 dev eth1
sleep 3
ifconfig eth1 down
ifconfig eth2 up
route add default gw 192.168.12.1 dev eth2

