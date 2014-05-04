all:
	g++ -std=c++14 -lOgreMain -lOgreOverlay -lOIS -I/usr/include/OGRE -lboost_system main.cc

.PHONY : all
