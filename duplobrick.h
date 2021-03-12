#ifndef DUPLOBRICK_H
#define DUPLOBRICK_H

#include <QtCore/QString>
#include <QDebug>
#include <iostream>
#include <math.h>

using namespace std;

class DuploBrick{
    private:
        int xCoord;        
        int zCoord;
        int placementPriority = 2;

        bool requiredSupport;

    protected:
        int yCoord;

        int studsXDir, studsYDir;
        int const width = 16;
        int const depth = 16;
        int brickRotation = 0;
        int const height = 19;

        QString name;
        QString displayName;
        QString dispenserName;
        QString fileLocation;

    public:

        DuploBrick(int X, int Y, int Z, bool support){
            xCoord = X;
            yCoord = Y;
            zCoord = Z;

            if(support) SetSupport();
        }

        //************************ Get ***************************
        QString Name(){
            return name;
        }

        QString DisplayName(){
            return displayName;
        }

        QString DispenserName(){
            return dispenserName;
        }

        QString FileLocation(){
            return fileLocation;
        }

        int X(){
            return xCoord;
        }

        int Y(){
            return yCoord;
        }

        int Z(){
            return zCoord;
        }

        int Priority(){
            return placementPriority;
        }

        bool RequiredSupport(){
            return requiredSupport;
        }

        int StudsXDir(){
            return studsXDir;
        }

        int StudsYDir(){
            return studsYDir;
        }

        int Width(){
            return width;
        }

        int Depth(){
            return depth;
        }

        int Height(){
            return height;
        }

        int Rotation(){
           return brickRotation;
        }
        //********************************************************

        //************************ Set ***************************
        /**
         * @brief SetPriority
         *
         * Sets the priority of the brick. The order of bricks that should be placed first
         * should be according to the brick's priority. Does not have much value in the
         * application at the moment. The priority should be calculated according to how
         * much support the brick has in the previous layers to avoid having the structure
         * collapse. Research suggests that machine learning should be used for this.
         *
         * Priority 0 means the brick cannot be placed.
         * Priority 1 means the brick has little support in the previous layer.
         * Priority 2 means the brick has full support in the previous layer.
         * Priority 3 means the brick is a support brick.
         *
         * @param priority
         * @param zCoordSupport
         */
        void SetPriority(int priority, int zCoordSupport = 0){
            switch(priority){
            case 0:
                this->placementPriority = 0;
                break;
            case 1:
                this->placementPriority = 1;
                break;
            case 2:
                if(!requiredSupport){
                    this->placementPriority = 2;
                }
                else{
                    cout << "This brick cannot be placed. Support is required." << endl;
                    this->placementPriority = 0;
                }
                break;
            case 3:
                zCoord = zCoordSupport;
                this->placementPriority = 3;
                break;
            }
        }

        /**
         * @brief SetSupport
         *
         * Sets the current brick to require support from previous layers (if the z coordinate > 0)
         *
         */
        void SetSupport(){
            if(zCoord > 0){
                requiredSupport = true;
            }
            requiredSupport = false;
        }
        //********************************************************

        //Sorting
        bool operator <(const DuploBrick & brickObj) const
        {
            return zCoord < brickObj.zCoord;
        }
};

class Brick1x1 : public DuploBrick{
    using DuploBrick::DuploBrick;

    public:

        Brick1x1(int X, int Y, int Z, bool support) : DuploBrick{X, Y, Z, support}{
            name = "duplo_brick_1x1";
            studsXDir = 1;
            studsYDir = 1;
            dispenserName = "Dispenser1x1";
            fileLocation = "C:/RoboDK/Library/duplo_brick_1x1.stl";
        }

};

class Brick2x2 : public DuploBrick{
    using DuploBrick::DuploBrick;

    public:

        Brick2x2(int X, int Y, int Z, bool support) : DuploBrick{X, Y, Z, support}{
            name = "duplo_brick_2x2";
            studsXDir = 2;
            studsYDir = 2;
            dispenserName = "Dispenser2x2";
            fileLocation = "C:/RoboDK/Library/duplo_brick_2x2.stl";
        }
};

class Brick2x4 : public DuploBrick{
    using DuploBrick::DuploBrick;

    private:
        int yCoordCopy;

    public:

        Brick2x4(int X, int Y, int Z, bool support) : DuploBrick{X, Y, Z, support}{
            name = "duplo_brick_2x4";
            studsXDir = 4;
            studsYDir = 2;
            yCoordCopy = Y;
            dispenserName = "Dispenser2x4";
            fileLocation = "C:/RoboDK/Library/duplo_brick_2x4.stl";
        }

        //************************ Set ***************************

        /**
         * @brief SetRotation
         *
         * Sets the rotation of the brick, has to be either 0 or 90 degress (for now)
         *
         * @param rotation
         */
        void SetRotation(int rotation){
            switch (rotation) {
            case 90:
                studsXDir = 2;
                studsYDir = 4;
                yCoord = yCoord + 1;
                brickRotation = rotation;
                break;
            case 0:
                studsXDir = 4;
                studsYDir = 2;
                yCoord = yCoordCopy;
                brickRotation = rotation;
                break;
            default:
                cout << "Invalid rotation" << endl;
                break;
            }
        }
        //********************************************************
};

#endif // DUPLOBRICK_H
