all:
	g++ -Wall -std=c++14 -lOgreMain -lOgreOverlay -lOIS -I/usr/include/OGRE -I/usr/include/OIS -lboost_system main.cc

.PHONY : all
