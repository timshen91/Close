all:
	g++ -Wall -std=c++14 -I/usr/include/OGRE -I/usr/include/OIS main.cc -lOgreMain -lOgreOverlay -lOIS -lboost_system -lasound

.PHONY : all
