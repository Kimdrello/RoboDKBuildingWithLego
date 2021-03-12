#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


// TIP: use #define RDK_SKIP_NAMESPACE to avoid using namespaces
#include "robodk_api.h"
#include <stdint.h>
#include "duplobrick.h"


using namespace RoboDK_API;
using namespace std;


namespace Ui {
class MainWindow;
}


/// Example's main window (robot panel)
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    /// Select a robot
    void Select_Robot();

    /// Validate if RoboDK is running (RDK is valid)
    bool Check_RoboDK();

    /// Validate if a Robot has been selected (ROBOT variable is valid)
    bool Check_Robot();

    /// Apply an incremental movement
    void IncrementalMove(int id, double sense);

    list<Item> pickUpBrick_and_Move(RoboDK *rdk, Item *robot, Item *parent_frame, Item *work_frame, Item *tool, list<Item> dispenser, DuploBrick *brick);

    int CheckIfPlaceable(int studsXdir, int studsYdir, bool arr[50][50][50], int x, int y, int z);

    list<Item> fillDispenser(DuploBrick *brick, double RGB[4]);

    void UpdateDispenser(list<Item> dispenser, QString brickName);

    bool checkIfdispenserIsEmpty(Item *robot);

    void setUpUIGrid();

    void blocking_task();

    void EmergencyTriggered();

    void BuildStructure();

    void AddSupportBricks(DuploBrick *brick, int x, int y, int z);

    bool ListComparison(DuploBrick first, DuploBrick second);

    void PlaceHolderFunctionScenario4();

    bool EventsLoop();
    bool SampleRoboDkEvent(int evt, Item itm);
private slots:
    void on_btnLoadFile_clicked();
    void on_btnSelectRobot_clicked();
    void on_btnTestButton_clicked();
    void on_btnGetPosition_clicked();
    void on_btnMoveJoints_clicked();
    void on_btnMovePose_clicked();
    void on_btnProgRun_clicked();
    void on_btnTXn_clicked();
    void on_btnTYn_clicked();
    void on_btnTZn_clicked();
    void on_btnRXn_clicked();
    void on_btnRYn_clicked();
    void on_btnRZn_clicked();
    void on_btnTXp_clicked();
    void on_btnTYp_clicked();
    void on_btnTZp_clicked();
    void on_btnRXp_clicked();
    void on_btnRYp_clicked();
    void on_btnRZp_clicked();

    void on_btnThingMLRun_clicked();
    void on_fill4x2feederbtn_clicked();
    void on_fill2x2feederbtn_clicked();
    void on_fill1x1feederbtn_clicked();
    void on_dropBrickbtn_clicked();
    void on_humanInterferencebtn_clicked();
    void on_emergencyStopbtn_clicked();
    void on_releaseEmergenyStopbtn_clicked();

    void on_btnOneCoordOneBrick_clicked();
    void on_btnOneCoordOneStud_clicked();

    void on_btnAddLayer_clicked();
    void on_load3DModelbtn_clicked();
    void on_resetLayerbtn_clicked();

    void on_radSimulation_clicked();
    void on_radOfflineProgramming_clicked();
    void on_radRunOnRobot_clicked();
    void on_btnMakeProgram_clicked();

    void on_radShowRoboDK_clicked();
    void on_radHideRoboDK_clicked();
    void on_radIntegrateRoboDK_clicked();

    void on_btnEmbed_clicked();

private:
    void robodk_window_clear();

private:
    Ui::MainWindow *ui;

    /// Pointer to RoboDK
    RoboDK *RDK;

    /// Pointer to the robot item
    Item *ROBOT;

    /// Pointer to the RoboDK window
    QWindow *robodk_window;
};

#endif // MAINWINDOW_H
