des: DES.o event.o process.o scheduler.o
	g++ -std=c++11 DES.o event.o process.o scheduler.o -o des

DES.o: DES.cpp event.h process.h scheduler.h
	g++ -std=c++11 -c DES.cpp

event.o: event.cpp event.h
	g++ -std=c++11 -c event.cpp

process.o: process.cpp process.h
	g++ -std=c++11 -c process.cpp

scheduler.o: scheduler.cpp scheduler.h
	g++ -std=c++11 -c scheduler.cpp

clean:
	rm -f des *.o
