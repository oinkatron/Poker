all : server client	

server : server.o list.o util.o channel.o poker.o
	cc -g -o server server.o list.o util.o channel.o poker.o

client : client.o util.o list.o tb_ui.o channel.o
	cc -g -o client client.o util.o list.o tb_ui.o channel.o -lcrypto -ltermbox

poker.o : poker.h poker.c
	cc -g -c poker.c

server.o  : server.c
	cc -c server.c

client.o : client.c
	cc -c client.c

packet.o : packet.c packet.h
	cc -c packet.c

util.o : util.c util.h
	cc -c util.c

list.o : list.c list.h
	cc -c list.c

udp.o : udp.c udp.h
	cc -c udp.c

tb_ui.o : tb_ui.h tb_ui.c
	cc -c -g tb_ui.c

channel.o: channel.h channel.c
	cc -c -g channel.c
