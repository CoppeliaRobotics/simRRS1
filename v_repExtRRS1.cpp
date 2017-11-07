#include "v_repExtRRS1.h"
#include "v_repLib.h"
#include "scriptFunctionData.h"
#include "socketOutConnection.h"
#include "inputOutputBlock.h"
#include <iostream>
#include <boost/lexical_cast.hpp>

#ifdef _WIN32
    #ifdef QT_COMPIL
        #include <direct.h>
    #else
        #include <Shellapi.h>
        #include <shlwapi.h>
        #pragma comment(lib, "Shlwapi.lib")
        #pragma comment(lib, "Shell32.lib")
    #endif
#endif
#ifdef QT_COMPIL
    #include <QString>
    #include <QProcess>
#endif
#if defined (__linux) || defined (__APPLE__)
    #include <unistd.h>
    #define _stricmp(x,y) strcasecmp(x,y)
#endif

#define PLUGIN_VERSION 3 // 2 since 3.4.1
#define CONCAT(x,y,z) x y z
#define strConCat(x,y,z)    CONCAT(x,y,z)

LIBRARY vrepLib;

struct sRcsServer
{
    CSocketOutConnection* connection;
    int scriptId;
    int rcsServerHandle;
    bool isCurrentServer;
};

std::vector<sRcsServer> allRcsServers;
int nextRcsServerHandle=0;
std::string currentDirAndPath;

int getServerIndexFromServerHandle(int serverHandle)
{
    for (unsigned int i=0;i<allRcsServers.size();i++)
    {
        if (allRcsServers[i].rcsServerHandle==serverHandle)
            return(i);
    }
    return(-1);
}

int getServerIndexFromScriptId(int scriptId)
{
    for (unsigned int i=0;i<allRcsServers.size();i++)
    {
        if ( (allRcsServers[i].scriptId==scriptId)&&allRcsServers[i].isCurrentServer )
            return(i);
    }
    return(-1);
}

std::string getPartialString(const std::string& str,int charCnt)
{
    std::string retString;
    retString.assign(str.begin(),str.begin()+charCnt);
    return(retString);
}

// Following for backward compatibility:
#define LUA_START_RCS_SERVER_COMMANDOLD "simExtRRS1_startRcsServer"
#define LUA_SELECT_RCS_SERVER_COMMANDOLD "simExtRRS1_selectRcsServer"
#define LUA_STOP_RCS_SERVER_COMMANDOLD "simExtRRS1_stopRcsServer"
#define LUA_INITIALIZE_COMMANDOLD "simExtRRS1_INITIALIZE"
#define LUA_RESET_COMMANDOLD "simExtRRS1_RESET"
#define LUA_TERMINATE_COMMANDOLD "simExtRRS1_TERMINATE"
#define LUA_GET_ROBOT_STAMP_COMMANDOLD "simExtRRS1_GET_ROBOT_STAMP"
#define LUA_GET_HOME_JOINT_POSITION_COMMANDOLD "simExtRRS1_GET_HOME_JOINT_POSITION"
#define LUA_GET_RCS_DATA_COMMANDOLD "simExtRRS1_GET_RCS_DATA"
#define LUA_MODIFY_RCS_DATA_COMMANDOLD "simExtRRS1_MODIFY_RCS_DATA"
#define LUA_SAVE_RCS_DATA_COMMANDOLD "simExtRRS1_SAVE_RCS_DATA"
#define LUA_LOAD_RCS_DATA_COMMANDOLD "simExtRRS1_LOAD_RCS_DATA"
#define LUA_GET_INVERSE_KINEMATIC_COMMANDOLD "simExtRRS1_GET_INVERSE_KINEMATIC"
#define LUA_GET_FORWARD_KINEMATIC_COMMANDOLD "simExtRRS1_GET_FORWARD_KINEMATIC"
#define LUA_MATRIX_TO_CONTROLLER_POSITION_COMMANDOLD "simExtRRS1_MATRIX_TO_CONTROLLER_POSITION"
#define LUA_CONTROLLER_POSITION_TO_MATRIX_COMMANDOLD "simExtRRS1_CONTROLLER_POSITION_TO_MATRIX"
#define LUA_GET_CELL_FRAME_COMMANDOLD "simExtRRS1_GET_CELL_FRAME"
#define LUA_MODIFY_CELL_FRAME_COMMANDOLD "simExtRRS1_MODIFY_CELL_FRAME"
#define LUA_SELECT_WORK_FRAMES_COMMANDOLD "simExtRRS1_SELECT_WORK_FRAMES"
#define LUA_SET_INITIAL_POSITION_COMMANDOLD "simExtRRS1_SET_INITIAL_POSITION"
#define LUA_SET_NEXT_TARGET_COMMANDOLD "simExtRRS1_SET_NEXT_TARGET"
#define LUA_GET_NEXT_STEP_COMMANDOLD "simExtRRS1_GET_NEXT_STEP"
#define LUA_SET_INTERPOLATION_TIME_COMMANDOLD "simExtRRS1_SET_INTERPOLATION_TIME"
#define LUA_SELECT_MOTION_TYPE_COMMANDOLD "simExtRRS1_SELECT_MOTION_TYPE"
#define LUA_SELECT_TARGET_TYPE_COMMANDOLD "simExtRRS1_SELECT_TARGET_TYPE"
#define LUA_SELECT_TRAJECTORY_MODE_COMMANDOLD "simExtRRS1_SELECT_TRAJECTORY_MODE"
#define LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMANDOLD "simExtRRS1_SELECT_ORIENTATION_INTERPOLATION_MODE"
#define LUA_SELECT_DOMINANT_INTERPOLATION_COMMANDOLD "simExtRRS1_SELECT_DOMINANT_INTERPOLATION"
#define LUA_SET_ADVANCE_MOTION_COMMANDOLD "simExtRRS1_SET_ADVANCE_MOTION"
#define LUA_SET_MOTION_FILTER_COMMANDOLD "simExtRRS1_SET_MOTION_FILTER"
#define LUA_SET_OVERRIDE_POSITION_COMMANDOLD "simExtRRS1_SET_OVERRIDE_POSITION"
#define LUA_REVERSE_MOTION_COMMANDOLD "simExtRRS1_REVERSE_MOTION"
#define LUA_SET_PAYLOAD_PARAMETER_COMMANDOLD "simExtRRS1_SET_PAYLOAD_PARAMETER"
#define LUA_SELECT_TIME_COMPENSATION_COMMANDOLD "simExtRRS1_SELECT_TIME_COMPENSATION"
#define LUA_SET_CONFIGURATION_CONTROL_COMMANDOLD "simExtRRS1_SET_CONFIGURATION_CONTROL"
#define LUA_SET_JOINT_SPEEDS_COMMANDOLD "simExtRRS1_SET_JOINT_SPEEDS"
#define LUA_SET_CARTESIAN_POSITION_SPEED_COMMANDOLD "simExtRRS1_SET_CARTESIAN_POSITION_SPEED"
#define LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMANDOLD "simExtRRS1_SET_CARTESIAN_ORIENTATION_SPEED"
#define LUA_SET_JOINT_ACCELERATIONS_COMMANDOLD "simExtRRS1_SET_JOINT_ACCELERATIONS"
#define LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMANDOLD "simExtRRS1_SET_CARTESIAN_POSITION_ACCELERATION"
#define LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMANDOLD "simExtRRS1_SET_CARTESIAN_ORIENTATION_ACCELERATION"
#define LUA_SET_JOINT_JERKS_COMMANDOLD "simExtRRS1_SET_JOINT_JERKS"
#define LUA_SET_MOTION_TIME_COMMANDOLD "simExtRRS1_SET_MOTION_TIME"
#define LUA_SET_OVERRIDE_SPEED_COMMANDOLD "simExtRRS1_SET_OVERRIDE_SPEED"
#define LUA_SET_OVERRIDE_ACCELERATION_COMMANDOLD "simExtRRS1_SET_OVERRIDE_ACCELERATION"
#define LUA_SELECT_FLYBY_MODE_COMMANDOLD "simExtRRS1_SELECT_FLYBY_MODE"
#define LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMANDOLD "simExtRRS1_SET_FLYBY_CRITERIA_PARAMETER"
#define LUA_SELECT_FLYBY_CRITERIA_COMMANDOLD "simExtRRS1_SELECT_FLYBY_CRITERIA"
#define LUA_CANCEL_FLYBY_CRITERIA_COMMANDOLD "simExtRRS1_CANCEL_FLYBY_CRITERIA"
#define LUA_SELECT_POINT_ACCURACY_COMMANDOLD "simExtRRS1_SELECT_POINT_ACCURACY"
#define LUA_SET_POINT_ACCURACY_PARAMETER_COMMANDOLD "simExtRRS1_SET_POINT_ACCURACY_PARAMETER"
#define LUA_SET_REST_PARAMETER_COMMANDOLD "simExtRRS1_SET_REST_PARAMETER"
#define LUA_GET_CURRENT_TARGETID_COMMANDOLD "simExtRRS1_GET_CURRENT_TARGETID"
#define LUA_SELECT_TRACKING_COMMANDOLD "simExtRRS1_SELECT_TRACKING"
#define LUA_SET_CONVEYOR_POSITION_COMMANDOLD "simExtRRS1_SET_CONVEYOR_POSITION"
#define LUA_DEFINE_EVENT_COMMANDOLD "simExtRRS1_DEFINE_EVENT"
#define LUA_CANCEL_EVENT_COMMANDOLD "simExtRRS1_CANCEL_EVENT"
#define LUA_GET_EVENT_COMMANDOLD "simExtRRS1_GET_EVENT"
#define LUA_STOP_MOTION_COMMANDOLD "simExtRRS1_STOP_MOTION"
#define LUA_CONTINUE_MOTION_COMMANDOLD "simExtRRS1_CONTINUE_MOTION"
#define LUA_CANCEL_MOTION_COMMANDOLD "simExtRRS1_CANCEL_MOTION"
#define LUA_GET_MESSAGE_COMMANDOLD "simExtRRS1_GET_MESSAGE"
#define LUA_SELECT_WEAVING_MODE_COMMANDOLD "simExtRRS1_SELECT_WEAVING_MODE"
#define LUA_SELECT_WEAVING_GROUP_COMMANDOLD "simExtRRS1_SELECT_WEAVING_GROUP"
#define LUA_SET_WEAVING_GROUP_PARAMETER_COMMANDOLD "simExtRRS1_SET_WEAVING_GROUP_PARAMETER"
#define LUA_DEBUG_COMMANDOLD "simExtRRS1_DEBUG"
#define LUA_EXTENDED_SERVICE_COMMANDOLD "simExtRRS1_EXTENDED_SERVICE"

// --------------------------------------------------------------------------------------
// Auxiliary function
// --------------------------------------------------------------------------------------
#define LUA_START_RCS_SERVER_COMMAND "simRRS1.startRcsServer"

const int inArgs_START_RCS_SERVER[]={
    3,
    sim_script_arg_string,0,
    sim_script_arg_string,0,
    sim_script_arg_int32,0,
};

void LUA_START_RCS_SERVER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    int handle=-1; // means error
    if (D.readDataFromStack(p->stackID,inArgs_START_RCS_SERVER,inArgs_START_RCS_SERVER[0],LUA_START_RCS_SERVER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        std::string arguments;
        arguments+=inData->at(0).stringData[0];
        arguments+=" ";
        arguments+=inData->at(1).stringData[0];
        arguments+=" ";
        arguments+=boost::lexical_cast<std::string>(inData->at(2).int32Data[0]);

        #ifdef _WIN32
            ShellExecuteA(NULL,"open","rcsServer",arguments.c_str(),NULL,SW_SHOWDEFAULT);
            Sleep(1000);
        #else
            #ifdef QT_COMPIL
                QStringList strList;
                strList << QString(inData->at(0).stringData[0].c_str());
                strList << QString(inData->at(1).stringData[0].c_str());
                strList << QString(std::string(boost::lexical_cast<std::string>(inData->at(2).int32Data[0])).c_str());
                QProcess::startDetached("./rcsServer",strList,QString(currentDirAndPath.c_str()),NULL);
                sleep(1);
            #else
                pid_t pid=fork();
                if (pid==0)
                {
                    execl("rcsServer","rcsServer",inData->at(0).stringData[0].c_str(),inData->at(1).stringData[0].c_str(),boost::lexical_cast<std::string>(inData->at(2).int32Data[0]).c_str(), (char*) 0);
                    exit(0);
                }
                sleep(1);
            #endif
        #endif

        CSocketOutConnection* connection=new CSocketOutConnection("127.0.0.1",inData->at(2).int32Data[0]);
        if (connection->connectToServer()==1)
        { // we could connect!
            // Now check if the rcsSever could correctly load the specified rcs library and bind the rcs function:
            if (connection->sendData("0",1))
            { // send was successful
                // Now wait for the status reply:
                int dataSize;
                char* data=connection->receiveReplyData(dataSize);
                if (dataSize>0)
                { // data reception was ok!
                    if (data[0]==2)
                    { // ok, rcs server was launched and loaded the RCS library, and the RCS service function was successful!
                        handle=nextRcsServerHandle++;
                        sRcsServer server;
                        server.connection=connection;
                        server.scriptId=p->scriptID;
                        server.rcsServerHandle=handle;
                        server.isCurrentServer=true;
                        allRcsServers.push_back(server);
                    }
                    else
                    { // there was a problem
                        if (data[0]==0)
                            simSetLastError(LUA_START_RCS_SERVER_COMMAND,"The RCS server failed to load the RCS library.");
                        if (data[0]==1)
                            simSetLastError(LUA_START_RCS_SERVER_COMMAND,"The RCS server failed to bind the RCS service function.");
                        delete connection;
                    }
                    delete[] data;
                }
                else
                {
                    delete connection;
                    simSetLastError(LUA_START_RCS_SERVER_COMMAND,"Failed to receive data from the RCS server.");
                }
            }
            else
            {
                delete connection;
                simSetLastError(LUA_START_RCS_SERVER_COMMAND,"Failed to send data to the RCS server.");
            }
        }
        else
        {
            delete connection;
            simSetLastError(LUA_START_RCS_SERVER_COMMAND,"Failed to start or connect to RCS server. (i.e. 'rcsServer.exe')");
        }

    }
    D.pushOutData(CScriptFunctionDataItem(handle));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Auxiliary function
// --------------------------------------------------------------------------------------
#define LUA_SELECT_RCS_SERVER_COMMAND "simRRS1.selectRcsServer"

const int inArgs_SELECT_RCS_SERVER[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SELECT_RCS_SERVER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_RCS_SERVER,inArgs_SELECT_RCS_SERVER[0],LUA_SELECT_RCS_SERVER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int serverHandle=inData->at(0).int32Data[0];
        int scriptId=p->scriptID;
        int index=getServerIndexFromServerHandle(serverHandle);
        if (index!=-1)
        {
            if (allRcsServers[index].scriptId==scriptId)
            {
                for (unsigned int i=0;i<allRcsServers.size();i++)
                {
                    if (allRcsServers[i].scriptId==scriptId)
                        allRcsServers[i].isCurrentServer=false;
                }
                allRcsServers[index].isCurrentServer=true;
                success=true;
            }
            else
                simSetLastError(LUA_SELECT_RCS_SERVER_COMMAND,"Cannot access RCS server started in a different script.");
        }
        else
            simSetLastError(LUA_SELECT_RCS_SERVER_COMMAND,"Invalid RCS server handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Auxiliary function
// --------------------------------------------------------------------------------------
#define LUA_STOP_RCS_SERVER_COMMAND "simRRS1.stopRcsServer"

const int inArgs_STOP_RCS_SERVER[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_STOP_RCS_SERVER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(p->stackID,inArgs_STOP_RCS_SERVER,inArgs_STOP_RCS_SERVER[0],LUA_STOP_RCS_SERVER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int serverHandle=inData->at(0).int32Data[0];
        int scriptId=p->scriptID;
        int index=getServerIndexFromServerHandle(serverHandle);
        if (index!=-1)
        {
            if (allRcsServers[index].scriptId==scriptId)
            {
                delete allRcsServers[index].connection;
                allRcsServers.erase(allRcsServers.begin()+index);
                success=true;
            }
            else
                simSetLastError(LUA_STOP_RCS_SERVER_COMMAND,"Cannot access RCS server started in a different script.");
        }
        else
            simSetLastError(LUA_STOP_RCS_SERVER_COMMAND,"Invalid RCS server handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------
// INITIALIZE
// --------------------------------------------------------------------------------------
#define LUA_INITIALIZE_COMMAND "simRRS1.INITIALIZE"
const int inArgs_INITIALIZE[]={
    6,
    sim_script_arg_int32,0,
    sim_script_arg_string,0,
    sim_script_arg_string,0,
    sim_script_arg_string,0,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
};

void LUA_INITIALIZE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_INITIALIZE,inArgs_INITIALIZE[0],LUA_INITIALIZE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            CInputOutputBlock inputBlock(101,0);
            inputBlock.pushBackInteger(inData->at(0).int32Data[0]);
            inputBlock.pushBackString(inData->at(1).stringData[0]);
            inputBlock.pushBackString(inData->at(2).stringData[0]);
            inputBlock.pushBackString(inData->at(3).stringData[0]);
            inputBlock.pushBackInteger(inData->at(4).int32Data[0]);
            inputBlock.pushBackInteger(inData->at(5).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=16)
                        {
                            std::string rcsHandle=outputBlock.readBitstring2();
                            D.pushOutData(CScriptFunctionDataItem(&rcsHandle[0],BITSTRING2_SIZE));
                        }
                        if (s>=20)
                        {
                            int version1=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(version1));
                        }
                        if (s>=24)
                        {
                            int version2=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(version2));
                        }
                        if (s>=28)
                        {
                            int numberOfMsg=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(numberOfMsg));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_INITIALIZE_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_INITIALIZE_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_INITIALIZE_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_INITIALIZE_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// RESET
// --------------------------------------------------------------------------------------
#define LUA_RESET_COMMAND "simRRS1.RESET"
const int inArgs_RESET[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_RESET_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_RESET,inArgs_RESET[0],LUA_RESET_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(102,rcsHandle.c_str());
            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            int numberOfMsg=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(numberOfMsg));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_RESET_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_RESET_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_RESET_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_RESET_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// TERMINATE
// --------------------------------------------------------------------------------------
#define LUA_TERMINATE_COMMAND "simRRS1.TERMINATE"

const int inArgs_TERMINATE[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_TERMINATE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_TERMINATE,inArgs_TERMINATE[0],LUA_TERMINATE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(103,rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_TERMINATE_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_TERMINATE_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_TERMINATE_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_TERMINATE_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_ROBOT_STAMP
// --------------------------------------------------------------------------------------
#define LUA_GET_ROBOT_STAMP_COMMAND "simRRS1.GET_ROBOT_STAMP"
const int inArgs_GET_ROBOT_STAMP[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_GET_ROBOT_STAMP_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_ROBOT_STAMP,inArgs_GET_ROBOT_STAMP[0],LUA_GET_ROBOT_STAMP_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(104,rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string manipulator=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(manipulator));
                            std::string controller=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(controller));
                            std::string software=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(software));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_ROBOT_STAMP_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_ROBOT_STAMP_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_ROBOT_STAMP_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_ROBOT_STAMP_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_HOME_JOINT_POSITION
// --------------------------------------------------------------------------------------
#define LUA_GET_HOME_JOINT_POSITION_COMMAND "simRRS1.GET_HOME_JOINT_POSITION"
const int inArgs_GET_HOME_JOINT_POSITION[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_GET_HOME_JOINT_POSITION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_HOME_JOINT_POSITION,inArgs_GET_HOME_JOINT_POSITION[0],LUA_GET_HOME_JOINT_POSITION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(166,rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string jointPos(outputBlock.readJointPos());
                            D.pushOutData(CScriptFunctionDataItem(&jointPos[0],(int)jointPos.size()));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_HOME_JOINT_POSITION_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_HOME_JOINT_POSITION_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_HOME_JOINT_POSITION_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_HOME_JOINT_POSITION_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_RCS_DATA
// --------------------------------------------------------------------------------------
#define LUA_GET_RCS_DATA_COMMAND "simRRS1.GET_RCS_DATA"
const int inArgs_GET_RCS_DATA[]={
    4,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
    sim_script_arg_string,0,
};

void LUA_GET_RCS_DATA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_RCS_DATA,inArgs_GET_RCS_DATA[0],LUA_GET_RCS_DATA_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(105,rcsHandle.c_str());
            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);
            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);
            inputBlock.pushBackString(inData->at(3).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string paramId=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(paramId));

                            std::string paramContents=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(paramContents));

                            int permissions=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(permissions));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_RCS_DATA_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_RCS_DATA_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_RCS_DATA_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_RCS_DATA_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// MODIFY_RCS_DATA
// --------------------------------------------------------------------------------------
#define LUA_MODIFY_RCS_DATA_COMMAND "simRRS1.MODIFY_RCS_DATA"
const int inArgs_MODIFY_RCS_DATA[]={
    4,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_string,0,
    sim_script_arg_string,0,
};

void LUA_MODIFY_RCS_DATA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_MODIFY_RCS_DATA,inArgs_MODIFY_RCS_DATA[0],LUA_MODIFY_RCS_DATA_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(106,rcsHandle.c_str());
            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);
            inputBlock.pushBackString(inData->at(2).stringData[0]);
            inputBlock.pushBackString(inData->at(3).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_MODIFY_RCS_DATA_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_MODIFY_RCS_DATA_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_MODIFY_RCS_DATA_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_MODIFY_RCS_DATA_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SAVE_RCS_DATA
// --------------------------------------------------------------------------------------
#define LUA_SAVE_RCS_DATA_COMMAND "simRRS1.SAVE_RCS_DATA"
const int inArgs_SAVE_RCS_DATA[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_SAVE_RCS_DATA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SAVE_RCS_DATA,inArgs_SAVE_RCS_DATA[0],LUA_SAVE_RCS_DATA_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(107,rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SAVE_RCS_DATA_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SAVE_RCS_DATA_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SAVE_RCS_DATA_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SAVE_RCS_DATA_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// LOAD_RCS_DATA
// --------------------------------------------------------------------------------------
#define LUA_LOAD_RCS_DATA_COMMAND "simRRS1.LOAD_RCS_DATA"
const int inArgs_LOAD_RCS_DATA[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_LOAD_RCS_DATA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_LOAD_RCS_DATA,inArgs_LOAD_RCS_DATA[0],LUA_LOAD_RCS_DATA_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(108,rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            int msgNb=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(msgNb));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_LOAD_RCS_DATA_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_LOAD_RCS_DATA_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_LOAD_RCS_DATA_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_LOAD_RCS_DATA_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_INVERSE_KINEMATIC
// --------------------------------------------------------------------------------------
#define LUA_GET_INVERSE_KINEMATIC_COMMAND "simRRS1.GET_INVERSE_KINEMATIC"
const int inArgs_GET_INVERSE_KINEMATIC[]={
    5,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,CARTPOS_SIZE,
    sim_script_arg_charbuff,JOINTPOS_SIZE,
    sim_script_arg_string,0,
    sim_script_arg_charbuff,BITSTRING_SIZE,
};

void LUA_GET_INVERSE_KINEMATIC_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_INVERSE_KINEMATIC,inArgs_GET_INVERSE_KINEMATIC[0],LUA_GET_INVERSE_KINEMATIC_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(109,rcsHandle.c_str());

            std::string cartPos(getPartialString(inData->at(1).stringData[0],CARTPOS_SIZE));
            inputBlock.pushBackCartPos(cartPos);

            std::string jointPos(getPartialString(inData->at(2).stringData[0],JOINTPOS_SIZE));
            inputBlock.pushBackJointPos(jointPos);

            inputBlock.pushBackString(inData->at(3).stringData[0]);

            std::string outputFormat(getPartialString(inData->at(4).stringData[0],BITSTRING_SIZE));
            inputBlock.pushBackBitstring(outputFormat);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string jointPosOut(outputBlock.readJointPos());
                            D.pushOutData(CScriptFunctionDataItem(&jointPosOut[0],(int)jointPosOut.size()));

                            std::string jointLimits=outputBlock.readBitstring();
                            D.pushOutData(CScriptFunctionDataItem(&jointLimits[0],(int)jointLimits.size()));

                            int msgNb=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(msgNb));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_INVERSE_KINEMATIC_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_INVERSE_KINEMATIC_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_INVERSE_KINEMATIC_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_INVERSE_KINEMATIC_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_FORWARD_KINEMATIC
// --------------------------------------------------------------------------------------
#define LUA_GET_FORWARD_KINEMATIC_COMMAND "simRRS1.GET_FORWARD_KINEMATIC"
const int inArgs_GET_FORWARD_KINEMATIC[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,JOINTPOS_SIZE,
};

void LUA_GET_FORWARD_KINEMATIC_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_FORWARD_KINEMATIC,inArgs_GET_FORWARD_KINEMATIC[0],LUA_GET_FORWARD_KINEMATIC_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(110,rcsHandle.c_str());

            std::string jointPos(getPartialString(inData->at(1).stringData[0],JOINTPOS_SIZE));
            inputBlock.pushBackJointPos(jointPos);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string cartPosOut(outputBlock.readCartPos());
                            D.pushOutData(CScriptFunctionDataItem(&cartPosOut[0],(int)cartPosOut.size()));

                            std::string jointPosOut(outputBlock.readJointPos());
                            D.pushOutData(CScriptFunctionDataItem(&jointPosOut[0],(int)jointPosOut.size()));

                            std::string configuration=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(configuration));

                            std::string jointLimits=outputBlock.readBitstring();
                            D.pushOutData(CScriptFunctionDataItem(&jointLimits[0],(int)jointLimits.size()));

                            int msgNb=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(msgNb));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_FORWARD_KINEMATIC_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_FORWARD_KINEMATIC_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_FORWARD_KINEMATIC_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_FORWARD_KINEMATIC_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// MATRIX_TO_CONTROLLER_POSITION
// --------------------------------------------------------------------------------------
#define LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND "simRRS1.MATRIX_TO_CONTROLLER_POSITION"
const int inArgs_MATRIX_TO_CONTROLLER_POSITION[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,CARTPOS_SIZE,
    sim_script_arg_string,0,
};

void LUA_MATRIX_TO_CONTROLLER_POSITION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_MATRIX_TO_CONTROLLER_POSITION,inArgs_MATRIX_TO_CONTROLLER_POSITION[0],LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(111,rcsHandle.c_str());

            std::string cartPos(getPartialString(inData->at(1).stringData[0],CARTPOS_SIZE));
            inputBlock.pushBackCartPos(cartPos);

            inputBlock.pushBackString(inData->at(2).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string contrPos=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(contrPos));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CONTROLLER_POSITION_TO_MATRIX
// --------------------------------------------------------------------------------------
#define LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND "simRRS1.CONTROLLER_POSITION_TO_MATRIX"
const int inArgs_CONTROLLER_POSITION_TO_MATRIX[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_string,0,
};

void LUA_CONTROLLER_POSITION_TO_MATRIX_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_CONTROLLER_POSITION_TO_MATRIX,inArgs_CONTROLLER_POSITION_TO_MATRIX[0],LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(112,rcsHandle.c_str());

            inputBlock.pushBackString(inData->at(1).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string cartPos=outputBlock.readCartPos();
                            D.pushOutData(CScriptFunctionDataItem(&cartPos[0],(int)cartPos.size()));

                            std::string configuration=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(configuration));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_CELL_FRAME
// --------------------------------------------------------------------------------------
#define LUA_GET_CELL_FRAME_COMMAND "simRRS1.GET_CELL_FRAME"
const int inArgs_GET_CELL_FRAME[]={
    4,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
    sim_script_arg_string,0,
};

void LUA_GET_CELL_FRAME_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_CELL_FRAME,inArgs_GET_CELL_FRAME[0],LUA_GET_CELL_FRAME_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(113,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            inputBlock.pushBackString(inData->at(3).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string frameId=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(frameId));

                            int frameType=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(frameType));

                            std::string relativeToId=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(relativeToId));

                            std::string jointNumber=outputBlock.readBitstring();
                            D.pushOutData(CScriptFunctionDataItem(&jointNumber[0],(int)jointNumber.size()));

                            std::string frameData=outputBlock.readFrame();
                            D.pushOutData(CScriptFunctionDataItem(&frameData[0],(int)frameData.size()));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_CELL_FRAME_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_CELL_FRAME_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_CELL_FRAME_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_CELL_FRAME_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// MODIFY_CELL_FRAME
// --------------------------------------------------------------------------------------
#define LUA_MODIFY_CELL_FRAME_COMMAND "simRRS1.MODIFY_CELL_FRAME"
const int inArgs_MODIFY_CELL_FRAME[]={
    4,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_string,0,
    sim_script_arg_charbuff,FRAME_SIZE,
};

void LUA_MODIFY_CELL_FRAME_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_MODIFY_CELL_FRAME,inArgs_MODIFY_CELL_FRAME[0],LUA_MODIFY_CELL_FRAME_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(114,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackString(inData->at(2).stringData[0]);

            std::string frameData(getPartialString(inData->at(3).stringData[0],FRAME_SIZE));
            inputBlock.pushBackFrame(frameData);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_MODIFY_CELL_FRAME_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_MODIFY_CELL_FRAME_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_MODIFY_CELL_FRAME_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_MODIFY_CELL_FRAME_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_WORK_FRAMES
// --------------------------------------------------------------------------------------
#define LUA_SELECT_WORK_FRAMES_COMMAND "simRRS1.SELECT_WORK_FRAMES"
const int inArgs_SELECT_WORK_FRAMES[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_string,0,
    sim_script_arg_string,0,
};

void LUA_SELECT_WORK_FRAMES_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_WORK_FRAMES,inArgs_SELECT_WORK_FRAMES[0],LUA_SELECT_WORK_FRAMES_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(115,rcsHandle.c_str());

            inputBlock.pushBackString(inData->at(1).stringData[0]);

            inputBlock.pushBackString(inData->at(2).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_WORK_FRAMES_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_WORK_FRAMES_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_WORK_FRAMES_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_WORK_FRAMES_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_INITIAL_POSITION
// --------------------------------------------------------------------------------------
#define LUA_SET_INITIAL_POSITION_COMMAND "simRRS1.SET_INITIAL_POSITION"
const int inArgs_SET_INITIAL_POSITION[]={
    4,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,CARTPOS_SIZE,
    sim_script_arg_charbuff,JOINTPOS_SIZE,
    sim_script_arg_string,0,
};

void LUA_SET_INITIAL_POSITION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_INITIAL_POSITION,inArgs_SET_INITIAL_POSITION[0],LUA_SET_INITIAL_POSITION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(116,rcsHandle.c_str());

            std::string cartPosData(getPartialString(inData->at(1).stringData[0],CARTPOS_SIZE));
            inputBlock.pushBackCartPos(cartPosData);

            std::string jointPosData(getPartialString(inData->at(2).stringData[0],JOINTPOS_SIZE));
            inputBlock.pushBackJointPos(jointPosData);

            inputBlock.pushBackString(inData->at(3).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string jointLimit=outputBlock.readBitstring();
                            D.pushOutData(CScriptFunctionDataItem(&jointLimit[0],(int)jointLimit.size()));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_INITIAL_POSITION_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_INITIAL_POSITION_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_INITIAL_POSITION_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_INITIAL_POSITION_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_NEXT_TARGET
// --------------------------------------------------------------------------------------
#define LUA_SET_NEXT_TARGET_COMMAND "simRRS1.SET_NEXT_TARGET"
const int inArgs_SET_NEXT_TARGET[]={
    7,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
    sim_script_arg_charbuff,CARTPOS_SIZE,
    sim_script_arg_charbuff,JOINTPOS_SIZE,
    sim_script_arg_string,0,
    sim_script_arg_double,0,
};

void LUA_SET_NEXT_TARGET_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_NEXT_TARGET,inArgs_SET_NEXT_TARGET[0],LUA_SET_NEXT_TARGET_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(117,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            std::string cartPosData(getPartialString(inData->at(3).stringData[0],CARTPOS_SIZE));
            inputBlock.pushBackCartPos(cartPosData);

            std::string jointPosData(getPartialString(inData->at(4).stringData[0],JOINTPOS_SIZE));
            inputBlock.pushBackJointPos(jointPosData);

            inputBlock.pushBackString(inData->at(5).stringData[0]);

            inputBlock.pushBackReal(inData->at(6).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_NEXT_TARGET_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_NEXT_TARGET_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_NEXT_TARGET_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_NEXT_TARGET_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_NEXT_STEP
// --------------------------------------------------------------------------------------
#define LUA_GET_NEXT_STEP_COMMAND "simRRS1.GET_NEXT_STEP"
const int inArgs_GET_NEXT_STEP[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,BITSTRING_SIZE,
};

void LUA_GET_NEXT_STEP_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_NEXT_STEP,inArgs_GET_NEXT_STEP[0],LUA_GET_NEXT_STEP_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(118,rcsHandle.c_str());

            std::string outputFormat(getPartialString(inData->at(1).stringData[0],BITSTRING_SIZE));
            inputBlock.pushBackBitstring(outputFormat);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
               int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                       int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s>=12)
                        {
                            std::string cartPos=outputBlock.readCartPos();
                            D.pushOutData(CScriptFunctionDataItem(&cartPos[0],(int)cartPos.size()));

                            std::string jointPos=outputBlock.readJointPos();
                            D.pushOutData(CScriptFunctionDataItem(&jointPos[0],(int)jointPos.size()));

                            std::string configuration=outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(configuration));

                            double elapsedTime=outputBlock.readReal();
                            D.pushOutData(CScriptFunctionDataItem(elapsedTime));

                            std::string jointLimit=outputBlock.readBitstring();
                            D.pushOutData(CScriptFunctionDataItem(&jointLimit[0],(int)jointLimit.size()));

                            int numberOfEvents=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(numberOfEvents));

                            int numberOfMessages=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(numberOfMessages));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_NEXT_STEP_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_NEXT_STEP_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_NEXT_STEP_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_NEXT_STEP_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_INTERPOLATION_TIME
// --------------------------------------------------------------------------------------
#define LUA_SET_INTERPOLATION_TIME_COMMAND "simRRS1.SET_INTERPOLATION_TIME"
const int inArgs_SET_INTERPOLATION_TIME[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_double,0,
};

void LUA_SET_INTERPOLATION_TIME_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_INTERPOLATION_TIME,inArgs_SET_INTERPOLATION_TIME[0],LUA_SET_INTERPOLATION_TIME_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(119,rcsHandle.c_str());

            inputBlock.pushBackReal(inData->at(1).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_INTERPOLATION_TIME_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_INTERPOLATION_TIME_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_INTERPOLATION_TIME_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_INTERPOLATION_TIME_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_MOTION_TYPE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_MOTION_TYPE_COMMAND "simRRS1.SELECT_MOTION_TYPE"
const int inArgs_SELECT_MOTION_TYPE[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_SELECT_MOTION_TYPE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_MOTION_TYPE,inArgs_SELECT_MOTION_TYPE[0],LUA_SELECT_MOTION_TYPE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(120,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_MOTION_TYPE_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_MOTION_TYPE_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_MOTION_TYPE_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_MOTION_TYPE_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_TARGET_TYPE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_TARGET_TYPE_COMMAND "simRRS1.SELECT_TARGET_TYPE"
const int inArgs_SELECT_TARGET_TYPE[]={
    5,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_charbuff,CARTPOS_SIZE,
    sim_script_arg_charbuff,JOINTPOS_SIZE,
    sim_script_arg_string,0,
};

void LUA_SELECT_TARGET_TYPE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_TARGET_TYPE,inArgs_SELECT_TARGET_TYPE[0],LUA_SELECT_TARGET_TYPE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(121,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            std::string cartPosData(getPartialString(inData->at(2).stringData[0],CARTPOS_SIZE));
            inputBlock.pushBackCartPos(cartPosData);

            std::string jointPosData(getPartialString(inData->at(3).stringData[0],JOINTPOS_SIZE));
            inputBlock.pushBackJointPos(jointPosData);

            inputBlock.pushBackString(inData->at(4).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_TARGET_TYPE_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_TARGET_TYPE_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_TARGET_TYPE_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_TARGET_TYPE_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_TRAJECTORY_MODE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_TRAJECTORY_MODE_COMMAND "simRRS1.SELECT_TRAJECTORY_MODE"
const int inArgs_SELECT_TRAJECTORY_MODE[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_SELECT_TRAJECTORY_MODE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_TRAJECTORY_MODE,inArgs_SELECT_TRAJECTORY_MODE[0],LUA_SELECT_TRAJECTORY_MODE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(122,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_TRAJECTORY_MODE_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_TRAJECTORY_MODE_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_TRAJECTORY_MODE_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_TRAJECTORY_MODE_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_ORIENTATION_INTERPOLATION_MODE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND "simRRS1.SELECT_ORIENTATION_INTERPOLATION_MODE"
const int inArgs_SELECT_ORIENTATION_INTERPOLATION_MODE[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
};

void LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_ORIENTATION_INTERPOLATION_MODE,inArgs_SELECT_ORIENTATION_INTERPOLATION_MODE[0],LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(123,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_DOMINANT_INTERPOLATION
// --------------------------------------------------------------------------------------
#define LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND "simRRS1.SELECT_DOMINANT_INTERPOLATION"
const int inArgs_SELECT_DOMINANT_INTERPOLATION[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
};

void LUA_SELECT_DOMINANT_INTERPOLATION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_DOMINANT_INTERPOLATION,inArgs_SELECT_DOMINANT_INTERPOLATION[0],LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(124,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_ADVANCE_MOTION
// --------------------------------------------------------------------------------------
#define LUA_SET_ADVANCE_MOTION_COMMAND "simRRS1.SET_ADVANCE_MOTION"
const int inArgs_SET_ADVANCE_MOTION[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_SET_ADVANCE_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_ADVANCE_MOTION,inArgs_SET_ADVANCE_MOTION[0],LUA_SET_ADVANCE_MOTION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(127,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_ADVANCE_MOTION_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_ADVANCE_MOTION_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_ADVANCE_MOTION_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_ADVANCE_MOTION_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_MOTION_FILTER
// --------------------------------------------------------------------------------------
#define LUA_SET_MOTION_FILTER_COMMAND "simRRS1.SET_MOTION_FILTER"
const int inArgs_SET_MOTION_FILTER[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_SET_MOTION_FILTER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_MOTION_FILTER,inArgs_SET_MOTION_FILTER[0],LUA_SET_MOTION_FILTER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(128,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_MOTION_FILTER_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_MOTION_FILTER_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_MOTION_FILTER_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_MOTION_FILTER_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_OVERRIDE_POSITION
// --------------------------------------------------------------------------------------
#define LUA_SET_OVERRIDE_POSITION_COMMAND "simRRS1.SET_OVERRIDE_POSITION"
const int inArgs_SET_OVERRIDE_POSITION[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,FRAME_SIZE,
};

void LUA_SET_OVERRIDE_POSITION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_OVERRIDE_POSITION,inArgs_SET_OVERRIDE_POSITION[0],LUA_SET_OVERRIDE_POSITION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(129,rcsHandle.c_str());

            std::string posOffset(getPartialString(inData->at(1).stringData[0],FRAME_SIZE));
            inputBlock.pushBackFrame(posOffset);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_OVERRIDE_POSITION_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_OVERRIDE_POSITION_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_OVERRIDE_POSITION_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_OVERRIDE_POSITION_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// REVERSE_MOTION
// --------------------------------------------------------------------------------------
#define LUA_REVERSE_MOTION_COMMAND "simRRS1.REVERSE_MOTION"
const int inArgs_REVERSE_MOTION[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_double,0,
};

void LUA_REVERSE_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_REVERSE_MOTION,inArgs_REVERSE_MOTION[0],LUA_REVERSE_MOTION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(130,rcsHandle.c_str());

            inputBlock.pushBackReal(inData->at(1).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_REVERSE_MOTION_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_REVERSE_MOTION_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_REVERSE_MOTION_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_REVERSE_MOTION_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_PAYLOAD_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_PAYLOAD_PARAMETER_COMMAND "simRRS1.SET_PAYLOAD_PARAMETER"
const int inArgs_SET_PAYLOAD_PARAMETER[]={
    5,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_string,0,
    sim_script_arg_int32,0,
    sim_script_arg_double,0,
};

void LUA_SET_PAYLOAD_PARAMETER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_PAYLOAD_PARAMETER,inArgs_SET_PAYLOAD_PARAMETER[0],LUA_SET_PAYLOAD_PARAMETER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(160,rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackString(inData->at(2).stringData[0]);

            inputBlock.pushBackInteger(inData->at(3).int32Data[0]);

            inputBlock.pushBackReal(inData->at(4).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_PAYLOAD_PARAMETER_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_PAYLOAD_PARAMETER_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_PAYLOAD_PARAMETER_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_PAYLOAD_PARAMETER_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_TIME_COMPENSATION
// --------------------------------------------------------------------------------------
#define LUA_SELECT_TIME_COMPENSATION_COMMAND "simRRS1.SELECT_TIME_COMPENSATION"
const int inArgs_SELECT_TIME_COMPENSATION[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,BITSTRING_SIZE,
};

void LUA_SELECT_TIME_COMPENSATION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_TIME_COMPENSATION,inArgs_SELECT_TIME_COMPENSATION[0],LUA_SELECT_TIME_COMPENSATION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(165,rcsHandle.c_str());

            inputBlock.pushBackBitstring(inData->at(1).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_TIME_COMPENSATION_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_TIME_COMPENSATION_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_TIME_COMPENSATION_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_TIME_COMPENSATION_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CONFIGURATION_CONTROL
// --------------------------------------------------------------------------------------
#define LUA_SET_CONFIGURATION_CONTROL_COMMAND "simRRS1.SET_CONFIGURATION_CONTROL"
const int inArgs_SET_CONFIGURATION_CONTROL[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_string,0,
    sim_script_arg_string,0,
};

void LUA_SET_CONFIGURATION_CONTROL_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_CONFIGURATION_CONTROL,inArgs_SET_CONFIGURATION_CONTROL[0],LUA_SET_CONFIGURATION_CONTROL_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int scriptId=p->scriptID;
        int index=getServerIndexFromScriptId(scriptId);
        if (index!=-1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0],BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(161,rcsHandle.c_str());

            inputBlock.pushBackString(inData->at(1).stringData[0]);

            inputBlock.pushBackString(inData->at(2).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer=inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer,inputBlockSize))
            {
                int outputBlockSize;
                char* data=allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data!=0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data,outputBlockSize);
                    delete[] data;
                    if (outputBlockSize>=4)
                    {
                        int s=outputBlock.readInteger();
                        if (s>=8)
                        {
                            int status=outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_CONFIGURATION_CONTROL_COMMAND,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_CONFIGURATION_CONTROL_COMMAND,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_CONFIGURATION_CONTROL_COMMAND,"Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_CONFIGURATION_CONTROL_COMMAND,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_JOINT_SPEEDS
// --------------------------------------------------------------------------------------
#define LUA_SET_JOINT_SPEEDS_COMMAND "simRRS1.SET_JOINT_SPEEDS"
const int inArgs_SET_JOINT_SPEEDS[] = {
    4,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_charbuff, BITSTRING_SIZE,
    sim_script_arg_double|sim_script_arg_table, 32,
};

void LUA_SET_JOINT_SPEEDS_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_JOINT_SPEEDS, inArgs_SET_JOINT_SPEEDS[0], LUA_SET_JOINT_SPEEDS_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(131, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackBitstring(inData->at(2).stringData[0]);

            for (unsigned int i=0;i<32;i++)
                inputBlock.pushBackReal(inData->at(3).doubleData[i]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_JOINT_SPEEDS_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_JOINT_SPEEDS_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_JOINT_SPEEDS_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_JOINT_SPEEDS_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CARTESIAN_POSITION_SPEED
// --------------------------------------------------------------------------------------
#define LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND "simRRS1.SET_CARTESIAN_POSITION_SPEED"
const int inArgs_SET_CARTESIAN_POSITION_SPEED[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
};

void LUA_SET_CARTESIAN_POSITION_SPEED_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_CARTESIAN_POSITION_SPEED, inArgs_SET_CARTESIAN_POSITION_SPEED[0], LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(133, rcsHandle.c_str());

            inputBlock.pushBackReal(inData->at(1).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CARTESIAN_ORIENTATION_SPEED
// --------------------------------------------------------------------------------------
#define LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND "simRRS1.SET_CARTESIAN_ORIENTATION_SPEED"
const int inArgs_SET_CARTESIAN_ORIENTATION_SPEED[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
};

void LUA_SET_CARTESIAN_ORIENTATION_SPEED_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_CARTESIAN_ORIENTATION_SPEED, inArgs_SET_CARTESIAN_ORIENTATION_SPEED[0], LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(134, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackReal(inData->at(2).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_JOINT_ACCELERATIONS
// --------------------------------------------------------------------------------------
#define LUA_SET_JOINT_ACCELERATIONS_COMMAND "simRRS1.SET_JOINT_ACCELERATIONS"
const int inArgs_SET_JOINT_ACCELERATIONS[] = {
    5,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_charbuff, BITSTRING_SIZE,
    sim_script_arg_double|sim_script_arg_table, 32,
    sim_script_arg_int32, 0,
};

void LUA_SET_JOINT_ACCELERATIONS_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_JOINT_ACCELERATIONS, inArgs_SET_JOINT_ACCELERATIONS[0], LUA_SET_JOINT_ACCELERATIONS_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(135, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackBitstring(inData->at(2).stringData[0]);

            for (unsigned int i=0;i<32;i++)
                inputBlock.pushBackReal(inData->at(3).doubleData[i]);

            inputBlock.pushBackInteger(inData->at(4).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_JOINT_ACCELERATIONS_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_JOINT_ACCELERATIONS_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_JOINT_ACCELERATIONS_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_JOINT_ACCELERATIONS_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CARTESIAN_POSITION_ACCELERATION
// --------------------------------------------------------------------------------------
#define LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND "simRRS1.SET_CARTESIAN_POSITION_ACCELERATION"
const int inArgs_SET_CARTESIAN_POSITION_ACCELERATION[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
    sim_script_arg_int32, 0,
};

void LUA_SET_CARTESIAN_POSITION_ACCELERATION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_CARTESIAN_POSITION_ACCELERATION, inArgs_SET_CARTESIAN_POSITION_ACCELERATION[0], LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(137, rcsHandle.c_str());

            inputBlock.pushBackReal(inData->at(1).doubleData[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CARTESIAN_ORIENTATION_ACCELERATION
// --------------------------------------------------------------------------------------
#define LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND "simRRS1.SET_CARTESIAN_ORIENTATION_ACCELERATION"
const int inArgs_SET_CARTESIAN_ORIENTATION_ACCELERATION[] = {
    4,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
    sim_script_arg_int32, 0,
};

void LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_CARTESIAN_ORIENTATION_ACCELERATION, inArgs_SET_CARTESIAN_ORIENTATION_ACCELERATION[0], LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(138, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackReal(inData->at(2).doubleData[0]);

            inputBlock.pushBackInteger(inData->at(3).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_JOINT_JERKS
// --------------------------------------------------------------------------------------
#define LUA_SET_JOINT_JERKS_COMMAND "simRRS1.SET_JOINT_JERKS"
const int inArgs_SET_JOINT_JERKS[] = {
    5,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_charbuff, BITSTRING_SIZE,
    sim_script_arg_double|sim_script_arg_table, 32,
    sim_script_arg_int32, 0,
};

void LUA_SET_JOINT_JERKS_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_JOINT_JERKS, inArgs_SET_JOINT_JERKS[0], LUA_SET_JOINT_JERKS_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(162, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackBitstring(inData->at(2).stringData[0]);

            for (unsigned int i=0;i<32;i++)
                inputBlock.pushBackReal(inData->at(3).doubleData[i]);

            inputBlock.pushBackInteger(inData->at(4).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_JOINT_JERKS_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_JOINT_JERKS_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_JOINT_JERKS_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_JOINT_JERKS_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_MOTION_TIME
// --------------------------------------------------------------------------------------
#define LUA_SET_MOTION_TIME_COMMAND "simRRS1.SET_MOTION_TIME"
const int inArgs_SET_MOTION_TIME[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
};

void LUA_SET_MOTION_TIME_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_MOTION_TIME, inArgs_SET_MOTION_TIME[0], LUA_SET_MOTION_TIME_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(156, rcsHandle.c_str());

            inputBlock.pushBackReal(inData->at(1).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_MOTION_TIME_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_MOTION_TIME_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_MOTION_TIME_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_MOTION_TIME_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_OVERRIDE_SPEED
// --------------------------------------------------------------------------------------
#define LUA_SET_OVERRIDE_SPEED_COMMAND "simRRS1.SET_OVERRIDE_SPEED"
const int inArgs_SET_OVERRIDE_SPEED[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
    sim_script_arg_int32, 0,
};

void LUA_SET_OVERRIDE_SPEED_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_OVERRIDE_SPEED, inArgs_SET_OVERRIDE_SPEED[0], LUA_SET_OVERRIDE_SPEED_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(139, rcsHandle.c_str());

            inputBlock.pushBackReal(inData->at(1).doubleData[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_OVERRIDE_SPEED_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_OVERRIDE_SPEED_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_OVERRIDE_SPEED_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_OVERRIDE_SPEED_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_OVERRIDE_ACCELERATION
// --------------------------------------------------------------------------------------
#define LUA_SET_OVERRIDE_ACCELERATION_COMMAND "simRRS1.SET_OVERRIDE_ACCELERATION"
const int inArgs_SET_OVERRIDE_ACCELERATION[] = {
    4,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
    sim_script_arg_int32, 0,
    sim_script_arg_int32, 0,
};

void LUA_SET_OVERRIDE_ACCELERATION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_OVERRIDE_ACCELERATION, inArgs_SET_OVERRIDE_ACCELERATION[0], LUA_SET_OVERRIDE_ACCELERATION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(155, rcsHandle.c_str());

            inputBlock.pushBackReal(inData->at(1).doubleData[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(3).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_OVERRIDE_ACCELERATION_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_OVERRIDE_ACCELERATION_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_OVERRIDE_ACCELERATION_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_OVERRIDE_ACCELERATION_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_FLYBY_MODE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_FLYBY_MODE_COMMAND "simRRS1.SELECT_FLYBY_MODE"
const int inArgs_SELECT_FLYBY_MODE[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
};

void LUA_SELECT_FLYBY_MODE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_FLYBY_MODE, inArgs_SELECT_FLYBY_MODE[0], LUA_SELECT_FLYBY_MODE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(140, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_FLYBY_MODE_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_FLYBY_MODE_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_FLYBY_MODE_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_FLYBY_MODE_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_FLYBY_CRITERIA_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND "simRRS1.SET_FLYBY_CRITERIA_PARAMETER"
const int inArgs_SET_FLYBY_CRITERIA_PARAMETER[] = {
    4,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
};

void LUA_SET_FLYBY_CRITERIA_PARAMETER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_FLYBY_CRITERIA_PARAMETER, inArgs_SET_FLYBY_CRITERIA_PARAMETER[0], LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(141, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            inputBlock.pushBackReal(inData->at(3).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_FLYBY_CRITERIA
// --------------------------------------------------------------------------------------
#define LUA_SELECT_FLYBY_CRITERIA_COMMAND "simRRS1.SELECT_FLYBY_CRITERIA"
const int inArgs_SELECT_FLYBY_CRITERIA[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
};

void LUA_SELECT_FLYBY_CRITERIA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_FLYBY_CRITERIA, inArgs_SELECT_FLYBY_CRITERIA[0], LUA_SELECT_FLYBY_CRITERIA_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(142, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_FLYBY_CRITERIA_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_FLYBY_CRITERIA_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_FLYBY_CRITERIA_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_FLYBY_CRITERIA_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CANCEL_FLYBY_CRITERIA
// --------------------------------------------------------------------------------------
#define LUA_CANCEL_FLYBY_CRITERIA_COMMAND "simRRS1.CANCEL_FLYBY_CRITERIA"
const int inArgs_CANCEL_FLYBY_CRITERIA[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
};

void LUA_CANCEL_FLYBY_CRITERIA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_CANCEL_FLYBY_CRITERIA, inArgs_CANCEL_FLYBY_CRITERIA[0], LUA_CANCEL_FLYBY_CRITERIA_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(143, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_CANCEL_FLYBY_CRITERIA_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_CANCEL_FLYBY_CRITERIA_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_CANCEL_FLYBY_CRITERIA_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_CANCEL_FLYBY_CRITERIA_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_POINT_ACCURACY
// --------------------------------------------------------------------------------------
#define LUA_SELECT_POINT_ACCURACY_COMMAND "simRRS1.SELECT_POINT_ACCURACY"
const int inArgs_SELECT_POINT_ACCURACY[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
};

void LUA_SELECT_POINT_ACCURACY_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_POINT_ACCURACY, inArgs_SELECT_POINT_ACCURACY[0], LUA_SELECT_POINT_ACCURACY_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(144, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_POINT_ACCURACY_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_POINT_ACCURACY_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_POINT_ACCURACY_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_POINT_ACCURACY_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_POINT_ACCURACY_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND "simRRS1.SET_POINT_ACCURACY_PARAMETER"
const int inArgs_SET_POINT_ACCURACY_PARAMETER[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
};

void LUA_SET_POINT_ACCURACY_PARAMETER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_POINT_ACCURACY_PARAMETER, inArgs_SET_POINT_ACCURACY_PARAMETER[0], LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(145, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackReal(inData->at(2).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_REST_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_REST_PARAMETER_COMMAND "simRRS1.SET_REST_PARAMETER"
const int inArgs_SET_REST_PARAMETER[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
};

void LUA_SET_REST_PARAMETER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_REST_PARAMETER, inArgs_SET_REST_PARAMETER[0], LUA_SET_REST_PARAMETER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(164, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackReal(inData->at(2).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_REST_PARAMETER_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_REST_PARAMETER_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_REST_PARAMETER_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_REST_PARAMETER_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_CURRENT_TARGETID
// --------------------------------------------------------------------------------------
#define LUA_GET_CURRENT_TARGETID_COMMAND "simRRS1.GET_CURRENT_TARGETID"
const int inArgs_GET_CURRENT_TARGETID[] = {
    1,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
};

void LUA_GET_CURRENT_TARGETID_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_GET_CURRENT_TARGETID, inArgs_GET_CURRENT_TARGETID[0], LUA_GET_CURRENT_TARGETID_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(163, rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s >= 12)
                        {
                            int targetId = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(targetId));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_CURRENT_TARGETID_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_CURRENT_TARGETID_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_CURRENT_TARGETID_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_CURRENT_TARGETID_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_TRACKING
// --------------------------------------------------------------------------------------
#define LUA_SELECT_TRACKING_COMMAND "simRRS1.SELECT_TRACKING"
const int inArgs_SELECT_TRACKING[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_charbuff, BITSTRING_SIZE,
};

void LUA_SELECT_TRACKING_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_TRACKING, inArgs_SELECT_TRACKING[0], LUA_SELECT_TRACKING_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(146, rcsHandle.c_str());

            inputBlock.pushBackBitstring(inData->at(1).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_TRACKING_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_TRACKING_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_TRACKING_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_TRACKING_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CONVEYOR_POSITION
// --------------------------------------------------------------------------------------
#define LUA_SET_CONVEYOR_POSITION_COMMAND "simRRS1.SET_CONVEYOR_POSITION"
const int inArgs_SET_CONVEYOR_POSITION[] = {
    4,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_charbuff, BITSTRING_SIZE,
    sim_script_arg_charbuff, BITSTRING_SIZE,
    sim_script_arg_double|sim_script_arg_table, 32,
};

void LUA_SET_CONVEYOR_POSITION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_CONVEYOR_POSITION, inArgs_SET_CONVEYOR_POSITION[0], LUA_SET_CONVEYOR_POSITION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(147, rcsHandle.c_str());

            inputBlock.pushBackBitstring(inData->at(1).stringData[0]);

            inputBlock.pushBackBitstring(inData->at(2).stringData[0]);

            for (unsigned int i=0;i<32;i++)
                inputBlock.pushBackReal(inData->at(3).doubleData[i]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_CONVEYOR_POSITION_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_CONVEYOR_POSITION_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_CONVEYOR_POSITION_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_CONVEYOR_POSITION_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// DEFINE_EVENT
// --------------------------------------------------------------------------------------
#define LUA_DEFINE_EVENT_COMMAND "simRRS1.DEFINE_EVENT"
const int inArgs_DEFINE_EVENT[] = {
    6,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
    sim_script_arg_int32, 0,
    sim_script_arg_double|sim_script_arg_table, 16,
};

void LUA_DEFINE_EVENT_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_DEFINE_EVENT, inArgs_DEFINE_EVENT[0], LUA_DEFINE_EVENT_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(148, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            inputBlock.pushBackReal(inData->at(3).doubleData[0]);

            inputBlock.pushBackInteger(inData->at(4).int32Data[0]);

            for (unsigned int i=0;i<16;i++)
                inputBlock.pushBackReal(inData->at(5).doubleData[i]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_DEFINE_EVENT_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_DEFINE_EVENT_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_DEFINE_EVENT_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_DEFINE_EVENT_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CANCEL_EVENT
// --------------------------------------------------------------------------------------
#define LUA_CANCEL_EVENT_COMMAND "simRRS1.CANCEL_EVENT"
const int inArgs_CANCEL_EVENT[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
 };

void LUA_CANCEL_EVENT_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_CANCEL_EVENT, inArgs_CANCEL_EVENT[0], LUA_CANCEL_EVENT_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(149, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_CANCEL_EVENT_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_CANCEL_EVENT_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_CANCEL_EVENT_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_CANCEL_EVENT_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_EVENT
// --------------------------------------------------------------------------------------
#define LUA_GET_EVENT_COMMAND "simRRS1.GET_EVENT"
const int inArgs_GET_EVENT[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
 };

void LUA_GET_EVENT_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_GET_EVENT, inArgs_GET_EVENT[0], LUA_GET_EVENT_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(150, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s >= 12)
                        {
                            int eventId = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(eventId));

                            double timeTillEvent = outputBlock.readReal();
                            D.pushOutData(CScriptFunctionDataItem(timeTillEvent));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_EVENT_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_EVENT_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_EVENT_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_EVENT_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// STOP_MOTION
// --------------------------------------------------------------------------------------
#define LUA_STOP_MOTION_COMMAND "simRRS1.STOP_MOTION"
const int inArgs_STOP_MOTION[] = {
    1,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
 };

void LUA_STOP_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_STOP_MOTION, inArgs_STOP_MOTION[0], LUA_STOP_MOTION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(151, rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_STOP_MOTION_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_STOP_MOTION_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_STOP_MOTION_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_STOP_MOTION_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CONTINUE_MOTION
// --------------------------------------------------------------------------------------
#define LUA_CONTINUE_MOTION_COMMAND "simRRS1.CONTINUE_MOTION"
const int inArgs_CONTINUE_MOTION[] = {
    1,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
 };

void LUA_CONTINUE_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_CONTINUE_MOTION, inArgs_CONTINUE_MOTION[0], LUA_CONTINUE_MOTION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(152, rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_CONTINUE_MOTION_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_CONTINUE_MOTION_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_CONTINUE_MOTION_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_CONTINUE_MOTION_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CANCEL_MOTION
// --------------------------------------------------------------------------------------
#define LUA_CANCEL_MOTION_COMMAND "simRRS1.CANCEL_MOTION"
const int inArgs_CANCEL_MOTION[] = {
    1,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
 };

void LUA_CANCEL_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_CANCEL_MOTION, inArgs_CANCEL_MOTION[0], LUA_CANCEL_MOTION_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(153, rcsHandle.c_str());

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_CANCEL_MOTION_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_CANCEL_MOTION_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_CANCEL_MOTION_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_CANCEL_MOTION_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_MESSAGE
// --------------------------------------------------------------------------------------
#define LUA_GET_MESSAGE_COMMAND "simRRS1.GET_MESSAGE"
const int inArgs_GET_MESSAGE[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
 };

void LUA_GET_MESSAGE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_GET_MESSAGE, inArgs_GET_MESSAGE[0], LUA_GET_MESSAGE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(154, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }
                        if (s >= 12)
                        {
                            int severity = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(severity));

                            std::string text = outputBlock.readString();
                            D.pushOutData(CScriptFunctionDataItem(text));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_GET_MESSAGE_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_GET_MESSAGE_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_GET_MESSAGE_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_GET_MESSAGE_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_WEAVING_MODE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_WEAVING_MODE_COMMAND "simRRS1.SELECT_WEAVING_MODE"
const int inArgs_SELECT_WEAVING_MODE[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
 };

void LUA_SELECT_WEAVING_MODE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_WEAVING_MODE, inArgs_SELECT_WEAVING_MODE[0], LUA_SELECT_WEAVING_MODE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(157, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_WEAVING_MODE_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_WEAVING_MODE_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_WEAVING_MODE_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_WEAVING_MODE_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_WEAVING_GROUP
// --------------------------------------------------------------------------------------
#define LUA_SELECT_WEAVING_GROUP_COMMAND "simRRS1.SELECT_WEAVING_GROUP"
const int inArgs_SELECT_WEAVING_GROUP[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_int32, 0,
 };

void LUA_SELECT_WEAVING_GROUP_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_WEAVING_GROUP, inArgs_SELECT_WEAVING_GROUP[0], LUA_SELECT_WEAVING_GROUP_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(158, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SELECT_WEAVING_GROUP_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SELECT_WEAVING_GROUP_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SELECT_WEAVING_GROUP_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SELECT_WEAVING_GROUP_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_WEAVING_GROUP_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND "simRRS1.SET_WEAVING_GROUP_PARAMETER"
const int inArgs_SET_WEAVING_GROUP_PARAMETER[] = {
    4,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
 };

void LUA_SET_WEAVING_GROUP_PARAMETER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_WEAVING_GROUP_PARAMETER, inArgs_SET_WEAVING_GROUP_PARAMETER[0], LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(159, rcsHandle.c_str());

            inputBlock.pushBackInteger(inData->at(1).int32Data[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            inputBlock.pushBackReal(inData->at(3).doubleData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// DEBUG
// --------------------------------------------------------------------------------------
#define LUA_DEBUG_COMMAND "simRRS1.DEBUG"
const int inArgs_DEBUG[] = {
    4,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_charbuff, BITSTRING_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_string, 0,
 };

void LUA_DEBUG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_DEBUG, inArgs_DEBUG[0], LUA_DEBUG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(1000, rcsHandle.c_str());

            inputBlock.pushBackBitstring(inData->at(1).stringData[0]);

            inputBlock.pushBackInteger(inData->at(2).int32Data[0]);

            inputBlock.pushBackString(inData->at(3).stringData[0]);

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();
                        if (s >= 8)
                        {
                            int status = outputBlock.readInteger();
                            D.pushOutData(CScriptFunctionDataItem(status));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_DEBUG_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_DEBUG_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_DEBUG_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_DEBUG_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// EXTENDED_SERVICE
// --------------------------------------------------------------------------------------
#define LUA_EXTENDED_SERVICE_COMMAND "simRRS1.EXTENDED_SERVICE"
const int inArgs_EXTENDED_SERVICE[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_charbuff, 0, // can be any length
 };

void LUA_EXTENDED_SERVICE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_EXTENDED_SERVICE, inArgs_EXTENDED_SERVICE[0], LUA_EXTENDED_SERVICE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData = D.getInDataPtr();
        int scriptId = p->scriptID;
        int index = getServerIndexFromScriptId(scriptId);
        if (index != -1)
        {
            std::string rcsHandle(getPartialString(inData->at(0).stringData[0], BITSTRING2_SIZE));
            CInputOutputBlock inputBlock(1001, rcsHandle.c_str());

            inputBlock.pushBackBuffer(&inData->at(1).stringData[0][0],(int)inData->at(1).stringData[0].size());

            int inputBlockSize;
            void* inputBlockPointer = inputBlock.getDataPointer(&inputBlockSize);
            if (allRcsServers[index].connection->sendData((char*)inputBlockPointer, inputBlockSize))
            {
                int outputBlockSize;
                char* data = allRcsServers[index].connection->receiveReplyData(outputBlockSize);
                if (data != 0)
                {
                    CInputOutputBlock outputBlock((unsigned char*)data, outputBlockSize);
                    delete[] data;
                    if (outputBlockSize >= 4)
                    {
                        int s = outputBlock.readInteger();

                        if (s>8)
                        {
                            std::string outData=outputBlock.readBuffer(s-8);
                            D.pushOutData(CScriptFunctionDataItem(&outData[0],(int)outData.size()));
                        }

                        D.writeDataToStack(p->stackID);
                    }
                    else
                        simSetLastError(LUA_EXTENDED_SERVICE_COMMAND, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(LUA_EXTENDED_SERVICE_COMMAND, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(LUA_EXTENDED_SERVICE_COMMAND, "Failed sending data to the server.");
        }
        else
            simSetLastError(LUA_EXTENDED_SERVICE_COMMAND, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

VREP_DLLEXPORT unsigned char v_repStart(void* reservedPointer,int reservedInt)
{
    // 1. Figure out this plugin's directory:
    char curDirAndFile[1024];
#ifdef _WIN32
    #ifdef QT_COMPIL
        _getcwd(curDirAndFile, sizeof(curDirAndFile));
    #else
        GetModuleFileName(NULL,curDirAndFile,1023);
        PathRemoveFileSpec(curDirAndFile);
    #endif
#elif defined (__linux) || defined (__APPLE__)
    getcwd(curDirAndFile, sizeof(curDirAndFile));
#endif
    currentDirAndPath=curDirAndFile;

    // 2. Append the V-REP library's name:
    std::string temp(currentDirAndPath);
#ifdef _WIN32
    temp+="\\v_rep.dll";
#elif defined (__linux)
    temp+="/libv_rep.so";
#elif defined (__APPLE__)
    temp+="/libv_rep.dylib";
#endif 

    // 3. Load the V-REP library:
    vrepLib=loadVrepLibrary(temp.c_str());
    if (vrepLib==NULL)
    {
        std::cout << "Error, could not find or correctly load the V-REP library. Cannot start 'RRS1' plugin.\n";
        return(0); 
    }
    if (getVrepProcAddresses(vrepLib)==0)
    {
        std::cout << "Error, could not find all required functions in the V-REP library. Cannot start 'RRS1' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0);
    }

    // Check the version of V-REP:
    int vrepVer,vrepRev;
    simGetIntegerParameter(sim_intparam_program_version,&vrepVer);
    simGetIntegerParameter(sim_intparam_program_revision,&vrepRev);
    if( (vrepVer<30400) || ((vrepVer==30400)&&(vrepRev<9)) )
    {
        std::cout << "Sorry, your V-REP copy is somewhat old, V-REP 3.4.0 rev9 or higher is required. Cannot start 'RRS1' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0);
    }

    simRegisterScriptVariable("simRRS1","require('simExtRRS1')",0);

    // startRcsModule (auxiliary command)
    simRegisterScriptCallbackFunction(strConCat(LUA_START_RCS_SERVER_COMMAND,"@","RRS1"),strConCat("number rcsServerHandle=",LUA_START_RCS_SERVER_COMMAND,"(string rcsLibraryFilename,string rcsLibraryFunctionName,int portNumber)"),LUA_START_RCS_SERVER_CALLBACK);

    // selectRcsModule (auxiliary command)
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_RCS_SERVER_COMMAND,"@","RRS1"),strConCat("boolean result=",LUA_SELECT_RCS_SERVER_COMMAND,"(number rcsServerHandle)"),LUA_SELECT_RCS_SERVER_CALLBACK);

    // stopRcsModule (auxiliary command)
    simRegisterScriptCallbackFunction(strConCat(LUA_STOP_RCS_SERVER_COMMAND,"@","RRS1"),strConCat("boolean result=",LUA_STOP_RCS_SERVER_COMMAND,"(number rcsServerHandle)"),LUA_STOP_RCS_SERVER_CALLBACK);

    // INITIALIZE
    simRegisterScriptCallbackFunction(strConCat(LUA_INITIALIZE_COMMAND,"@","RRS1"),strConCat("int status,bitstring2 rcsHandle,int rcsRrsVersion,int rcsVersion,int numberOfMessages=",LUA_INITIALIZE_COMMAND,"(int robotNumber,string robotPathName,string modulePathName,string manipulatorType,int CarrrsVersion,int debug)"),LUA_INITIALIZE_CALLBACK);

    // RESET
    simRegisterScriptCallbackFunction(strConCat(LUA_RESET_COMMAND,"@","RRS1"),strConCat("int status,int numberOfMessages=",LUA_RESET_COMMAND,"(bitstring2 rcsHandle,int resetLevel)"),LUA_RESET_CALLBACK);

    // TERMINATE
    simRegisterScriptCallbackFunction(strConCat(LUA_TERMINATE_COMMAND,"@","RRS1"),strConCat("int status=",LUA_TERMINATE_COMMAND,"(bitstring2 rcsHandle)"),LUA_TERMINATE_CALLBACK);

    // GET_ROBOT_STAMP
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_ROBOT_STAMP_COMMAND,"@","RRS1"),strConCat("int status,string manipulator,string controller,string software=",LUA_GET_ROBOT_STAMP_COMMAND,"(bitstring2 rcsHandle)"),LUA_GET_ROBOT_STAMP_CALLBACK);

    // GET_HOME_JOINT_POSITION
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_HOME_JOINT_POSITION_COMMAND,"@","RRS1"),strConCat("int status,jointPosType homePosition=",LUA_GET_HOME_JOINT_POSITION_COMMAND,"(bitstring2 rcsHandle)"),LUA_GET_HOME_JOINT_POSITION_CALLBACK);

    // GET_RCS_DATA
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_RCS_DATA_COMMAND,"@","RRS1"),strConCat("int status,string paramId,string paramContents,int permission=",LUA_GET_RCS_DATA_COMMAND,"(bitstring2 rcsHandle,int storage,int firstNext,string paramId)"),LUA_GET_RCS_DATA_CALLBACK);

    // MODIFY_RCS_DATA
    simRegisterScriptCallbackFunction(strConCat(LUA_MODIFY_RCS_DATA_COMMAND,"@","RRS1"),strConCat("int status=",LUA_MODIFY_RCS_DATA_COMMAND,"(bitstring2 rcsHandle,int storage,string paramId,string paramContents)"),LUA_MODIFY_RCS_DATA_CALLBACK);

    // SAVE_RCS_DATA
    simRegisterScriptCallbackFunction(strConCat(LUA_SAVE_RCS_DATA_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SAVE_RCS_DATA_COMMAND,"(bitstring2 rcsHandle)"),LUA_SAVE_RCS_DATA_CALLBACK);

    // LOAD_RCS_DATA
    simRegisterScriptCallbackFunction(strConCat(LUA_LOAD_RCS_DATA_COMMAND,"@","RRS1"),strConCat("int status,int numberOfMessages=",LUA_LOAD_RCS_DATA_COMMAND,"(bitstring2 rcsHandle)"),LUA_LOAD_RCS_DATA_CALLBACK);

    // GET_INVERSE_KINEMATIC
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_INVERSE_KINEMATIC_COMMAND,"@","RRS1"),strConCat("int status,jointPosType jointPos,bitString jointLimit,int numberOfMessages=",LUA_GET_INVERSE_KINEMATIC_COMMAND,"(bitstring2 rcsHandle,cartPosType cartPos,jointPosType jointPos,string configuration,bitstring outputFormat)"),LUA_GET_INVERSE_KINEMATIC_CALLBACK);

    // GET_FORWARD_KINEMATIC
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_FORWARD_KINEMATIC_COMMAND,"@","RRS1"),strConCat("int status,cartPosType cartPos,jointPosType jointPos,string configuration,bitString jointLimit,int numberOfMessages=",LUA_GET_FORWARD_KINEMATIC_COMMAND,"(bitstring2 rcsHandle,jointPosType jointPos)"),LUA_GET_FORWARD_KINEMATIC_CALLBACK);

    // MATRIX_TO_CONTROLLER_POSITION
    simRegisterScriptCallbackFunction(strConCat(LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND,"@","RRS1"),strConCat("int status,string contrPos=",LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND,"(bitstring2 rcsHandle,cartPosType cartPos,string configuration)"),LUA_MATRIX_TO_CONTROLLER_POSITION_CALLBACK);

    // CONTROLLER_POSITION_TO_MATRIX
    simRegisterScriptCallbackFunction(strConCat(LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND,"@","RRS1"),strConCat("int status,cartPosType cartPos,string configuration=",LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND,"(bitstring2 rcsHandle,string contrPos)"),LUA_CONTROLLER_POSITION_TO_MATRIX_CALLBACK);

    // GET_CELL_FRAME
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_CELL_FRAME_COMMAND,"@","RRS1"),strConCat("int status,string frameId,int frameType,string relativeToId,bitstring jointNumber,frame frameData=",LUA_GET_CELL_FRAME_COMMAND,"(bitstring2 rcsHandle,int storage,int firstNext,string frameId)"),LUA_GET_CELL_FRAME_CALLBACK);

    // MODIFY_CELL_FRAME
    simRegisterScriptCallbackFunction(strConCat(LUA_MODIFY_CELL_FRAME_COMMAND,"@","RRS1"),strConCat("int status=",LUA_MODIFY_CELL_FRAME_COMMAND,"(bitstring2 rcsHandle,int storage,string frameId,frame frameData)"),LUA_MODIFY_CELL_FRAME_CALLBACK);

    // SELECT_WORK_FRAMES
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_WORK_FRAMES_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_WORK_FRAMES_COMMAND,"(bitstring2 rcsHandle,string toolId,string objectId)"),LUA_SELECT_WORK_FRAMES_CALLBACK);

    // SET_INITIAL_POSITION
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_INITIAL_POSITION_COMMAND,"@","RRS1"),strConCat("int status,bitstring jointLimit=",LUA_SET_INITIAL_POSITION_COMMAND,"(bitstring2 rcsHandle,cartPosType cartPos,jointPosType jointPos,string configuration)"),LUA_SET_INITIAL_POSITION_CALLBACK);

    // SET_NEXT_TARGET
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_NEXT_TARGET_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_NEXT_TARGET_COMMAND,"(bitstring2 rcsHandle,int targetId,int targetParam,cartPosType cartPos,jointPosType jointPos,string configuration,real targetParamValue)"),LUA_SET_NEXT_TARGET_CALLBACK);

    // GET_NEXT_STEP
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_NEXT_STEP_COMMAND,"@","RRS1"),strConCat("int status,cartPosType cartPos,jointPosType jointPos,string configuration,real elapsedTime,bitstring jointLimit,int numberOfEvents,int numberOfMessages=",LUA_GET_NEXT_STEP_COMMAND,"(bitstring2 rcsHandle,bitstring outputFormat)"),LUA_GET_NEXT_STEP_CALLBACK);

    // SET_INTERPOLATION_TIME
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_INTERPOLATION_TIME_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_INTERPOLATION_TIME_COMMAND,"(bitstring2 rcsHandle,real interpolationTime)"),LUA_SET_INTERPOLATION_TIME_CALLBACK);

    // SELECT_MOTION_TYPE
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_MOTION_TYPE_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_MOTION_TYPE_COMMAND,"(bitstring2 rcsHandle,int motionType)"),LUA_SELECT_MOTION_TYPE_CALLBACK);

    // SELECT_TARGET_TYPE
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_TARGET_TYPE_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_TARGET_TYPE_COMMAND,"(bitstring2 rcsHandle,int targetType,cartPosType cartPos,jointPosType jointPos,string configuration)"),LUA_SELECT_TARGET_TYPE_CALLBACK);

    // SELECT_TRAJECTORY_MODE
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_TRAJECTORY_MODE_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_TRAJECTORY_MODE_COMMAND,"(bitstring2 rcsHandle,int trajectoryOn)"),LUA_SELECT_TRAJECTORY_MODE_CALLBACK);

    // SELECT_ORIENTATION_INTERPOLATION_MODE
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND,"(bitstring2 rcsHandle,int interpolationMode,int oriConst)"),LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_CALLBACK);

    // SELECT_DOMINANT_INTERPOLATION
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND,"(bitstring2 rcsHandle,int dominantIntType,int dominantIntParam)"),LUA_SELECT_DOMINANT_INTERPOLATION_CALLBACK);

    // SET_ADVANCE_MOTION
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_ADVANCE_MOTION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_ADVANCE_MOTION_COMMAND,"(bitstring2 rcsHandle,int numberOfMotion)"),LUA_SET_ADVANCE_MOTION_CALLBACK);

    // SET_MOTION_FILTER
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_MOTION_FILTER_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_MOTION_FILTER_COMMAND,"(bitstring2 rcsHandle,int filterFactor)"),LUA_SET_MOTION_FILTER_CALLBACK);

    // SET_OVERRIDE_POSITION
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_OVERRIDE_POSITION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_OVERRIDE_POSITION_COMMAND,"(bitstring2 rcsHandle,frame posOffset)"),LUA_SET_OVERRIDE_POSITION_CALLBACK);

    // REVERSE_MOTION
    simRegisterScriptCallbackFunction(strConCat(LUA_REVERSE_MOTION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_REVERSE_MOTION_COMMAND,"(bitstring2 rcsHandle,real distance)"),LUA_REVERSE_MOTION_CALLBACK);

    // SET_PAYLOAD_PARAMETER
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_PAYLOAD_PARAMETER_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_PAYLOAD_PARAMETER_COMMAND,"(bitstring2 rcsHandle,int storage,string frameId,int paramNumber,real paramValue)"),LUA_SET_PAYLOAD_PARAMETER_CALLBACK);

    // SELECT_TIME_COMPENSATION
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_TIME_COMPENSATION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_TIME_COMPENSATION_COMMAND,"(bitstring2 rcsHandle,bitstring compensation)"),LUA_SELECT_TIME_COMPENSATION_CALLBACK);

    // SET_CONFIGURATION_CONTROL
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CONFIGURATION_CONTROL_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_CONFIGURATION_CONTROL_COMMAND,"(bitstring2 rcsHandle,string paramId,string paramContents)"),LUA_SET_CONFIGURATION_CONTROL_CALLBACK);

    // SET_JOINT_SPEEDS
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_JOINT_SPEEDS_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_JOINT_SPEEDS_COMMAND,"(bitstring2 rcsHandle,int allJointFlags,bitstring jointFlags,real_32 speedPercent)"),LUA_SET_JOINT_SPEEDS_CALLBACK);

    // SET_CARTESIAN_POSITION_SPEED
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND,"(bitstring2 rcsHandle,real speedValue)"),LUA_SET_CARTESIAN_POSITION_SPEED_CALLBACK);

    // SET_CARTESIAN_ORIENTATION_SPEED
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND,"(bitstring2 rcsHandle,int rotationNo,real speedValue)"),LUA_SET_CARTESIAN_ORIENTATION_SPEED_CALLBACK);

    // SET_JOINT_ACCELERATIONS
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_JOINT_ACCELERATIONS_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_JOINT_ACCELERATIONS_COMMAND,"(bitstring2 rcsHandle,int allJointFlags,bitstring jointFlags,real_32 accelPercent,int accelType)"),LUA_SET_JOINT_ACCELERATIONS_CALLBACK);

    // SET_CARTESIAN_POSITION_ACCELERATION
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND,"(bitstring2 rcsHandle,real accelValue,int accelType)"),LUA_SET_CARTESIAN_POSITION_ACCELERATION_CALLBACK);

    // SET_CARTESIAN_ORIENTATION_ACCELERATION
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND,"(bitstring2 rcsHandle,int rotationNo,real accelValue,int accelType)"),LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_CALLBACK);

    // SET_JOINT_JERKS
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_JOINT_JERKS_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_JOINT_JERKS_COMMAND,"(bitstring2 rcsHandle,int allJointFlags,bitstring jointFlags,real_32 jerkPercent,int jerkType)"),LUA_SET_JOINT_JERKS_CALLBACK);

    // SET_MOTION_TIME
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_MOTION_TIME_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_MOTION_TIME_COMMAND,"(bitstring2 rcsHandle,real timeValue)"),LUA_SET_MOTION_TIME_CALLBACK);

    // SET_OVERRIDE_SPEED
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_OVERRIDE_SPEED_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_OVERRIDE_SPEED_COMMAND,"(bitstring2 rcsHandle,real correctionValue,int correctionType)"),LUA_SET_OVERRIDE_SPEED_CALLBACK);

    // SET_OVERRIDE_ACCELERATION
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_OVERRIDE_ACCELERATION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_OVERRIDE_ACCELERATION_COMMAND,"(bitstring2 rcsHandle,real correctionValue,int accelType,int correctionType)"),LUA_SET_OVERRIDE_ACCELERATION_CALLBACK);

    // SELECT_FLYBY_MODE
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_FLYBY_MODE_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_FLYBY_MODE_COMMAND,"(bitstring2 rcsHandle,int flyByOn)"),LUA_SELECT_FLYBY_MODE_CALLBACK);

    // SET_FLYBY_CRITERIA_PARAMETER
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND,"(bitstring2 rcsHandle,int paramNumber,int jointNr,real paramValue)"),LUA_SET_FLYBY_CRITERIA_PARAMETER_CALLBACK);

    // SELECT_FLYBY_CRITERIA
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_FLYBY_CRITERIA_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_FLYBY_CRITERIA_COMMAND,"(bitstring2 rcsHandle,int paramNumber)"),LUA_SELECT_FLYBY_CRITERIA_CALLBACK);

    // CANCEL_FLYBY_CRITERIA
    simRegisterScriptCallbackFunction(strConCat(LUA_CANCEL_FLYBY_CRITERIA_COMMAND,"@","RRS1"),strConCat("int status=",LUA_CANCEL_FLYBY_CRITERIA_COMMAND,"(bitstring2 rcsHandle,int paramNumber)"),LUA_CANCEL_FLYBY_CRITERIA_CALLBACK);

    // SELECT_POINT_ACCURACY
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_POINT_ACCURACY_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_POINT_ACCURACY_COMMAND,"(bitstring2 rcsHandle,int accuracyType)"),LUA_SELECT_POINT_ACCURACY_CALLBACK);

    // SET_POINT_ACCURACY_PARAMETER
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND,"(bitstring2 rcsHandle,int accuracyType,real accuracyValue)"),LUA_SET_POINT_ACCURACY_PARAMETER_CALLBACK);

    // SET_REST_PARAMETER
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_REST_PARAMETER_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_REST_PARAMETER_COMMAND,"(bitstring2 rcsHandle,int paramNumber,real paramValue)"),LUA_SET_REST_PARAMETER_CALLBACK);

    // GET_CURRENT_TARGETID
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_CURRENT_TARGETID_COMMAND,"@","RRS1"),strConCat("int status,int targetId=",LUA_GET_CURRENT_TARGETID_COMMAND,"(bitstring2 rcsHandle)"),LUA_GET_CURRENT_TARGETID_CALLBACK);

    // SELECT_TRACKING
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_TRACKING_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_TRACKING_COMMAND,"(bitstring2 rcsHandle,bitstring conveyorFlags)"),LUA_SELECT_TRACKING_CALLBACK);

    // SET_CONVEYOR_POSITION
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CONVEYOR_POSITION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_CONVEYOR_POSITION_COMMAND,"(bitstring2 rcsHandle,bitstring inputFormat,bitstring conveyorFlags,real_32 conveyorPos)"),LUA_SET_CONVEYOR_POSITION_CALLBACK);

    // DEFINE_EVENT
    simRegisterScriptCallbackFunction(strConCat(LUA_DEFINE_EVENT_COMMAND,"@","RRS1"),strConCat("int status=",LUA_DEFINE_EVENT_COMMAND,"(bitstring2 rcsHandle,int eventId,int targetId,real resolution,int typeOfEvent,real_16 eventSpec)"),LUA_DEFINE_EVENT_CALLBACK);

    // CANCEL_EVENT
    simRegisterScriptCallbackFunction(strConCat(LUA_CANCEL_EVENT_COMMAND,"@","RRS1"),strConCat("int status=",LUA_CANCEL_EVENT_COMMAND,"(bitstring2 rcsHandle,int eventId)"),LUA_CANCEL_EVENT_CALLBACK);

    // GET_EVENT
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_EVENT_COMMAND,"@","RRS1"),strConCat("int status,int eventId,real timeTillEvent=",LUA_GET_EVENT_COMMAND,"(bitstring2 rcsHandle,int eventNumber)"),LUA_GET_EVENT_CALLBACK);

    // STOP_MOTION
    simRegisterScriptCallbackFunction(strConCat(LUA_STOP_MOTION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_STOP_MOTION_COMMAND,"(bitstring2 rcsHandle)"),LUA_STOP_MOTION_CALLBACK);

    // CONTINUE_MOTION
    simRegisterScriptCallbackFunction(strConCat(LUA_CONTINUE_MOTION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_CONTINUE_MOTION_COMMAND,"(bitstring2 rcsHandle)"),LUA_CONTINUE_MOTION_CALLBACK);

    // CANCEL_MOTION
    simRegisterScriptCallbackFunction(strConCat(LUA_CANCEL_MOTION_COMMAND,"@","RRS1"),strConCat("int status=",LUA_CANCEL_MOTION_COMMAND,"(bitstring2 rcsHandle)"),LUA_CANCEL_MOTION_CALLBACK);

    // GET_MESSAGE
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_MESSAGE_COMMAND,"@","RRS1"),strConCat("int status,int severity,string text=",LUA_GET_MESSAGE_COMMAND,"(bitstring2 rcsHandle,int messageNumber)"),LUA_GET_MESSAGE_CALLBACK);

    // SELECT_WEAVING_MODE
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_WEAVING_MODE_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_WEAVING_MODE_COMMAND,"(bitstring2 rcsHandle,int weavingMode)"),LUA_SELECT_WEAVING_MODE_CALLBACK);

    // SELECT_WEAVING_GROUP
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_WEAVING_GROUP_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SELECT_WEAVING_GROUP_COMMAND,"(bitstring2 rcsHandle,int groupNo,int groupOn)"),LUA_SELECT_WEAVING_GROUP_CALLBACK);

    // SET_WEAVING_GROUP_PARAMETER
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND,"@","RRS1"),strConCat("int status=",LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND,"(bitstring2 rcsHandle,int groupNo,int paramNo,real paramValue)"),LUA_SET_WEAVING_GROUP_PARAMETER_CALLBACK);

    // DEBUG
    simRegisterScriptCallbackFunction(strConCat(LUA_DEBUG_COMMAND,"@","RRS1"),strConCat("int status=",LUA_DEBUG_COMMAND,"(bitstring2 rcsHandle,bitstring debugFlags,int opcodeSelect,string logFileName)"),LUA_DEBUG_CALLBACK);

    // EXTENDED_SERVICE
    simRegisterScriptCallbackFunction(strConCat(LUA_EXTENDED_SERVICE_COMMAND,"@","RRS1"),strConCat("int status,string outData=",LUA_EXTENDED_SERVICE_COMMAND,"(bitstring2 rcsHandle,string inData)"),LUA_EXTENDED_SERVICE_CALLBACK);

    // Following for backward compatibility:
    simRegisterScriptVariable(LUA_START_RCS_SERVER_COMMANDOLD,LUA_START_RCS_SERVER_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_RCS_SERVER_COMMANDOLD,LUA_SELECT_RCS_SERVER_COMMAND,-1);
    simRegisterScriptVariable(LUA_STOP_RCS_SERVER_COMMANDOLD,LUA_STOP_RCS_SERVER_COMMAND,-1);
    simRegisterScriptVariable(LUA_INITIALIZE_COMMANDOLD,LUA_INITIALIZE_COMMAND,-1);
    simRegisterScriptVariable(LUA_RESET_COMMANDOLD,LUA_RESET_COMMAND,-1);
    simRegisterScriptVariable(LUA_TERMINATE_COMMANDOLD,LUA_TERMINATE_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_ROBOT_STAMP_COMMANDOLD,LUA_GET_ROBOT_STAMP_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_HOME_JOINT_POSITION_COMMANDOLD,LUA_GET_HOME_JOINT_POSITION_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_RCS_DATA_COMMANDOLD,LUA_GET_RCS_DATA_COMMAND,-1);
    simRegisterScriptVariable(LUA_MODIFY_RCS_DATA_COMMANDOLD,LUA_MODIFY_RCS_DATA_COMMAND,-1);
    simRegisterScriptVariable(LUA_SAVE_RCS_DATA_COMMANDOLD,LUA_SAVE_RCS_DATA_COMMAND,-1);
    simRegisterScriptVariable(LUA_LOAD_RCS_DATA_COMMANDOLD,LUA_LOAD_RCS_DATA_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_INVERSE_KINEMATIC_COMMANDOLD,LUA_GET_INVERSE_KINEMATIC_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_FORWARD_KINEMATIC_COMMANDOLD,LUA_GET_FORWARD_KINEMATIC_COMMAND,-1);
    simRegisterScriptVariable(LUA_MATRIX_TO_CONTROLLER_POSITION_COMMANDOLD,LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND,-1);
    simRegisterScriptVariable(LUA_CONTROLLER_POSITION_TO_MATRIX_COMMANDOLD,LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_CELL_FRAME_COMMANDOLD,LUA_GET_CELL_FRAME_COMMAND,-1);
    simRegisterScriptVariable(LUA_MODIFY_CELL_FRAME_COMMANDOLD,LUA_MODIFY_CELL_FRAME_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_WORK_FRAMES_COMMANDOLD,LUA_SELECT_WORK_FRAMES_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_INITIAL_POSITION_COMMANDOLD,LUA_SET_INITIAL_POSITION_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_NEXT_TARGET_COMMANDOLD,LUA_SET_NEXT_TARGET_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_NEXT_STEP_COMMANDOLD,LUA_GET_NEXT_STEP_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_INTERPOLATION_TIME_COMMANDOLD,LUA_SET_INTERPOLATION_TIME_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_MOTION_TYPE_COMMANDOLD,LUA_SELECT_MOTION_TYPE_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_TARGET_TYPE_COMMANDOLD,LUA_SELECT_TARGET_TYPE_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_TRAJECTORY_MODE_COMMANDOLD,LUA_SELECT_TRAJECTORY_MODE_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMANDOLD,LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_DOMINANT_INTERPOLATION_COMMANDOLD,LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_ADVANCE_MOTION_COMMANDOLD,LUA_SET_ADVANCE_MOTION_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_MOTION_FILTER_COMMANDOLD,LUA_SET_MOTION_FILTER_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_OVERRIDE_POSITION_COMMANDOLD,LUA_SET_OVERRIDE_POSITION_COMMAND,-1);
    simRegisterScriptVariable(LUA_REVERSE_MOTION_COMMANDOLD,LUA_REVERSE_MOTION_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_PAYLOAD_PARAMETER_COMMANDOLD,LUA_SET_PAYLOAD_PARAMETER_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_TIME_COMPENSATION_COMMANDOLD,LUA_SELECT_TIME_COMPENSATION_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_CONFIGURATION_CONTROL_COMMANDOLD,LUA_SET_CONFIGURATION_CONTROL_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_JOINT_SPEEDS_COMMANDOLD,LUA_SET_JOINT_SPEEDS_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_CARTESIAN_POSITION_SPEED_COMMANDOLD,LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMANDOLD,LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_JOINT_ACCELERATIONS_COMMANDOLD,LUA_SET_JOINT_ACCELERATIONS_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMANDOLD,LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMANDOLD,LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_JOINT_JERKS_COMMANDOLD,LUA_SET_JOINT_JERKS_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_MOTION_TIME_COMMANDOLD,LUA_SET_MOTION_TIME_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_OVERRIDE_SPEED_COMMANDOLD,LUA_SET_OVERRIDE_SPEED_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_OVERRIDE_ACCELERATION_COMMANDOLD,LUA_SET_OVERRIDE_ACCELERATION_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_FLYBY_MODE_COMMANDOLD,LUA_SELECT_FLYBY_MODE_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMANDOLD,LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_FLYBY_CRITERIA_COMMANDOLD,LUA_SELECT_FLYBY_CRITERIA_COMMAND,-1);
    simRegisterScriptVariable(LUA_CANCEL_FLYBY_CRITERIA_COMMANDOLD,LUA_CANCEL_FLYBY_CRITERIA_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_POINT_ACCURACY_COMMANDOLD,LUA_SELECT_POINT_ACCURACY_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_POINT_ACCURACY_PARAMETER_COMMANDOLD,LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_REST_PARAMETER_COMMANDOLD,LUA_SET_REST_PARAMETER_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_CURRENT_TARGETID_COMMANDOLD,LUA_GET_CURRENT_TARGETID_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_TRACKING_COMMANDOLD,LUA_SELECT_TRACKING_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_CONVEYOR_POSITION_COMMANDOLD,LUA_SET_CONVEYOR_POSITION_COMMAND,-1);
    simRegisterScriptVariable(LUA_DEFINE_EVENT_COMMANDOLD,LUA_DEFINE_EVENT_COMMAND,-1);
    simRegisterScriptVariable(LUA_CANCEL_EVENT_COMMANDOLD,LUA_CANCEL_EVENT_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_EVENT_COMMANDOLD,LUA_GET_EVENT_COMMAND,-1);
    simRegisterScriptVariable(LUA_STOP_MOTION_COMMANDOLD,LUA_STOP_MOTION_COMMAND,-1);
    simRegisterScriptVariable(LUA_CONTINUE_MOTION_COMMANDOLD,LUA_CONTINUE_MOTION_COMMAND,-1);
    simRegisterScriptVariable(LUA_CANCEL_MOTION_COMMANDOLD,LUA_CANCEL_MOTION_COMMAND,-1);
    simRegisterScriptVariable(LUA_GET_MESSAGE_COMMANDOLD,LUA_GET_MESSAGE_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_WEAVING_MODE_COMMANDOLD,LUA_SELECT_WEAVING_MODE_COMMAND,-1);
    simRegisterScriptVariable(LUA_SELECT_WEAVING_GROUP_COMMANDOLD,LUA_SELECT_WEAVING_GROUP_COMMAND,-1);
    simRegisterScriptVariable(LUA_SET_WEAVING_GROUP_PARAMETER_COMMANDOLD,LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND,-1);
    simRegisterScriptVariable(LUA_DEBUG_COMMANDOLD,LUA_DEBUG_COMMAND,-1);
    simRegisterScriptVariable(LUA_EXTENDED_SERVICE_COMMANDOLD,LUA_EXTENDED_SERVICE_COMMAND,-1);
    simRegisterScriptCallbackFunction(strConCat(LUA_START_RCS_SERVER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_START_RCS_SERVER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_RCS_SERVER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_RCS_SERVER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_STOP_RCS_SERVER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_STOP_RCS_SERVER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_INITIALIZE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_INITIALIZE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_RESET_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_RESET_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_TERMINATE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_TERMINATE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_ROBOT_STAMP_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_ROBOT_STAMP_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_HOME_JOINT_POSITION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_HOME_JOINT_POSITION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_RCS_DATA_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_RCS_DATA_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_MODIFY_RCS_DATA_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_MODIFY_RCS_DATA_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SAVE_RCS_DATA_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SAVE_RCS_DATA_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_LOAD_RCS_DATA_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_LOAD_RCS_DATA_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_INVERSE_KINEMATIC_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_INVERSE_KINEMATIC_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_FORWARD_KINEMATIC_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_FORWARD_KINEMATIC_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_MATRIX_TO_CONTROLLER_POSITION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_CONTROLLER_POSITION_TO_MATRIX_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_CELL_FRAME_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_CELL_FRAME_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_MODIFY_CELL_FRAME_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_MODIFY_CELL_FRAME_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_WORK_FRAMES_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_WORK_FRAMES_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_INITIAL_POSITION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_INITIAL_POSITION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_NEXT_TARGET_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_NEXT_TARGET_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_NEXT_STEP_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_NEXT_STEP_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_INTERPOLATION_TIME_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_INTERPOLATION_TIME_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_MOTION_TYPE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_MOTION_TYPE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_TARGET_TYPE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_TARGET_TYPE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_TRAJECTORY_MODE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_TRAJECTORY_MODE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_DOMINANT_INTERPOLATION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_ADVANCE_MOTION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_ADVANCE_MOTION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_MOTION_FILTER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_MOTION_FILTER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_OVERRIDE_POSITION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_OVERRIDE_POSITION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_REVERSE_MOTION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_REVERSE_MOTION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_PAYLOAD_PARAMETER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_PAYLOAD_PARAMETER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_TIME_COMPENSATION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_TIME_COMPENSATION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CONFIGURATION_CONTROL_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_CONFIGURATION_CONTROL_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_JOINT_SPEEDS_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_JOINT_SPEEDS_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CARTESIAN_POSITION_SPEED_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_JOINT_ACCELERATIONS_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_JOINT_ACCELERATIONS_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_JOINT_JERKS_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_JOINT_JERKS_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_MOTION_TIME_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_MOTION_TIME_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_OVERRIDE_SPEED_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_OVERRIDE_SPEED_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_OVERRIDE_ACCELERATION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_OVERRIDE_ACCELERATION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_FLYBY_MODE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_FLYBY_MODE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_FLYBY_CRITERIA_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_FLYBY_CRITERIA_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_CANCEL_FLYBY_CRITERIA_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_CANCEL_FLYBY_CRITERIA_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_POINT_ACCURACY_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_POINT_ACCURACY_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_POINT_ACCURACY_PARAMETER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_REST_PARAMETER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_REST_PARAMETER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_CURRENT_TARGETID_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_CURRENT_TARGETID_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_TRACKING_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_TRACKING_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_CONVEYOR_POSITION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_CONVEYOR_POSITION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_DEFINE_EVENT_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_DEFINE_EVENT_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_CANCEL_EVENT_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_CANCEL_EVENT_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_EVENT_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_EVENT_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_STOP_MOTION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_STOP_MOTION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_CONTINUE_MOTION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_CONTINUE_MOTION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_CANCEL_MOTION_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_CANCEL_MOTION_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_GET_MESSAGE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_GET_MESSAGE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_WEAVING_MODE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_WEAVING_MODE_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SELECT_WEAVING_GROUP_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SELECT_WEAVING_GROUP_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_SET_WEAVING_GROUP_PARAMETER_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_DEBUG_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_DEBUG_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(strConCat(LUA_EXTENDED_SERVICE_COMMANDOLD,"@","RRS1"),strConCat("Please use the ",LUA_EXTENDED_SERVICE_COMMAND," notation instead"),0);


    return(PLUGIN_VERSION);
}

VREP_DLLEXPORT void v_repEnd()
{
    unloadVrepLibrary(vrepLib);
}

VREP_DLLEXPORT void* v_repMessage(int message,int* auxiliaryData,void* customData,int* replyData)
{ // This is called quite often. Just watch out for messages/events you want to handle
    // Keep following 5 lines at the beginning and unchanged:
    int errorModeSaved;
    simGetIntegerParameter(sim_intparam_error_report_mode,&errorModeSaved);
    simSetIntegerParameter(sim_intparam_error_report_mode,sim_api_errormessage_ignore);
    void* retVal=NULL;

    if (message==sim_message_eventcallback_simulationended)
    { // simulation ended. End all started RCS servers:
        for (unsigned int i=0;i<allRcsServers.size();i++)
            delete allRcsServers[i].connection;
        allRcsServers.clear();
    }


    // Keep following unchanged:
    simSetIntegerParameter(sim_intparam_error_report_mode,errorModeSaved);
    return(retVal);
}

