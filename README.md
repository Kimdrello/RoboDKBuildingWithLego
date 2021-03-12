# RoboDKBuildingWithLego
This is a hobby project which is a newer, simplified and altered version of what I researched in my Master's thesis. The main idea is having a robot build structures using Lego Duplo bricks. The robot has 3 dispensers at its disposal. Each dispenser provides bricks of type 1x1, 2x2 and 2x4. Each brick is assigned their own coordinates in the application so the robot knows where to put them. Support bricks is also utilized to avoid floating bricks. For future work I will add functionality to avoid structure collapse when placing bricks on top of bricks that does not have good structural support. Research on this have already been done using machine learning so I'm planning to add something similar.

This project is a altered and edited version of the example provided at RoboDK's C++ API github page which can be found at: https://github.com/RoboDK/RoboDK-API/tree/master/C++/Example

The RoboDK C++ API files can also be found in the link above, should you have any questions regarding API specific funtions.

See files: mainwindow.cpp and duplobrick.h for the core work regarding this project.
