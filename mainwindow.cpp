#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "embedexample.h"
#include <QFileDialog>
#include <QWindow>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include "robomove.h"
#include "timer.h"
#include <iostream>
#include <stdint.h>
#include <cstdint>
#include <cstddef>
#include <typeinfo>
#include<fstream>
#include<sstream>
#include<string>

using namespace std;
using namespace RoboDK_API;

#ifdef WIN32
// this is used to integrate RoboDK window as a child window
#include <windows.h>
#pragma comment(lib,"user32.lib")
#endif

//Class for creating the UI Grid of radio buttons to create patterns. Done by assigning each radio button an x and y ID which serves as coordinates.
class AppRadButton{

public:
    QRadioButton *rbtn;
    int xID;
    int yID;
};

// Coord Grid for Scenarios
bool buildCoords[50][50][50];
bool supportCoords[50][50][50];
bool running;
std::thread *mainThread;
QFuture<void> updateDispenserThread;

list<Item> TwoByTwoBlockDispenser;
list<Item> TwoByFourBlockDispenser;
list<Item> OneByOneBlockDispenser;
list<DuploBrick> bricklist;

list<DuploBrick> supportbricklist;
bool EmergencyStopTriggered;
int z = 0;
int notPlaceAbleCounter;
QGridLayout *grid1;
QGroupBox *qbox;
QString title;
QFuture<void> simulateDispenser;

list<AppRadButton> radBtnList;


#define M_PI 3.14159265358979323846

///************************************** RoboDK Code **************************************
///The following functions is created by RoboDK and is a part of the RoboDK-API-Cpp-Sample.
///
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    robodk_window = NULL;
    ui->setupUi(this);
    ui->widgetRoboDK->hide();
    adjustSize();

    //Own edit. The UI grid for creating patterns are generated when the UI is created.
    setUpUIGrid();

    // Start RoboDK API here (RoboDK will start if it is not running)
    ROBOT = NULL;
    RDK = new RoboDK();
    if (!RDK->Connected()){
        qDebug() << "Failed to start RoboDK API!!";
    }

}

MainWindow::~MainWindow() {
    robodk_window_clear();
    RDK->CloseRoboDK();
    delete ui;
    delete RDK;
}

bool MainWindow::Check_RoboDK(){
    if (RDK == NULL){
        statusBar()->showMessage("RoboDK API is not connected");
        return false;
    }
    if (!RDK->Connected()){
        statusBar()->showMessage("RoboDK is not running");
        return false;
    }
    return true;
}

bool MainWindow::Check_Robot(){
    if (!Check_RoboDK()){ return false; }

    if (ROBOT == NULL){
        statusBar()->showMessage("Select a robot first");
        return false;
    }
    if (!ROBOT->Valid()){
        statusBar()->showMessage("Robot item is not valid");
        return false;
    }
    return true;
}

void MainWindow::Select_Robot(){
    if (ROBOT != NULL){
        delete ROBOT;
        ROBOT = NULL;
    }
    ROBOT = new Item(RDK->ItemUserPick("Select a robot", RoboDK::ITEM_TYPE_ROBOT));
    //ROBOT = new Item(RDK->getItem("UR10", RoboDK::ITEM_TYPE_ROBOT));
    if (Check_Robot()){
        statusBar()->showMessage("Robot selected: " + ROBOT->Name());
    }
}

void MainWindow::on_btnLoadFile_clicked() {
    if (!Check_RoboDK()){ return; }

    QStringList files = QFileDialog::getOpenFileNames(this, tr("Open one or more files with RoboDK"));
    foreach (QString file, files){
        qDebug() << "Loading: " << file;
        RDK->AddFile(file);
    }

    if (!Check_Robot()){
        Select_Robot();
    }
}

void MainWindow::on_btnSelectRobot_clicked(){
    Select_Robot();
}


void MainWindow::on_btnGetPosition_clicked(){
    if (!Check_Robot()){ return; }

    QString separator = " , ";
    int decimals = 1;

    // Get robot joints
    tJoints joints(ROBOT->Joints());
    QString joints_str = joints.ToString(separator, decimals);
    ui->txtJoints->setText(joints_str);

    // Get robot pose
    Mat robot_pose(ROBOT->Pose());
    QString pose_str = robot_pose.ToString(separator, decimals);
    ui->txtXYZWPR->setText(pose_str);
}

void MainWindow::on_btnMoveJoints_clicked(){
    if (!Check_Robot()){ return; }

    tJoints joints;

    joints.FromString(ui->txtJoints->text());

    bool blocking = true;

    ROBOT->MoveJ(joints, blocking);

}

void MainWindow::on_btnMovePose_clicked(){
    if (!Check_Robot()){ return; }

    Mat pose;

    pose.FromString(ui->txtXYZWPR->text());

    bool blocking = true;

    ROBOT->MoveJ(pose, blocking);

}
///************************************ RoboDK Code End ************************************
///*****************************************************************************************

/**
 * @brief MainWindow::setUpUIGrid
 * Generates a set of radio buttons representing a 2 dimensional grid
 * where the user can toggle the buttons to create a pattern which
 * the robot will build.
 */
void MainWindow::setUpUIGrid(){
    grid1 = new QGridLayout;
    qbox = new QGroupBox;
    QString msg;

    for(int x = 0; x < 13; x++){
        for(int y = 0; y < 13; y++){
            buildCoords[z][x][y] = false;
            msg.append("X: ");
            msg.append(x);
            msg.append("Y: ");
            msg.append(y);

            QRadioButton *radioBtn = new QRadioButton(tr(""));
            radioBtn->setCheckable(true);
            radioBtn->setAutoExclusive(false);

            AppRadButton *rBtn = new AppRadButton();
            rBtn->rbtn = radioBtn;
            rBtn->xID = x;
            rBtn->yID = y;

            grid1->addWidget(rBtn->rbtn, rBtn->xID, rBtn->yID = y);
            radBtnList.push_back(*rBtn);
        }
    }
    qbox->setLayout(grid1);
    title = "Layer: ";
    title.append(QString::number(z));
    qbox->setTitle(title);
    ui->verticalLayout_14->addWidget(qbox);
}

/**
 * @brief MainWindow::ListComparison
 *
 * Compares the priority of two DuploBricks.
 *
 * @param first
 * @param second
 * @return
 */
bool MainWindow::ListComparison(DuploBrick first, DuploBrick second){
    return(first.Priority() < second.Priority());
}

/**
 * @brief MainWindow::on_btnOneCoordOneBrick_clicked
 *
 * When clicked, each coordinate is treated according to a 2x2 brick.
 * This is currently not in use.
 *
 */
void MainWindow::on_btnOneCoordOneBrick_clicked(){

}

/**
 * @brief MainWindow::on_btnOneCoordOneStud_clicked
 *
 * When clicked, each coordinate is treated according to the studs of
 * each brick and the base plate.
 *
 * The algorithm counts the studs of each layer which in this case
 * is represented by a bool value = true in the buildCoords array.
 * The algorithm will always try to place a 2x4 brick first, then
 * 2x2, and then 1x1 if none of the other bricks can be placed. It
 * will also check if a 2x4 brick can indeed be place by rotating
 * it 90 degrees.
 *
 * To avoid floating bricks, the function AddSupportBricks is
 * ran concurrently after the algorithm has checked what brick
 * to place at the current coordinates.
 *
 */
void MainWindow::on_btnOneCoordOneStud_clicked(){
    QFuture<void> addSupportBricksThread;

    if(bricklist.empty()){

        cout << "Function: on_btnOneCoordOneStud_clicked, Message: Loaded: One Coord One Stud" << endl;

        for(int z = 0; z < 50; z++){
            for(int x = 0; x < 50; x++){
                for(int y = 0; y < 50; y++){
                    if(buildCoords[z][x][y]){
                        Brick2x2 *temp2x2 = new Brick2x2(x, y, z, false);
                        Brick2x4 *temp2x4 = new Brick2x4(x, y, z, false);
                        Brick1x1 *temp1x1 = new Brick1x1(x, y, z, false);

                        DuploBrick *brick = new DuploBrick(x, y, z, false);
                        DuploBrick *supportBrick = new DuploBrick(x, y, z, false);

                        bool canPlace2x2 = (CheckIfPlaceable(temp2x2->StudsXDir(), temp2x2->StudsYDir(), buildCoords, x, y, z) == 4);
                        bool canPlace2x4 = (CheckIfPlaceable(temp2x4->StudsXDir(), temp2x4->StudsYDir(), buildCoords, x, y, z) == 8);

                        if(!canPlace2x4){
                            temp2x4->SetRotation(90);
                            cout << "Function: on_btnOneCoordOneStud_clicked, Message: Trying to rotate brick." << endl;
                            canPlace2x4 = (CheckIfPlaceable(temp2x4->StudsXDir(), temp2x4->StudsYDir(), buildCoords, x, y, z) == 8);
                        }      

                        if(canPlace2x4){
                            brick = temp2x4;

                            Brick2x4 *support2x4 = new Brick2x4(x, y, z, false);
                            support2x4->SetRotation(temp2x4->Rotation());
                            supportBrick = support2x4;
                            cout << "Function: on_btnOneCoordOneStud_clicked, Message: Adding 2x4 Brick to list." << endl;
                        }
                        else if(canPlace2x2){
                            brick = temp2x2;

                            Brick2x2 *support2x2 = new Brick2x2(x, y, z, false);
                            supportBrick = support2x2;
                            cout << "Function: on_btnOneCoordOneStud_clicked, Message: Adding 2x2 Brick to list." << endl;
                        }
                        else{
                            brick = temp1x1;

                            Brick1x1 *support1x1 = new Brick1x1(x, y, z, false);
                            supportBrick = support1x1;
                            cout << "Function: on_btnOneCoordOneStud_clicked, Message: Adding 1x1 Brick to list." << endl;
                        }

                        if(z > 0){
                            addSupportBricksThread = QtConcurrent::run(this, &MainWindow::AddSupportBricks, supportBrick, x, y, z);
                            brick->SetSupport();
                        }

                        for(int i = 0; i < brick->StudsXDir(); i++){
                            for(int j = 0; j < brick->StudsYDir(); j++){
                                buildCoords[z][x+i][y+j] = false;
                                supportCoords[z][x+i][y+j] = true;
                            }
                        }

                        if(addSupportBricksThread.isRunning()){
                            addSupportBricksThread.waitForFinished();
                        }

                        bricklist.push_back(*brick);
                    }
                }
            }
        }
        bricklist.splice(bricklist.end(), supportbricklist);
        bricklist.sort();
        cout << "Function: on_btnOneCoordOneStud_clicked, Message: Sorting done" << endl;
    }
}

/**
 * @brief MainWindow::AddSupportBricks
 *
 * If the current layer is > 0, this function will check the previous layers to see
 * if there are studs in which a brick can be placed upon. This is to avoid
 * floating bricks.
 *
 * @param brick is the current brick that will be added to the bricklist. The z
 *        coordinate of this brick is > 0 which means we have to check if there
 *        are studs in the previous layer for it to be placed upon.
 * @param x is the x coordinate of the brick
 * @param y is the y coordinate of the brick
 * @param z is the z coordinate of the brick
 */
void MainWindow::AddSupportBricks(DuploBrick *brick, int x, int y, int z){
    int countStudsForChildLayer = CheckIfPlaceable(brick->StudsXDir(), brick->StudsYDir(), supportCoords, x, y, z - 1);
    cout << "Function: AddSupportBricks, Message: Studs in Childlayer: " << countStudsForChildLayer << endl;
    if(countStudsForChildLayer == 0){
        int childLayerCounter = 1;
        while(childLayerCounter <= z){
            cout << "Function: AddSupportBricks, Message: Inside support whileloop" << endl;
            int countStudsForChildLayers = CheckIfPlaceable(brick->StudsXDir(), brick->StudsYDir(), supportCoords, x, y, childLayerCounter - 1);

            if(countStudsForChildLayers == 0 /*|| countStudsForChildLayers >= maxSupport*/){
                DuploBrick *supportBlock = new DuploBrick(0, 0, 0, false);
                supportBlock = brick;
                supportBlock->SetPriority(3, childLayerCounter - 1);

                for(int i = 0; i < brick->StudsXDir(); i++){
                    for(int j = 0; j < brick->StudsYDir(); j++){
                        supportCoords[childLayerCounter - 1][x+i][y+j] = true;
                    }
                }
                cout << "Function: AddSupportBricks, Message: Support brick type: " << supportBlock->Name().toStdString() << endl;
                cout << "Function: AddSupportBricks, Message: Support brick prio: " << supportBlock->Priority() << endl;
                supportbricklist.push_back(*supportBlock);
            }
            childLayerCounter++;
        }
    }
}

/**
 * @brief MainWindow::on_btnProgRun_clicked
 *
 * When clicked it will start a new thread. At the start of the thread,
 * the function BuildStructure is called.
 *
 * Before the thread is initiated, the application will first check
 * if a robot is active in the simulation. Then it will check if
 * the bricklist is empty.
 *
 */
void MainWindow::on_btnProgRun_clicked(){
    if (!Check_Robot()){ return; }

    if(bricklist.empty()){
        cout << "Function: on_btnProgRun_clicked, Message: List is empty!" << endl;
        return;
    }

    //*************************Direct method*******************************************************************

    mainThread = new std::thread(&MainWindow::BuildStructure, this);

    //*********************************************************************************************************
}

/**
 * @brief MainWindow::on_btnThingMLRun_clicked
 *
 * When clicked it will run a generated code which has been transformed
 * into C++ code from ThingML code. This was purely for research purposes
 * on my master's thesis and is not in use anymore.
 *
 */
void MainWindow::on_btnThingMLRun_clicked(){
    cout << "btnThingMLRun clicked" << endl;

    if (!Check_Robot()){ return; }

    if(bricklist.empty()){ return; }

    clock_t start;
    double duration;

    start = clock();

    //*************************Modelling method, Runs the program via generated code from ThingML (Not in use anymore as of 01.01.2021)*************************

    //TASK buildWithThingML;
    //buildWithThingML.Robobuild(RDK, TwoByTwoBlockDispenser, TwoByFourBlockDispenser, bricklist);

    //**********************************************************************************************************************************************************

    duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;

    cout<<"Function: on_btnThingMLRun_clicked, Message: duration of build was "<< duration <<'\n';
}

/**
 * @brief MainWindow::BuildStructure
 *
 * The main function that handles the building process. When we a run a new thread in
 * RoboDK we have to initalize a new instance of RoboDK which is done at the start
 * here. Since there are functions that interacts with the simulation outside of
 * this function we also need to set a global RoboDK and Item (Robot) variable to
 * point to the instance and robot ran inside this function.
 *
 * BuildStructure uses an iterator to go through the brick list and each dispenser
 * is updated when the function pickUpBrick_and_Move is called which is the process
 * of the robot moving over to a dispenser, picking out a brick and then placing it
 * at its correct spot.
 *
 */
void MainWindow::BuildStructure(){
    clock_t start;
    double duration;

    start = clock();

    cout << "Function: BuildStructure, Message: Build initialized" << endl;
    RoboDK *rdk = new RoboDK;
    RDK = rdk;
    Item *robot = new Item(rdk->getItem("UR10", RoboDK::ITEM_TYPE_ROBOT));
    ROBOT = robot;

    Mat pose_ref = robot->Pose();
    robot->setSpeed(100);

    Item *table_frame = new Item(rdk->getItem("Table Base", RoboDK::ITEM_TYPE_FRAME));
    Item *tool = new Item(rdk->getItem("Gripper RobotiQ 85", RoboDK::ITEM_TYPE_TOOL));
    Item *work_frame = new Item(rdk->getItem("Work Frame", RoboDK::ITEM_TYPE_FRAME));

    work_frame->setParent(*table_frame);
    work_frame->setPose(transl(0, 0, 0));
    robot->setPoseAbs(work_frame);
    robot->setGeometryPose(table_frame->Pose());

    double rgbColor[4] = {0.7, 0.0, 0.51, 1};

    list<DuploBrick>::iterator it;

    for (it = bricklist.begin(); it != bricklist.end(); it++){
        if(checkIfdispenserIsEmpty(robot) || EmergencyStopTriggered) break;

        ui->statusBar->showMessage("Dispenser status ok.");

        if(it->Priority() > 0){

            cout << "Function: BuildStructure, Message: The priority of next brick is: " << it->Priority() << endl;

            if(it->Name() == "duplo_brick_2x2"){
                Brick2x2 *temp2x2 = new Brick2x2(it->X(), it->Y(), it->Z(), it->RequiredSupport());

                if(it->Priority() == 3){
                    TwoByTwoBlockDispenser.front().setName("Support brick");
                    TwoByTwoBlockDispenser.front().setColor(rgbColor);
                }               
                cout << "Function: BuildStructure, Message: The brick type is 2x2." << endl;

                TwoByTwoBlockDispenser = pickUpBrick_and_Move(rdk, robot, table_frame, work_frame, tool, TwoByTwoBlockDispenser, temp2x2);
            }
            else if(it->Name() == "duplo_brick_2x4"){

                Brick2x4 *temp2x4 = new Brick2x4(it->X(), it->Y(), it->Z(), it->RequiredSupport());

                if(it->Rotation() > 0) temp2x4->SetRotation(it->Rotation());
                if(it->Priority() == 3){
                    TwoByFourBlockDispenser.front().setName("Support brick");
                    TwoByFourBlockDispenser.front().setColor(rgbColor);
                }                
                cout << "Function: BuildStructure, Message: The brick type is 2x4." << endl;

                TwoByFourBlockDispenser = pickUpBrick_and_Move(rdk, robot, table_frame, work_frame, tool, TwoByFourBlockDispenser, temp2x4);
            }
            else if(it->Name() == "duplo_brick_1x1"){
                Brick1x1 *temp1x1 = new Brick1x1(it->X(), it->Y(), it->Z(), it->RequiredSupport());

                if(it->Priority() == 3){
                    OneByOneBlockDispenser.front().setName("Support brick");
                    OneByOneBlockDispenser.front().setColor(rgbColor);
                }                
                cout << "Function: BuildStructure, Message: The brick type is 1x1." << endl;

                OneByOneBlockDispenser = pickUpBrick_and_Move(rdk, robot, table_frame, work_frame, tool, OneByOneBlockDispenser, temp1x1);
            }
        }
        if(it->Priority() == 0) notPlaceAbleCounter++;
        if(!EmergencyStopTriggered) bricklist.pop_front();
    }    
    duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;

    cout<<"Function: BuildStructure, Message: duration of build was "<< duration << endl;
}

/**
 * @brief MainWindow::pickUpBrick_and_Move
 *
 * This function simulates the process of the robot moving to a dispenser to pick out a brick,
 * moving over to the base plate or structure, and placing the brick on the correct spot.
 *
 * To simulate the dispensers when a brick is dragged out of the stack, the function
 * UpdateDispenser is called and ran concurrently when the robot has picked out a brick.
 *
 * @param rdk is the RoboDK instance.
 * @param robot is the active robot.
 * @param parent_frame is a 3 dimensional reference frame of the table. The robot uses
 *        coordinates inside this reference frame to place bricks on the base plate.
 * @param work_frame is a 3 dimensional reference frame. This is the main reference
 *        frame that points the robot in the right direction.
 * @param tool is the tool attached to the robot's head.
 * @param dispenser is the current dispenser.
 * @param brick is the current brick.
 * @return
 */
list<Item> MainWindow::pickUpBrick_and_Move(RoboDK *rdk, Item *robot, Item *parent_frame, Item *work_frame, Item *tool, list<Item> dispenser, DuploBrick *brick){
    //This is for One stud = One coord
    //int xPlace = 624;

    //For placing the bricks correctly according to the baseplate in the robot's workspace.
    int baseX = (brick->X() * brick->Depth()) + 432;
    int baseY = (brick->Y() * brick->Width()) + 354;

    //When there is multiple layers, the correct height placement of the bricks is the z coordinate * the height (19).
    int layerPlacement = brick->Z() * brick->Height();

    //Get Brick
    int dispSize = dispenser.size();
    cout << "Function: pickUpBrick_and_Move, Message: Size of current dispenser is " << dispSize << endl;

    //For placing the bricks correctly on the studs. Measurement is in mm.
    double studX = 8.9;
    double studY = 6.5;

    //When rotating the brick, the placement has to be slightly altered
    if(brick->Rotation() == 90){
        studX = studX + 0.85;
        studY = studY + 2.5;
    }

    Item *dispenser_frame = new Item(rdk->getItem(brick->DispenserName(), RoboDK::ITEM_TYPE_FRAME));
    work_frame->setParent(*dispenser_frame);
    work_frame->setPose(transl(0, 0, 0));
    robot->MoveL(transl(0, 0, -50));
    robot->MoveL(transl(0, 0, -30));
    tool->AttachClosest();
    dispenser.pop_front();
    if(dispSize != 0){ dispenser.resize(dispSize - 1); }
    cout << dispenser.size() << endl;
    robot->MoveL(transl(0, 0, -50));

    //parent_frame->Pose().rotate(rotation, 1.0, 0.0, 0.0);

    simulateDispenser = QtConcurrent::run(this, &MainWindow::UpdateDispenser, dispenser, brick->Name());

    work_frame->setParent(*parent_frame);
    work_frame->setPose(transl(0, 0, 0));

    robot->MoveL(transl(baseX + studX, baseY - studY, -100 - layerPlacement));

    Mat pose_ref = robot->Pose();
    pose_ref.rotate(brick->Rotation(), 0, 0, 1.0);
    cout << "Function: pickUpBrick_and_Move, Message: Rotation of brick is: " << brick->Rotation() << endl;
    robot->MoveL(pose_ref);

    pose_ref.setPos(baseX + studX, baseY - studY, -31.1 - layerPlacement);
    robot->MoveL(pose_ref);

    tool->DetachAll(*parent_frame);
    robot->MoveL(transl(baseX + studX, baseY - studY, -100 - layerPlacement));

    if(!simulateDispenser.isFinished()){
        simulateDispenser.waitForFinished();
    }

    return dispenser;
}

/**
 * @brief MainWindow::UpdateDispenser
 *
 * Simulates the scenario where a brick is dragged out from the bottom of a stack of bricks.
 *
 * @param dispenser is the dispenser
 * @param brickName is the name of the brick
 */
void MainWindow::UpdateDispenser(list<Item> dispenser, QString brickName){
    if(!dispenser.empty()){
        list<Item>::iterator it;
        int counter = 0;
        for (it = dispenser.begin(); it != dispenser.end(); it++){
            if(brickName == "duplo_brick_2x2" || brickName == "duplo_brick_1x1"){
                Mat pose_box = it->Pose();
                pose_box.setPos(0, counter*32 + 1, 0);
                it->setPose(pose_box);
            }
            else{
                Mat pose_box = it->Pose();
                pose_box.setPos(-80, counter*32 + 9.250, 0.1);
                it->setPose(pose_box);
            }
            counter++;
        }
    }
}

/**
 * @brief MainWindow::checkIfdispenserIsEmpty
 *
 * Checks if either of the dispensers are empty. Returns true if either of the
 * dispensers are empty and stops the robot if it is active.
 *
 * @param robot is the active robot.
 * @return
 */
bool MainWindow::checkIfdispenserIsEmpty(Item *robot){
    bool TwoByTwoListEmpty = TwoByTwoBlockDispenser.empty();
    bool TwoByFourListEmpty = TwoByFourBlockDispenser.empty();
    bool OneByOneListEmpty = OneByOneBlockDispenser.empty();

    if(TwoByTwoListEmpty){
        robot->Stop();
        ui->statusBar->showMessage("Please fill up 2x2 dispenser.");
        cout << "Function: checkIfdispenserIsEmpty, Message: Please fill up 2x2 dispenser." << endl;
    }
    else if(TwoByFourListEmpty){
        robot->Stop();
        ui->statusBar->showMessage("Please fill up 2x4 dispenser.");
        cout << "Function: checkIfdispenserIsEmpty, Message: Please fill up 2x4 dispenser." << endl;
    }
    else if(OneByOneListEmpty){
        robot->Stop();
        ui->statusBar->showMessage("Please fill up 1x1 dispenser.");
        cout << "Function: checkIfdispenserIsEmpty, Message: Please fill up 1x1 dispenser." << endl;
    }
    return TwoByTwoListEmpty || TwoByFourListEmpty || OneByOneListEmpty;
}

/**
 * @brief MainWindow::CheckIfPlaceable
 *
 * Function that checks if a brick is placeable. This is done by counting coordinates that are true
 * in the 3 dimensional array. A true value tells us that a brick is to be placed here. The
 * function counts the studs (value true) in x and y direction. If the total count returns 4 for
 * example, it means that a 2x2 brick can be placed here.
 *
 * @param studsXdir is how many studs there is in x direction of the brick that is checked if placeable.
 * @param studsYdir is how many studs there is in y direction of the brick that is checked if placeable.
 * @param arr is the array of boolean values where the value true means a brick is to be placed here.
 * @param x is the x coordinate.
 * @param y is the y coordinate.
 * @param z is the z coordinate.
 * @return
 */
int MainWindow::CheckIfPlaceable(int studsXdir, int studsYdir, bool arr[50][50][50], int x, int y, int z){
    int xCounter = 0;
    int studCounter = 0;

    while(xCounter < studsXdir){
        int yCounter = 0;
        while(yCounter < studsYdir){
            if(arr[z][x+xCounter][y+yCounter]){
                studCounter++;
            }
            yCounter++;
        }
        xCounter++;
    }
    return studCounter;
}

/**
 * @brief MainWindow::on_btnAddLayer_clicked
 *
 * When clicked, sets up the coordinate array according to the toggled radio buttons.
 * Starts at z = 0 and adds z by 1 for each time the button is clicked.
 *
 */
void MainWindow::on_btnAddLayer_clicked(){
    list<AppRadButton>::iterator it;

    for (it = radBtnList.begin(); it != radBtnList.end(); it++){
        if(it->rbtn->isChecked()){
            //supportCoords[z][it->xID][it->yID] = true;
            buildCoords[z][it->xID][it->yID] = true;
            it->rbtn->setChecked(false);
        }
        else{
            supportCoords[z][it->xID][it->yID] = false;
            buildCoords[z][it->xID][it->yID] = false;
        }
    }
    z++;

    title = "Layer: ";
    title.append(QString::number(z));
    qbox->setTitle(title);
}

/**
 * @brief MainWindow::on_load3DModelbtn_clicked
 *
 * Function that reads a text file to set up the coordinate array. The txt file
 * contains coordinates of voxels of a 3D model that has been voxelized.
 *
 */
void MainWindow::on_load3DModelbtn_clicked(){
    int bricks = 0;
    z = 0;

    string readX, readY, readZ;
    int xCoord, yCoord, zCoord;

    if(bricklist.empty()){
        notPlaceAbleCounter = 0;

        ifstream fileStream("E:/Users/kimandre/Documents/3dVoxelModels/_3DBenchy.txt");

        if(!fileStream.is_open()){
            cout<<"Function: on_load3DModelbtn_clicked, Message: Error when opening file"<<endl;
        }
        else{
            string line;

            while(getline(fileStream, line)){
                stringstream ss(line);
                getline(ss, readX, ',');
                xCoord = stoi(readX);

                getline(ss, readY, ',');
                yCoord = stoi(readY);

                getline(ss, readZ, ',');
                zCoord = stoi(readZ);

                if(yCoord > z){
                    cout << "Function: on_load3DModelbtn_clicked, Message: Current Layer: " << z << endl;
                    z = yCoord;
                }
                buildCoords[yCoord][xCoord][zCoord] = true;
                supportCoords[yCoord][xCoord][zCoord] = false;
                bricks++;
            }
            fileStream.close();
        }
        cout<<"Function: on_load3DModelbtn_clicked, Message: Loading 3D model done."<<endl;

        title = "Layer: ";
        title.append(QString::number(z));
        title.append(", Bricks: ");
        title.append(QString::number((bricks)));
        qbox->setTitle(title);
    }
}

/**
 * @brief MainWindow::on_resetLayerbtn_clicked
 *
 * When clicked, clears all toggled radio buttons.
 *
 */
void MainWindow::on_resetLayerbtn_clicked(){
    list<AppRadButton>::iterator it;

    for (it = radBtnList.begin(); it != radBtnList.end(); it++){
        if(it->rbtn->isChecked()){
            it->rbtn->setChecked(false);
        }
    }
}

/**
 * @brief MainWindow::on_fill4x2feederbtn_clicked
 *
 * When clicked, fills the 2x4 brick dispenser.
 *
 */
void MainWindow::on_fill4x2feederbtn_clicked(){
    double colorBlue[4] = {0.9, 0.9, 0.0, 1.0};
    Brick2x4 *temp = new Brick2x4(0, 0, 0, false);
    if(TwoByFourBlockDispenser.empty()){ TwoByFourBlockDispenser = fillDispenser(temp, colorBlue); }
}

/**
 * @brief MainWindow::on_fill2x2feederbtn_clicked
 *
 * When clicked, fills the 2x2 brick dispenser.
 *
 */
void MainWindow::on_fill2x2feederbtn_clicked(){
    double colorRed[4] = {0.0, 0.9, 0.9, 1.0};
    Brick2x2 *temp = new Brick2x2(0, 0, 0, false);
    if(TwoByTwoBlockDispenser.empty()){ TwoByTwoBlockDispenser = fillDispenser(temp, colorRed); }
}

/**
 * @brief MainWindow::on_fill1x1feederbtn_clicked
 *
 * When clicked, fills the 1x1 brick dispenser.
 *
 */
void MainWindow::on_fill1x1feederbtn_clicked(){
    double colorWhite[4] = {0.9, 0.9, 0.9, 1.0};
    Brick1x1 *temp = new Brick1x1(0, 0, 0, false);
    if(OneByOneBlockDispenser.empty()){ OneByOneBlockDispenser = fillDispenser(temp, colorWhite); }
}

/**
 * @brief MainWindow::fillDispenser
 *
 * Function that fills a brick dispenser up when empty.
 *
 * @param brick is the brick type.
 * @param RGB is the color of the brick.
 * @return
 */
list<Item> MainWindow::fillDispenser(DuploBrick *brick, double RGB[4]){
    list<Item> dispenserList;

    for(int i = 0; i < 14; i++){
        Item *dispenser_frame = new Item(RDK->getItem(brick->DispenserName(), RoboDK::ITEM_TYPE_FRAME));
        RDK->AddFile(brick->FileLocation(), dispenser_frame);
        Item *box = new Item(RDK->getItem(brick->Name(), RoboDK::ITEM_TYPE_OBJECT));
        box->setColor(RGB);
        if(brick->Name() == "duplo_brick_2x2" || brick->Name() == "duplo_brick_1x1"){
            Mat pose_box = box->Pose();
            pose_box.setPos(0, i*32 + 1, 0);
            pose_box.rotate(-180, 1.0, 0, 0);
            box->setPose(pose_box);
        }
        else{
            Mat pose_box = box->Pose();
            pose_box.setPos(-80, i*32 + 9.250, 0.1);
            pose_box.rotate(-90, 0, 0, 1.0);
            pose_box.rotate(-90, 1.0, 0, 0);
            box->setPose(pose_box);
        }
        box->setName("Build brick");
        dispenserList.push_back(*box);
    }
    return dispenserList;
}

/**
 * @brief MainWindow::on_dropBrickbtn_clicked
 *
 * When clicked, removes the brick from the robot's tool. Simulates a scenario where the robot
 * drops a brick.
 *
 */
void  MainWindow::on_dropBrickbtn_clicked(){
    RoboDK *rdk = new RoboDK;
    Item *tool = new Item(rdk->getItem("Gripper RobotiQ 85", RoboDK::ITEM_TYPE_TOOL));
    QList<Item> children;
    children = tool->Childs();
    if(!children.empty()){
        ROBOT->Stop();
        mainThread->detach();
        tool->Childs().back().Delete();
    }
    if(!simulateDispenser.isFinished()){
        simulateDispenser.waitForFinished();
    }
    else if(!TwoByTwoBlockDispenser.empty() || !TwoByFourBlockDispenser.empty()){
        mainThread = new std::thread(&MainWindow::BuildStructure, this);
    }
}

/**
 * @brief MainWindow::on_humanInterferencebtn_clicked
 *
 * When clicked, generates an arm model in the robot's workspace.
 *
 */
void MainWindow::on_humanInterferencebtn_clicked(){
     std::thread *insertArmThread = new std::thread(&MainWindow::EmergencyTriggered, this);
     insertArmThread->join();
}

/**
 * @brief MainWindow::on_releaseEmergenyStopbtn_clicked
 *
 * When clicked, any emergency stop triggers are cleared and reset.
 *
 */
void MainWindow::on_releaseEmergenyStopbtn_clicked(){
    RoboDK *rdk = new RoboDK;
    Item *frame = new Item(rdk->getItem("Person", RoboDK::ITEM_TYPE_FRAME));
    QList<Item> children;
    children = frame->Childs();
    if(!children.empty()){
        Item *arm = new Item(rdk->getItem("11535_arm_V3_", RoboDK::ITEM_TYPE_OBJECT));
        arm->Delete();
    }
    EmergencyStopTriggered = false;
}

/**
 * @brief MainWindow::on_emergencyStopbtn_clicked
 *
 * A button that stops the robot, acts as a emergency stop button.
 *
 * Note: Stopping the thread may cause unwanted behavior in the simulation.
 *
 */
void MainWindow::on_emergencyStopbtn_clicked(){
    ROBOT->Stop();
    mainThread->detach();
    EmergencyStopTriggered = true;
}

/**
 * @brief MainWindow::EmergencyTriggered
 *
 * Function that simulates the scenario where a person sticks their arm into the robot's
 * workspace. In a real life situation a emergency stop will trigger when this happens
 * which is are supposed to be simulated here.
 *
 * Note: Stopping the thread may cause unwanted behavior in the simulation.
 *
 */
void MainWindow::EmergencyTriggered(){
    RoboDK *rdk = new RoboDK;
    Item *frame = new Item(rdk->getItem("Person", RoboDK::ITEM_TYPE_FRAME));
    rdk->AddFile("C:/RoboDK/Library/11535_arm_V3_.obj", frame);
    Item *arm = new Item(rdk->getItem("11535_arm_V3_", RoboDK::ITEM_TYPE_OBJECT));
    arm->Scale(8.00000);
    ROBOT->Stop();
    mainThread->detach();
    EmergencyStopTriggered = true;
}

///************************************** RoboDK Code **************************************
///The following functions is created by RoboDK and is a part of the RoboDK-API-Cpp-Sample.
///
// Example to run a second instance of the RoboDK api in parallel:
// make sure to include #include <thread>
//std::thread *t1 = new std::thread(blocking_task);
/*
void blocking_task(){
    RoboDK rdk; // important! It will not block the main thread (blocking or non blocking won'T make a difference)

    // show the blocking popup:
    rdk.Popup_ISO9283_CubeProgram();

}
*/

// then, start the thread and let it finish once the user finishes with the popup
void MainWindow::on_btnTestButton_clicked(){

    // example to listen to events
    QtConcurrent::run(this, &MainWindow::EventsLoop);
    //RDK->EventsLoop();


    if (!Check_Robot()){ return; }

    /*
    if (false) {
        //ROBOT->setPose
        Item robot_base = ROBOT->Parent();

        // Create or select a reference frame
        Item ref = RDK->getItem("Ref 1");
        if (!ref.Valid()){
            ref = RDK->AddFrame("Ref 1", &robot_base);
            ref.setPose(Mat::transl(100,200,300));
        }

        // create or load a tool
        Item tcp = RDK->getItem("Tool 1");
        if (!tcp.Valid()){
            tcp = ROBOT->AddTool(Mat::transl(0,0,150)*Mat::rotx(0.5*M_PI), "Tool 1");
        }

        // Set the robot base and tool
        ROBOT->setPoseFrame(ref);
        ROBOT->setPoseTool(tcp);

        Mat TargetPose = ROBOT->Pose();

        RDK->Render();

        Item prog = RDK->AddProgram("AutomaticProg", ROBOT);
        prog.setPoseFrame(ref);
        prog.setPoseTool(tcp);

        //Create a target (or select it)
        for (int i=0; i<=100; i++){
            QString target_name(QString("Target %1").arg(i+1));
            Item target = RDK->getItem(target_name, RoboDK::ITEM_TYPE_TARGET);
            if (!target.Valid()){
                target = RDK->AddTarget(target_name, &ref);
                target.setAsCartesianTarget();
                target.setPose(TargetPose * transl(0,0, 2*i) * rotz((i/100.0)*30.0*M_PI/180.0));
                if (i == 0){
                    prog.MoveJ(target);
                } else {
                    prog.MoveL(target);
                }
            }
        }
    }

    // manage a gripper
    if (false){
        //RDK->getItem("");
        Item gripper = ROBOT->Childs()[0];

        tJoints joints_original = gripper.Joints();

        double pjoints[] = {0};
        tJoints joints_new(pjoints, 1);
        gripper.setJoints(joints_new);

        gripper.MoveJ(joints_original);
    }

    //target.Delete();

    // example to display text in the 3D window
    if (true){
        Item station = RDK->getActiveStation();
        Item text_object = RDK->AddFile("", &station);
        text_object.setPose(transl(200,300,100));
        //text_object.setPoseAbs(transl(200,300,100));
        text_object.setName("Display 3D text");
    }

    // Other useful functions to draw primitives
    //RDK->AddShape()
    //RDK->AddCurve()
    //RDK->AddPoints()


    return;
*/



    //int runmode = RDK->RunMode(); // retrieve the run mode

    //RoboDK *RDK = new RoboDK();
    //Item *ROBOT = new Item(RDK->getItem("Motoman SV3"));

    // Draw a hexagon inside a circle of radius 100.0 mm
    int n_sides = 6;
    float size = 100.0;
    // retrieve the reference frame and the tool frame (TCP)
    Mat pose_frame = ROBOT->PoseFrame();
    Mat pose_tool = ROBOT->PoseTool();
    Mat pose_ref = ROBOT->Pose();

    // Program start
    ROBOT->MoveJ(pose_ref);
    ROBOT->setPoseFrame(pose_frame);  // set the reference frame
    ROBOT->setPoseTool(pose_tool);    // set the tool frame: important for Online Programming
    ROBOT->setSpeed(100);             // Set Speed to 100 mm/s
    ROBOT->setRounding(5);            // set the rounding instruction (C_DIS & APO_DIS / CNT / ZoneData / Blend Radius / ...)
    ROBOT->RunInstruction("CallOnStart"); // run a program
    for (int i = 0; i <= n_sides; i++) {
        // calculate angle in degrees:
        double angle = ((double) i / n_sides) * 360.0;

        // create a pose relative to the pose_ref
        Mat pose_i(pose_ref);
        pose_i.rotate(angle,0,0,1.0);
        pose_i.translate(size, 0, 0);
        pose_i.rotate(-angle,0,0,1.0);

        // add a comment (when generating code)
        ROBOT->RunInstruction("Moving to point " + QString::number(i), RoboDK::INSTRUCTION_COMMENT);

        // example to retrieve the pose as Euler angles (X,Y,Z,W,P,R)
        double xyzwpr[6];
        pose_i.ToXYZRPW(xyzwpr);

        ROBOT->MoveL(pose_i);  // move the robot
    }
    ROBOT->RunInstruction("CallOnFinish");
    ROBOT->MoveL(pose_ref);     // move back to the reference point

    return;


    // Example to iterate through all the existing targets in the station (blocking):
    QList<Item> targets = RDK->getItemList(RoboDK::ITEM_TYPE_TARGET);
    foreach (Item target, targets){
        if (target.Type() == RoboDK::ITEM_TYPE_TARGET){
            ui->statusBar->showMessage("Moving to: " + target.Name());
            qApp->processEvents();
            ROBOT->MoveJ(target);
        }
    }
    return;

    QList<Item> list = RDK->getItemList();


    Mat pose_robot_base_abs = ROBOT->PoseAbs();
    Mat pose_robot = ROBOT->Pose();
    Mat pose_tcp = ROBOT->PoseTool();

    qDebug() << "Absolute Position of the robot:";
    qDebug() << pose_robot_base_abs;

    qDebug() << "Current robot position (active tool with respect to the active reference):";
    qDebug() << pose_robot;

    qDebug() << "Position of the active TCP:";
    qDebug() << pose_tcp;

    QList<Item> tool_list = ROBOT->Childs();
    if (tool_list.length() <= 0){
        statusBar()->showMessage("No tools available for the robot " + ROBOT->Name());
        return;
    }

    Item tool = tool_list.at(0);
    qDebug() << "Using tool: " << tool.Name();

    Mat pose_robot_flange_abs = tool.PoseAbs();
    pose_tcp = tool.PoseTool();
    Mat pose_tcp_abs = pose_robot_flange_abs * pose_tcp;


    Item object = RDK->getItem("", RoboDK::ITEM_TYPE_FRAME);
    Mat pose_object_abs = object.PoseAbs();
    qDebug() << pose_tcp;

    Mat tcp_wrt_obj = pose_object_abs.inverted() * pose_tcp_abs;

    qDebug() << "Pose of the TCP with respect to the selected reference frame";
    qDebug() << tcp_wrt_obj;

    tXYZWPR xyzwpr;
    tcp_wrt_obj.ToXYZRPW(xyzwpr);

    this->statusBar()->showMessage(QString("Tool with respect to %1").arg(object.Name()) + QString(": [X,Y,Z,W,P,R]=[%1, %2, %3, %4, %5, %6] mm/deg").arg(xyzwpr[0],0,'f',3).arg(xyzwpr[1],0,'f',3).arg(xyzwpr[2],0,'f',3).arg(xyzwpr[3],0,'f',3).arg(xyzwpr[4],0,'f',3).arg(xyzwpr[5],0,'f',3) );


    // Example to define a reference frame given 3 points:
    tMatrix2D* framePts = Matrix2D_Create();
    Matrix2D_Set_Size(framePts, 3, 3);
    double *p1 = Matrix2D_Get_col(framePts, 0);
    double *p2 = Matrix2D_Get_col(framePts, 1);
    double *p3 = Matrix2D_Get_col(framePts, 2);

    // Define point 1:
    p1[0] = 100;
    p1[1] = 200;
    p1[2] = 300;

    // Define point 2:
    p2[0] = 500;
    p2[1] = 200;
    p2[2] = 300;

    // Define point 3:
    p3[0] = 100;
    p3[1] = 500;
    p3[2] = 300;
    Mat diagLocalFrame = RDK->CalibrateReference(framePts, RoboDK::CALIBRATE_FRAME_3P_P1_ON_X);
    Item localPlaneFrame = RDK->AddFrame("Plane Coord");
    localPlaneFrame.setPose(diagLocalFrame);
    Matrix2D_Delete(&framePts);
    return;




    // Inverse kinematics test:
    //Mat tool_pose = transl(10,20,30);
    //Mat ref_pose = transl(100, 100,500);

    qDebug() << "Testing pose:";
    qDebug() << "Using robot: " << ROBOT;
    Mat pose_test(0.733722985, 0.0145948902, -0.679291904, -814.060547, 0.000000000, -0.999769211, -0.0214804877, -8.96536446, -0.679448724, 0.0157607272, -0.733553648, 340.561951);
    ROBOT->setAccuracyActive(1);
    pose_test.MakeHomogeneous();
    qDebug() << pose_test;

    // Calculate a single solution (closest to the current robot position):
    tJoints joints = ROBOT->SolveIK(pose_test); //, &tool_pose, &ref_pose);
    qDebug() << "Solution : " << joints;

    // Iterate through all possible solutions
    // Calculate all nominal solutions:
    ROBOT->setAccuracyActive(0);
    auto all_solutions = ROBOT->SolveIK_All(pose_test); //, &tool_pose, &ref_pose);
    // Use accurate kinematics and calculate inverse kinematics using the closest point
    ROBOT->setAccuracyActive(1);
    for (int i=0; i<all_solutions.length(); i++){
        tJoints joints_nominal_i = all_solutions.at(i);
        qDebug() << "Nominal  solution " << i << ": " << joints_nominal_i;
        tJoints joints_accurate_i = ROBOT->SolveIK(pose_test, joints_nominal_i); //, &tool_pose, &ref_pose);
        qDebug() << "Accurate solution " << i << ": " << joints_accurate_i;
    }




    /*qDebug() << joints.ToString();
    tJoints joints = ROBOT->SolveIK(pose_problems);
    qDebug() << joints.ToString();
    */
    return;


    /*
    // Example to create the ISO cube program
        tXYZ xyz;
        xyz[0] = 100;
        xyz[1] = 200;
        xyz[2] = 300;
        RDK->Popup_ISO9283_CubeProgram(ROBOT, xyz, 100, false);
        return;
    */

}

void MainWindow::on_btnTXn_clicked(){ IncrementalMove(0, -1); }
void MainWindow::on_btnTYn_clicked(){ IncrementalMove(1, -1); }
void MainWindow::on_btnTZn_clicked(){ IncrementalMove(2, -1); }
void MainWindow::on_btnRXn_clicked(){ IncrementalMove(3, -1); }
void MainWindow::on_btnRYn_clicked(){ IncrementalMove(4, -1); }
void MainWindow::on_btnRZn_clicked(){ IncrementalMove(5, -1); }

void MainWindow::on_btnTXp_clicked(){ IncrementalMove(0, +1); }
void MainWindow::on_btnTYp_clicked(){ IncrementalMove(1, +1); }
void MainWindow::on_btnTZp_clicked(){ IncrementalMove(2, +1); }
void MainWindow::on_btnRXp_clicked(){ IncrementalMove(3, +1); }
void MainWindow::on_btnRYp_clicked(){ IncrementalMove(4, +1); }
void MainWindow::on_btnRZp_clicked(){ IncrementalMove(5, +1); }

void MainWindow::IncrementalMove(int id, double sense){
    if (!Check_Robot()) { return; }

    // check the index
    if (id < 0 || id >= 6){
        qDebug()<< "Invalid id provided to for an incremental move";
        return;
    }

    // calculate the relative movement
    double step = sense * ui->spnStep->value();

    // apply to XYZWPR
    tXYZWPR xyzwpr;
    for (int i=0; i<6; i++){
        xyzwpr[i] = 0;
    }
    xyzwpr[id] = step;

    Mat pose_increment;
    pose_increment.FromXYZRPW(xyzwpr);

    Mat pose_robot = ROBOT->Pose();

    Mat pose_robot_new;

    // apply relative to the TCP:
    pose_robot_new = pose_robot * pose_increment;

    ROBOT->MoveJ(pose_robot_new);

}


void MainWindow::on_radSimulation_clicked()
{
    if (!Check_Robot()) { return; }

    // Important: stop any previous program generation (if we selected offline programming mode)
    ROBOT->Finish();

    // Set simulation mode
    RDK->setRunMode(RoboDK::RUNMODE_SIMULATE);
}

void MainWindow::on_radOfflineProgramming_clicked()
{
    if (!Check_Robot()) { return; }

    // Important: stop any previous program generation (if we selected offline programming mode)
    ROBOT->Finish();

    // Set simulation mode
    RDK->setRunMode(RoboDK::RUNMODE_MAKE_ROBOTPROG);

    // specify a program name, a folder to save the program and a post processor if desired
    RDK->ProgramStart("NewProgram");
}

void MainWindow::on_radRunOnRobot_clicked()
{
    if (!Check_Robot()) { return; }

    // Important: stop any previous program generation (if we selected offline programming mode)
    ROBOT->Finish();

    // Connect to real robot
    if (ROBOT->Connect())
    {
        // Set simulation mode
        RDK->setRunMode(RoboDK::RUNMODE_RUN_ROBOT);
    }
    else
    {
        ui->statusBar->showMessage("Can't connect to the robot. Check connection and parameters.");
    }

}

void MainWindow::on_btnMakeProgram_clicked()
{
    if (!Check_Robot()) { return; }

    // Trigger program generation
    ROBOT->Finish();

}

void MainWindow::robodk_window_clear(){
    if (robodk_window != NULL){
        robodk_window->setParent(NULL);
        robodk_window->setFlags(Qt::Window);
        //robodk_window->deleteLater();
        robodk_window = NULL;
        ui->widgetRoboDK->layout()->deleteLater();
    }
    // Make sure RoboDK widget is hidden
    ui->widgetRoboDK->hide();

    // Adjust the main window size
    adjustSize();
}


void MainWindow::on_radShowRoboDK_clicked()
{
    if (!Check_RoboDK()){ return; }

    // Hide embedded window
    robodk_window_clear();

    RDK->setWindowState(RoboDK::WINDOWSTATE_NORMAL);
    RDK->setWindowState(RoboDK::WINDOWSTATE_SHOW);
}

void MainWindow::on_radHideRoboDK_clicked()
{
    if (!Check_RoboDK()){ return; }

    // Hide embedded window
    robodk_window_clear();

    RDK->setWindowState(RoboDK::WINDOWSTATE_HIDDEN);

}

#ifdef _MSC_VER
HWND FindTopWindow(DWORD pid)
{
    std::pair<HWND, DWORD> params = { 0, pid };

    // Enumerate the windows using a lambda to process each window
    BOOL bResult = EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
    {
        auto pParams = (std::pair<HWND, DWORD>*)(lParam);

        DWORD processId;
        if (GetWindowThreadProcessId(hwnd, &processId) && processId == pParams->second)
        {
            // Stop enumerating
            SetLastError(-1);
            pParams->first = hwnd;
            return FALSE;
        }

        // Continue enumerating
        return TRUE;
    }, (LPARAM)&params);

    if (!bResult && GetLastError() == -1 && params.first)
    {
        return params.first;
    }

    return 0;
}
#endif

void MainWindow::on_radIntegrateRoboDK_clicked()
{
    if (!Check_RoboDK()){ return; }

    qint64 procWID = RDK->ProcessID();
    if (procWID == 0) {
        ui->statusBar->showMessage("Invalid handle. Close RoboDK and open RoboDK with this application");
        return;
    }


#ifdef _MSC_VER
    if (procWID != 0){
        qDebug() << "Using parent process=" << procWID;
        //SetParent((HWND) procWID, (HWND)widget_container->window()->winId());

        // Retrieve the top level window
        HWND wid_rdk = FindTopWindow(procWID);
        qDebug() << "HWND RoboDK window: " << wid_rdk;
        //SetParent((HWND) wid_rdk, (HWND)widget_container->winId());//->window()->winId());
        if (wid_rdk == NULL){
            ui->statusBar->showMessage("RoboDK top level window was not found...");
            return;
        }
        //HWND wid_rdk = (HWND) RDK->WindowID();
        // set parent widget
        robodk_window = QWindow::fromWinId((WId)wid_rdk);
        QWidget *new_widget = createWindowContainer(robodk_window);
        QVBoxLayout *vl = new QVBoxLayout();
        ui->widgetRoboDK->setLayout(vl);
        vl->addWidget(new_widget);
        new_widget->show();
        this->adjustSize();

        RDK->setWindowState(RoboDK::WINDOWSTATE_SHOW);
        RDK->setWindowState(RoboDK::WINDOWSTATE_FULLSCREEN_CINEMA);

        // Show the RoboDK widget (embedded screen)
        ui->widgetRoboDK->show();
    }
#endif

}



/// <summary>
/// This is a sample function that is executed when a new RoboDK Event occurs.
/// </summary>
/// <param name="evt"></param>
/// <param name="itm"></param>
/// <returns></returns>
bool MainWindow::SampleRoboDkEvent(int evt, Item itm)
{
    qDebug() << "";
    qDebug() << "**** New event ****";

    //If this runs in a seperate thread from the api instance thaw sapwned it you won't be able to do
    //Regular api calls like item.Valid()
    //if (itm.Valid())
    //{
        //qDebug() << "  Item: " + itm.Name() + " -> Type: " + itm.Type();
    //}
    //else
    //{
        //Console.WriteLine("  Item not applicable");
    //}

    switch (evt)
    {
        case RoboDK::EVENT_SELECTION_TREE_CHANGED:
            qDebug() << "Event: Selection changed (the tree was selected)";
            break;
        case RoboDK::EVENT_ITEM_MOVED:
            qDebug() << "Event: Item Moved";
            break;
        case RoboDK::EVENT_REFERENCE_PICKED:
            qDebug() << "Event: Reference Picked";
            break;
        case RoboDK::EVENT_REFERENCE_RELEASED:
            qDebug() << "Event: Reference Released";
            break;
        case RoboDK::EVENT_TOOL_MODIFIED:
            qDebug() << "Event: Tool Modified";
            break;
        case RoboDK::EVENT_3DVIEW_MOVED:
            qDebug() << "Event: 3D view moved"; // use ViewPose to retrieve the pose of the camera
            break;
        case RoboDK::EVENT_ROBOT_MOVED:
            qDebug() << "Event: Robot moved";
            break;

        // Important: The following events require consuming additional data from the _COM_EVT buffer
        case RoboDK::EVENT_SELECTION_3D_CHANGED:
        {
            qDebug() << "Event: Selection changed";
            // data contains the following information (24 values):
            // pose (16), xyz selection (3), ijk normal (3), picked feature id (1), picked id (1)
            //double[] data = RDK._recv_Array(RDK->_COM_EVT);
            double data[24];
            int valueCount;
            RDK->Event_Receive_3D_POS(data,&valueCount);
            //
            Mat pose_abs = Mat(data);
            double xyz[3] = { data[16], data[17], data[18] };
            double ijk[3] = { data[19], data[20], data[21] };
            int feature_type = data[22];
            int feature_id = data[23];
            qDebug() << "Additional event data - Absolute position (PoseAbs):";
            qDebug() << pose_abs.ToString();
            qDebug() << "Additional event data - Point and Normal (point selected in relative coordinates)";
            qDebug() << QString::number(xyz[0]) + "," + QString::number(xyz[1]) + "," + QString::number(xyz[2]);
            qDebug() << QString::number(ijk[0]) + "," + QString::number(ijk[1]) + "," + QString::number(ijk[2]);
            qDebug() << "Feature Type and ID";
            qDebug() << QString::number(feature_type) + "-" + QString::number(feature_id);
            break;
        }
        case RoboDK::EVENT_KEY:
        {
            int mouseData[3];
            RDK->Event_Receive_Mouse_data(mouseData);
            int key_press = mouseData[0]; // 1 = key pressed, 0 = key released
            int key_id = mouseData[1]; // Key id as per Qt mappings: https://doc.qt.io/qt-5/qt.html#Key-enum
            int modifiers = mouseData[2]; // Modifier bits as per Qt mappings: https://doc.qt.io/qt-5/qt.html#KeyboardModifier-enum
            qDebug() << "Event: Key pressed: " + QString::number(key_id) + " " + ((key_press > 0) ? "Pressed" : "Released") + ". Modifiers: " + QString::number(modifiers);
            break;
        }
        case RoboDK::EVENT_ITEM_MOVED_POSE:
        {
            Mat pose_rel;
            bool flag = RDK->Event_Receive_Event_Moved(&pose_rel);
            //int nvalues = _recv_Int(_COM_EVT);
            //Mat pose_rel = _recv_Pose(_COM_EVT);
            if (flag == false)
            {
                // future compatibility
            }
            qDebug() << "Event: item moved. Relative pose: " + pose_rel.ToString();
            break;
        }
        default:
            qDebug() << "Unknown event " + QString::number(evt);
            break;
    }
    return true;
}

bool MainWindow::EventsLoop()
{
    RDK->EventsListen();

    qDebug() << "Events loop started";
    while (RDK->Event_Connected())
    {
        int evt;
        Item itm;
        RDK->WaitForEvent(evt,itm);
        SampleRoboDkEvent(evt, itm);
    }
    qDebug() << "Event loop finished";
    return true;
}

void MainWindow::on_btnEmbed_clicked()
{
    if (!Check_RoboDK()){ return; }
    on_radShowRoboDK_clicked();
    EmbedExample *newWindow = new EmbedExample();
    newWindow->show();
    QString windowName = newWindow->windowTitle();
    qDebug() << windowName;
    RDK->EmbedWindow(windowName);

}
///************************************ RoboDK Code End ************************************
///*****************************************************************************************
