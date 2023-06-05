#include "simRRS1.h"
#include <simLib/simLib.h>
#include <simLib/scriptFunctionData.h>
#include <simLib/socketOutConnection.h>
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

#define PLUGIN_VERSION 4 // 4 since 4.6

static LIBRARY simLib;

struct sRcsServer
{
    CSocketOutConnection* connection;
    int scriptId;
    int rcsServerHandle;
    bool isCurrentServer;
};

static std::vector<sRcsServer> allRcsServers;
static int nextRcsServerHandle=0;
static std::string currentDirAndPath;

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

// --------------------------------------------------------------------------------------
// Auxiliary function
// --------------------------------------------------------------------------------------
#define LUA_START_RCS_SERVER_COMMAND "startRcsServer"

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
    if (D.readDataFromStack(p->stackID,inArgs_START_RCS_SERVER,inArgs_START_RCS_SERVER[0],nullptr))
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
                            simSetLastError(nullptr,"The RCS server failed to load the RCS library.");
                        if (data[0]==1)
                            simSetLastError(nullptr,"The RCS server failed to bind the RCS service function.");
                        delete connection;
                    }
                    delete[] data;
                }
                else
                {
                    delete connection;
                    simSetLastError(nullptr,"Failed to receive data from the RCS server.");
                }
            }
            else
            {
                delete connection;
                simSetLastError(nullptr,"Failed to send data to the RCS server.");
            }
        }
        else
        {
            delete connection;
            simSetLastError(nullptr,"Failed to start or connect to RCS server. (i.e. 'rcsServer.exe')");
        }

    }
    D.pushOutData(CScriptFunctionDataItem(handle));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Auxiliary function
// --------------------------------------------------------------------------------------
#define LUA_SELECT_RCS_SERVER_COMMAND "selectRcsServer"

const int inArgs_SELECT_RCS_SERVER[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SELECT_RCS_SERVER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_RCS_SERVER,inArgs_SELECT_RCS_SERVER[0],nullptr))
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
                simSetLastError(nullptr,"Cannot access RCS server started in a different script.");
        }
        else
            simSetLastError(nullptr,"Invalid RCS server handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Auxiliary function
// --------------------------------------------------------------------------------------
#define LUA_STOP_RCS_SERVER_COMMAND "stopRcsServer"

const int inArgs_STOP_RCS_SERVER[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_STOP_RCS_SERVER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(p->stackID,inArgs_STOP_RCS_SERVER,inArgs_STOP_RCS_SERVER[0],nullptr))
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
                simSetLastError(nullptr,"Cannot access RCS server started in a different script.");
        }
        else
            simSetLastError(nullptr,"Invalid RCS server handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------
// INITIALIZE
// --------------------------------------------------------------------------------------
#define LUA_INITIALIZE_COMMAND "INITIALIZE"
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
    if (D.readDataFromStack(p->stackID,inArgs_INITIALIZE,inArgs_INITIALIZE[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// RESET
// --------------------------------------------------------------------------------------
#define LUA_RESET_COMMAND "RESET"
const int inArgs_RESET[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_RESET_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_RESET,inArgs_RESET[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// TERMINATE
// --------------------------------------------------------------------------------------
#define LUA_TERMINATE_COMMAND "TERMINATE"

const int inArgs_TERMINATE[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_TERMINATE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_TERMINATE,inArgs_TERMINATE[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_ROBOT_STAMP
// --------------------------------------------------------------------------------------
#define LUA_GET_ROBOT_STAMP_COMMAND "GET_ROBOT_STAMP"
const int inArgs_GET_ROBOT_STAMP[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_GET_ROBOT_STAMP_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_ROBOT_STAMP,inArgs_GET_ROBOT_STAMP[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_HOME_JOINT_POSITION
// --------------------------------------------------------------------------------------
#define LUA_GET_HOME_JOINT_POSITION_COMMAND "GET_HOME_JOINT_POSITION"
const int inArgs_GET_HOME_JOINT_POSITION[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_GET_HOME_JOINT_POSITION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_HOME_JOINT_POSITION,inArgs_GET_HOME_JOINT_POSITION[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_RCS_DATA
// --------------------------------------------------------------------------------------
#define LUA_GET_RCS_DATA_COMMAND "GET_RCS_DATA"
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
    if (D.readDataFromStack(p->stackID,inArgs_GET_RCS_DATA,inArgs_GET_RCS_DATA[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// MODIFY_RCS_DATA
// --------------------------------------------------------------------------------------
#define LUA_MODIFY_RCS_DATA_COMMAND "MODIFY_RCS_DATA"
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
    if (D.readDataFromStack(p->stackID,inArgs_MODIFY_RCS_DATA,inArgs_MODIFY_RCS_DATA[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SAVE_RCS_DATA
// --------------------------------------------------------------------------------------
#define LUA_SAVE_RCS_DATA_COMMAND "SAVE_RCS_DATA"
const int inArgs_SAVE_RCS_DATA[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_SAVE_RCS_DATA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SAVE_RCS_DATA,inArgs_SAVE_RCS_DATA[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// LOAD_RCS_DATA
// --------------------------------------------------------------------------------------
#define LUA_LOAD_RCS_DATA_COMMAND "LOAD_RCS_DATA"
const int inArgs_LOAD_RCS_DATA[]={
    1,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
};

void LUA_LOAD_RCS_DATA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_LOAD_RCS_DATA,inArgs_LOAD_RCS_DATA[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_INVERSE_KINEMATIC
// --------------------------------------------------------------------------------------
#define LUA_GET_INVERSE_KINEMATIC_COMMAND "GET_INVERSE_KINEMATIC"
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
    if (D.readDataFromStack(p->stackID,inArgs_GET_INVERSE_KINEMATIC,inArgs_GET_INVERSE_KINEMATIC[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_FORWARD_KINEMATIC
// --------------------------------------------------------------------------------------
#define LUA_GET_FORWARD_KINEMATIC_COMMAND "GET_FORWARD_KINEMATIC"
const int inArgs_GET_FORWARD_KINEMATIC[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,JOINTPOS_SIZE,
};

void LUA_GET_FORWARD_KINEMATIC_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_FORWARD_KINEMATIC,inArgs_GET_FORWARD_KINEMATIC[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// MATRIX_TO_CONTROLLER_POSITION
// --------------------------------------------------------------------------------------
#define LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND "MATRIX_TO_CONTROLLER_POSITION"
const int inArgs_MATRIX_TO_CONTROLLER_POSITION[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,CARTPOS_SIZE,
    sim_script_arg_string,0,
};

void LUA_MATRIX_TO_CONTROLLER_POSITION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_MATRIX_TO_CONTROLLER_POSITION,inArgs_MATRIX_TO_CONTROLLER_POSITION[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CONTROLLER_POSITION_TO_MATRIX
// --------------------------------------------------------------------------------------
#define LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND "CONTROLLER_POSITION_TO_MATRIX"
const int inArgs_CONTROLLER_POSITION_TO_MATRIX[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_string,0,
};

void LUA_CONTROLLER_POSITION_TO_MATRIX_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_CONTROLLER_POSITION_TO_MATRIX,inArgs_CONTROLLER_POSITION_TO_MATRIX[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_CELL_FRAME
// --------------------------------------------------------------------------------------
#define LUA_GET_CELL_FRAME_COMMAND "GET_CELL_FRAME"
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
    if (D.readDataFromStack(p->stackID,inArgs_GET_CELL_FRAME,inArgs_GET_CELL_FRAME[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// MODIFY_CELL_FRAME
// --------------------------------------------------------------------------------------
#define LUA_MODIFY_CELL_FRAME_COMMAND "MODIFY_CELL_FRAME"
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
    if (D.readDataFromStack(p->stackID,inArgs_MODIFY_CELL_FRAME,inArgs_MODIFY_CELL_FRAME[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_WORK_FRAMES
// --------------------------------------------------------------------------------------
#define LUA_SELECT_WORK_FRAMES_COMMAND "SELECT_WORK_FRAMES"
const int inArgs_SELECT_WORK_FRAMES[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_string,0,
    sim_script_arg_string,0,
};

void LUA_SELECT_WORK_FRAMES_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_WORK_FRAMES,inArgs_SELECT_WORK_FRAMES[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_INITIAL_POSITION
// --------------------------------------------------------------------------------------
#define LUA_SET_INITIAL_POSITION_COMMAND "SET_INITIAL_POSITION"
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
    if (D.readDataFromStack(p->stackID,inArgs_SET_INITIAL_POSITION,inArgs_SET_INITIAL_POSITION[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_NEXT_TARGET
// --------------------------------------------------------------------------------------
#define LUA_SET_NEXT_TARGET_COMMAND "SET_NEXT_TARGET"
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
    if (D.readDataFromStack(p->stackID,inArgs_SET_NEXT_TARGET,inArgs_SET_NEXT_TARGET[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_NEXT_STEP
// --------------------------------------------------------------------------------------
#define LUA_GET_NEXT_STEP_COMMAND "GET_NEXT_STEP"
const int inArgs_GET_NEXT_STEP[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,BITSTRING_SIZE,
};

void LUA_GET_NEXT_STEP_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_GET_NEXT_STEP,inArgs_GET_NEXT_STEP[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_INTERPOLATION_TIME
// --------------------------------------------------------------------------------------
#define LUA_SET_INTERPOLATION_TIME_COMMAND "SET_INTERPOLATION_TIME"
const int inArgs_SET_INTERPOLATION_TIME[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_double,0,
};

void LUA_SET_INTERPOLATION_TIME_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_INTERPOLATION_TIME,inArgs_SET_INTERPOLATION_TIME[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_MOTION_TYPE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_MOTION_TYPE_COMMAND "SELECT_MOTION_TYPE"
const int inArgs_SELECT_MOTION_TYPE[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_SELECT_MOTION_TYPE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_MOTION_TYPE,inArgs_SELECT_MOTION_TYPE[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_TARGET_TYPE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_TARGET_TYPE_COMMAND "SELECT_TARGET_TYPE"
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
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_TARGET_TYPE,inArgs_SELECT_TARGET_TYPE[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_TRAJECTORY_MODE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_TRAJECTORY_MODE_COMMAND "SELECT_TRAJECTORY_MODE"
const int inArgs_SELECT_TRAJECTORY_MODE[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_SELECT_TRAJECTORY_MODE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_TRAJECTORY_MODE,inArgs_SELECT_TRAJECTORY_MODE[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_ORIENTATION_INTERPOLATION_MODE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND "SELECT_ORIENTATION_INTERPOLATION_MODE"
const int inArgs_SELECT_ORIENTATION_INTERPOLATION_MODE[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
};

void LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_ORIENTATION_INTERPOLATION_MODE,inArgs_SELECT_ORIENTATION_INTERPOLATION_MODE[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_DOMINANT_INTERPOLATION
// --------------------------------------------------------------------------------------
#define LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND "SELECT_DOMINANT_INTERPOLATION"
const int inArgs_SELECT_DOMINANT_INTERPOLATION[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
};

void LUA_SELECT_DOMINANT_INTERPOLATION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_DOMINANT_INTERPOLATION,inArgs_SELECT_DOMINANT_INTERPOLATION[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_ADVANCE_MOTION
// --------------------------------------------------------------------------------------
#define LUA_SET_ADVANCE_MOTION_COMMAND "SET_ADVANCE_MOTION"
const int inArgs_SET_ADVANCE_MOTION[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_SET_ADVANCE_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_ADVANCE_MOTION,inArgs_SET_ADVANCE_MOTION[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_MOTION_FILTER
// --------------------------------------------------------------------------------------
#define LUA_SET_MOTION_FILTER_COMMAND "SET_MOTION_FILTER"
const int inArgs_SET_MOTION_FILTER[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_int32,0,
};

void LUA_SET_MOTION_FILTER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_MOTION_FILTER,inArgs_SET_MOTION_FILTER[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_OVERRIDE_POSITION
// --------------------------------------------------------------------------------------
#define LUA_SET_OVERRIDE_POSITION_COMMAND "SET_OVERRIDE_POSITION"
const int inArgs_SET_OVERRIDE_POSITION[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,FRAME_SIZE,
};

void LUA_SET_OVERRIDE_POSITION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_OVERRIDE_POSITION,inArgs_SET_OVERRIDE_POSITION[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// REVERSE_MOTION
// --------------------------------------------------------------------------------------
#define LUA_REVERSE_MOTION_COMMAND "REVERSE_MOTION"
const int inArgs_REVERSE_MOTION[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_double,0,
};

void LUA_REVERSE_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_REVERSE_MOTION,inArgs_REVERSE_MOTION[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_PAYLOAD_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_PAYLOAD_PARAMETER_COMMAND "SET_PAYLOAD_PARAMETER"
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
    if (D.readDataFromStack(p->stackID,inArgs_SET_PAYLOAD_PARAMETER,inArgs_SET_PAYLOAD_PARAMETER[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_TIME_COMPENSATION
// --------------------------------------------------------------------------------------
#define LUA_SELECT_TIME_COMPENSATION_COMMAND "SELECT_TIME_COMPENSATION"
const int inArgs_SELECT_TIME_COMPENSATION[]={
    2,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_charbuff,BITSTRING_SIZE,
};

void LUA_SELECT_TIME_COMPENSATION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECT_TIME_COMPENSATION,inArgs_SELECT_TIME_COMPENSATION[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CONFIGURATION_CONTROL
// --------------------------------------------------------------------------------------
#define LUA_SET_CONFIGURATION_CONTROL_COMMAND "SET_CONFIGURATION_CONTROL"
const int inArgs_SET_CONFIGURATION_CONTROL[]={
    3,
    sim_script_arg_charbuff,BITSTRING2_SIZE,
    sim_script_arg_string,0,
    sim_script_arg_string,0,
};

void LUA_SET_CONFIGURATION_CONTROL_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SET_CONFIGURATION_CONTROL,inArgs_SET_CONFIGURATION_CONTROL[0],nullptr))
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
                        simSetLastError(nullptr,"Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr,"Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr,"Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr,"There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_JOINT_SPEEDS
// --------------------------------------------------------------------------------------
#define LUA_SET_JOINT_SPEEDS_COMMAND "SET_JOINT_SPEEDS"
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
    if (D.readDataFromStack(p->stackID, inArgs_SET_JOINT_SPEEDS, inArgs_SET_JOINT_SPEEDS[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CARTESIAN_POSITION_SPEED
// --------------------------------------------------------------------------------------
#define LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND "SET_CARTESIAN_POSITION_SPEED"
const int inArgs_SET_CARTESIAN_POSITION_SPEED[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
};

void LUA_SET_CARTESIAN_POSITION_SPEED_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_CARTESIAN_POSITION_SPEED, inArgs_SET_CARTESIAN_POSITION_SPEED[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CARTESIAN_ORIENTATION_SPEED
// --------------------------------------------------------------------------------------
#define LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND "SET_CARTESIAN_ORIENTATION_SPEED"
const int inArgs_SET_CARTESIAN_ORIENTATION_SPEED[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
};

void LUA_SET_CARTESIAN_ORIENTATION_SPEED_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_CARTESIAN_ORIENTATION_SPEED, inArgs_SET_CARTESIAN_ORIENTATION_SPEED[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_JOINT_ACCELERATIONS
// --------------------------------------------------------------------------------------
#define LUA_SET_JOINT_ACCELERATIONS_COMMAND "SET_JOINT_ACCELERATIONS"
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
    if (D.readDataFromStack(p->stackID, inArgs_SET_JOINT_ACCELERATIONS, inArgs_SET_JOINT_ACCELERATIONS[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CARTESIAN_POSITION_ACCELERATION
// --------------------------------------------------------------------------------------
#define LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND "SET_CARTESIAN_POSITION_ACCELERATION"
const int inArgs_SET_CARTESIAN_POSITION_ACCELERATION[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
    sim_script_arg_int32, 0,
};

void LUA_SET_CARTESIAN_POSITION_ACCELERATION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_CARTESIAN_POSITION_ACCELERATION, inArgs_SET_CARTESIAN_POSITION_ACCELERATION[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CARTESIAN_ORIENTATION_ACCELERATION
// --------------------------------------------------------------------------------------
#define LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND "SET_CARTESIAN_ORIENTATION_ACCELERATION"
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
    if (D.readDataFromStack(p->stackID, inArgs_SET_CARTESIAN_ORIENTATION_ACCELERATION, inArgs_SET_CARTESIAN_ORIENTATION_ACCELERATION[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_JOINT_JERKS
// --------------------------------------------------------------------------------------
#define LUA_SET_JOINT_JERKS_COMMAND "SET_JOINT_JERKS"
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
    if (D.readDataFromStack(p->stackID, inArgs_SET_JOINT_JERKS, inArgs_SET_JOINT_JERKS[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_MOTION_TIME
// --------------------------------------------------------------------------------------
#define LUA_SET_MOTION_TIME_COMMAND "SET_MOTION_TIME"
const int inArgs_SET_MOTION_TIME[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
};

void LUA_SET_MOTION_TIME_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_MOTION_TIME, inArgs_SET_MOTION_TIME[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_OVERRIDE_SPEED
// --------------------------------------------------------------------------------------
#define LUA_SET_OVERRIDE_SPEED_COMMAND "SET_OVERRIDE_SPEED"
const int inArgs_SET_OVERRIDE_SPEED[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_double, 0,
    sim_script_arg_int32, 0,
};

void LUA_SET_OVERRIDE_SPEED_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_OVERRIDE_SPEED, inArgs_SET_OVERRIDE_SPEED[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_OVERRIDE_ACCELERATION
// --------------------------------------------------------------------------------------
#define LUA_SET_OVERRIDE_ACCELERATION_COMMAND "SET_OVERRIDE_ACCELERATION"
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
    if (D.readDataFromStack(p->stackID, inArgs_SET_OVERRIDE_ACCELERATION, inArgs_SET_OVERRIDE_ACCELERATION[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_FLYBY_MODE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_FLYBY_MODE_COMMAND "SELECT_FLYBY_MODE"
const int inArgs_SELECT_FLYBY_MODE[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
};

void LUA_SELECT_FLYBY_MODE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_FLYBY_MODE, inArgs_SELECT_FLYBY_MODE[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_FLYBY_CRITERIA_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND "SET_FLYBY_CRITERIA_PARAMETER"
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
    if (D.readDataFromStack(p->stackID, inArgs_SET_FLYBY_CRITERIA_PARAMETER, inArgs_SET_FLYBY_CRITERIA_PARAMETER[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_FLYBY_CRITERIA
// --------------------------------------------------------------------------------------
#define LUA_SELECT_FLYBY_CRITERIA_COMMAND "SELECT_FLYBY_CRITERIA"
const int inArgs_SELECT_FLYBY_CRITERIA[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
};

void LUA_SELECT_FLYBY_CRITERIA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_FLYBY_CRITERIA, inArgs_SELECT_FLYBY_CRITERIA[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CANCEL_FLYBY_CRITERIA
// --------------------------------------------------------------------------------------
#define LUA_CANCEL_FLYBY_CRITERIA_COMMAND "CANCEL_FLYBY_CRITERIA"
const int inArgs_CANCEL_FLYBY_CRITERIA[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
};

void LUA_CANCEL_FLYBY_CRITERIA_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_CANCEL_FLYBY_CRITERIA, inArgs_CANCEL_FLYBY_CRITERIA[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_POINT_ACCURACY
// --------------------------------------------------------------------------------------
#define LUA_SELECT_POINT_ACCURACY_COMMAND "SELECT_POINT_ACCURACY"
const int inArgs_SELECT_POINT_ACCURACY[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
};

void LUA_SELECT_POINT_ACCURACY_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_POINT_ACCURACY, inArgs_SELECT_POINT_ACCURACY[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_POINT_ACCURACY_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND "SET_POINT_ACCURACY_PARAMETER"
const int inArgs_SET_POINT_ACCURACY_PARAMETER[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
};

void LUA_SET_POINT_ACCURACY_PARAMETER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_POINT_ACCURACY_PARAMETER, inArgs_SET_POINT_ACCURACY_PARAMETER[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_REST_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_REST_PARAMETER_COMMAND "SET_REST_PARAMETER"
const int inArgs_SET_REST_PARAMETER[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_double, 0,
};

void LUA_SET_REST_PARAMETER_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SET_REST_PARAMETER, inArgs_SET_REST_PARAMETER[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_CURRENT_TARGETID
// --------------------------------------------------------------------------------------
#define LUA_GET_CURRENT_TARGETID_COMMAND "GET_CURRENT_TARGETID"
const int inArgs_GET_CURRENT_TARGETID[] = {
    1,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
};

void LUA_GET_CURRENT_TARGETID_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_GET_CURRENT_TARGETID, inArgs_GET_CURRENT_TARGETID[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_TRACKING
// --------------------------------------------------------------------------------------
#define LUA_SELECT_TRACKING_COMMAND "SELECT_TRACKING"
const int inArgs_SELECT_TRACKING[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_charbuff, BITSTRING_SIZE,
};

void LUA_SELECT_TRACKING_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_TRACKING, inArgs_SELECT_TRACKING[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_CONVEYOR_POSITION
// --------------------------------------------------------------------------------------
#define LUA_SET_CONVEYOR_POSITION_COMMAND "SET_CONVEYOR_POSITION"
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
    if (D.readDataFromStack(p->stackID, inArgs_SET_CONVEYOR_POSITION, inArgs_SET_CONVEYOR_POSITION[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// DEFINE_EVENT
// --------------------------------------------------------------------------------------
#define LUA_DEFINE_EVENT_COMMAND "DEFINE_EVENT"
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
    if (D.readDataFromStack(p->stackID, inArgs_DEFINE_EVENT, inArgs_DEFINE_EVENT[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CANCEL_EVENT
// --------------------------------------------------------------------------------------
#define LUA_CANCEL_EVENT_COMMAND "CANCEL_EVENT"
const int inArgs_CANCEL_EVENT[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
 };

void LUA_CANCEL_EVENT_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_CANCEL_EVENT, inArgs_CANCEL_EVENT[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_EVENT
// --------------------------------------------------------------------------------------
#define LUA_GET_EVENT_COMMAND "GET_EVENT"
const int inArgs_GET_EVENT[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
 };

void LUA_GET_EVENT_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_GET_EVENT, inArgs_GET_EVENT[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// STOP_MOTION
// --------------------------------------------------------------------------------------
#define LUA_STOP_MOTION_COMMAND "STOP_MOTION"
const int inArgs_STOP_MOTION[] = {
    1,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
 };

void LUA_STOP_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_STOP_MOTION, inArgs_STOP_MOTION[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CONTINUE_MOTION
// --------------------------------------------------------------------------------------
#define LUA_CONTINUE_MOTION_COMMAND "CONTINUE_MOTION"
const int inArgs_CONTINUE_MOTION[] = {
    1,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
 };

void LUA_CONTINUE_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_CONTINUE_MOTION, inArgs_CONTINUE_MOTION[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// CANCEL_MOTION
// --------------------------------------------------------------------------------------
#define LUA_CANCEL_MOTION_COMMAND "CANCEL_MOTION"
const int inArgs_CANCEL_MOTION[] = {
    1,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
 };

void LUA_CANCEL_MOTION_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_CANCEL_MOTION, inArgs_CANCEL_MOTION[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// GET_MESSAGE
// --------------------------------------------------------------------------------------
#define LUA_GET_MESSAGE_COMMAND "GET_MESSAGE"
const int inArgs_GET_MESSAGE[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
 };

void LUA_GET_MESSAGE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_GET_MESSAGE, inArgs_GET_MESSAGE[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_WEAVING_MODE
// --------------------------------------------------------------------------------------
#define LUA_SELECT_WEAVING_MODE_COMMAND "SELECT_WEAVING_MODE"
const int inArgs_SELECT_WEAVING_MODE[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
 };

void LUA_SELECT_WEAVING_MODE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_WEAVING_MODE, inArgs_SELECT_WEAVING_MODE[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SELECT_WEAVING_GROUP
// --------------------------------------------------------------------------------------
#define LUA_SELECT_WEAVING_GROUP_COMMAND "SELECT_WEAVING_GROUP"
const int inArgs_SELECT_WEAVING_GROUP[] = {
    3,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_int32, 0,
    sim_script_arg_int32, 0,
 };

void LUA_SELECT_WEAVING_GROUP_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_SELECT_WEAVING_GROUP, inArgs_SELECT_WEAVING_GROUP[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// SET_WEAVING_GROUP_PARAMETER
// --------------------------------------------------------------------------------------
#define LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND "SET_WEAVING_GROUP_PARAMETER"
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
    if (D.readDataFromStack(p->stackID, inArgs_SET_WEAVING_GROUP_PARAMETER, inArgs_SET_WEAVING_GROUP_PARAMETER[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// DEBUG
// --------------------------------------------------------------------------------------
#define LUA_DEBUG_COMMAND "DEBUG"
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
    if (D.readDataFromStack(p->stackID, inArgs_DEBUG, inArgs_DEBUG[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// EXTENDED_SERVICE
// --------------------------------------------------------------------------------------
#define LUA_EXTENDED_SERVICE_COMMAND "EXTENDED_SERVICE"
const int inArgs_EXTENDED_SERVICE[] = {
    2,
    sim_script_arg_charbuff, BITSTRING2_SIZE,
    sim_script_arg_charbuff, 0, // can be any length
 };

void LUA_EXTENDED_SERVICE_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID, inArgs_EXTENDED_SERVICE, inArgs_EXTENDED_SERVICE[0],nullptr))
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
                        simSetLastError(nullptr, "Received a bad reply from the server.");
                }
                else
                    simSetLastError(nullptr, "Failed receiving a reply from the server.");
            }
            else
                simSetLastError(nullptr, "Failed sending data to the server.");
        }
        else
            simSetLastError(nullptr, "There is no RCS server currently selected for this script.");

    }
}
// --------------------------------------------------------------------------------------

SIM_DLLEXPORT int simInit(const char* pluginName)
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

    // 2. Append the CoppeliaSim library's name:
    std::string temp(currentDirAndPath);
#ifdef _WIN32
    temp+="\\coppeliaSim.dll";
#elif defined (__linux)
    temp+="/libcoppeliaSim.so";
#elif defined (__APPLE__)
    temp+="/libcoppeliaSim.dylib";
#endif 

    // 3. Load the CoppeliaSim library:
    simLib=loadSimLibrary(temp.c_str());
    if (simLib==NULL)
    {
        simAddLog(pluginName,sim_verbosity_errors,"could not find or correctly load the CoppeliaSim library. Cannot start the plugin.");
        return(0); 
    }
    if (getSimProcAddresses(simLib)==0)
    {
        simAddLog(pluginName,sim_verbosity_errors,"could not find all required functions in the CoppeliaSim library. Cannot start the plugin.");
        unloadSimLibrary(simLib);
        return(0);
    }

    // startRcsModule (auxiliary command)
    simRegisterScriptCallbackFunction(LUA_START_RCS_SERVER_COMMAND,nullptr,LUA_START_RCS_SERVER_CALLBACK);

    // selectRcsModule (auxiliary command)
    simRegisterScriptCallbackFunction(LUA_SELECT_RCS_SERVER_COMMAND,nullptr,LUA_SELECT_RCS_SERVER_CALLBACK);

    // stopRcsModule (auxiliary command)
    simRegisterScriptCallbackFunction(LUA_STOP_RCS_SERVER_COMMAND,nullptr,LUA_STOP_RCS_SERVER_CALLBACK);

    // INITIALIZE
    simRegisterScriptCallbackFunction(LUA_INITIALIZE_COMMAND,nullptr,LUA_INITIALIZE_CALLBACK);

    // RESET
    simRegisterScriptCallbackFunction(LUA_RESET_COMMAND,nullptr,LUA_RESET_CALLBACK);

    // TERMINATE
    simRegisterScriptCallbackFunction(LUA_TERMINATE_COMMAND,nullptr,LUA_TERMINATE_CALLBACK);

    // GET_ROBOT_STAMP
    simRegisterScriptCallbackFunction(LUA_GET_ROBOT_STAMP_COMMAND,nullptr,LUA_GET_ROBOT_STAMP_CALLBACK);

    // GET_HOME_JOINT_POSITION
    simRegisterScriptCallbackFunction(LUA_GET_HOME_JOINT_POSITION_COMMAND,nullptr,LUA_GET_HOME_JOINT_POSITION_CALLBACK);

    // GET_RCS_DATA
    simRegisterScriptCallbackFunction(LUA_GET_RCS_DATA_COMMAND,nullptr,LUA_GET_RCS_DATA_CALLBACK);

    // MODIFY_RCS_DATA
    simRegisterScriptCallbackFunction(LUA_MODIFY_RCS_DATA_COMMAND,nullptr,LUA_MODIFY_RCS_DATA_CALLBACK);

    // SAVE_RCS_DATA
    simRegisterScriptCallbackFunction(LUA_SAVE_RCS_DATA_COMMAND,nullptr,LUA_SAVE_RCS_DATA_CALLBACK);

    // LOAD_RCS_DATA
    simRegisterScriptCallbackFunction(LUA_LOAD_RCS_DATA_COMMAND,nullptr,LUA_LOAD_RCS_DATA_CALLBACK);

    // GET_INVERSE_KINEMATIC
    simRegisterScriptCallbackFunction(LUA_GET_INVERSE_KINEMATIC_COMMAND,nullptr,LUA_GET_INVERSE_KINEMATIC_CALLBACK);

    // GET_FORWARD_KINEMATIC
    simRegisterScriptCallbackFunction(LUA_GET_FORWARD_KINEMATIC_COMMAND,nullptr,LUA_GET_FORWARD_KINEMATIC_CALLBACK);

    // MATRIX_TO_CONTROLLER_POSITION
    simRegisterScriptCallbackFunction(LUA_MATRIX_TO_CONTROLLER_POSITION_COMMAND,nullptr,LUA_MATRIX_TO_CONTROLLER_POSITION_CALLBACK);

    // CONTROLLER_POSITION_TO_MATRIX
    simRegisterScriptCallbackFunction(LUA_CONTROLLER_POSITION_TO_MATRIX_COMMAND,nullptr,LUA_CONTROLLER_POSITION_TO_MATRIX_CALLBACK);

    // GET_CELL_FRAME
    simRegisterScriptCallbackFunction(LUA_GET_CELL_FRAME_COMMAND,nullptr,LUA_GET_CELL_FRAME_CALLBACK);

    // MODIFY_CELL_FRAME
    simRegisterScriptCallbackFunction(LUA_MODIFY_CELL_FRAME_COMMAND,nullptr,LUA_MODIFY_CELL_FRAME_CALLBACK);

    // SELECT_WORK_FRAMES
    simRegisterScriptCallbackFunction(LUA_SELECT_WORK_FRAMES_COMMAND,nullptr,LUA_SELECT_WORK_FRAMES_CALLBACK);

    // SET_INITIAL_POSITION
    simRegisterScriptCallbackFunction(LUA_SET_INITIAL_POSITION_COMMAND,nullptr,LUA_SET_INITIAL_POSITION_CALLBACK);

    // SET_NEXT_TARGET
    simRegisterScriptCallbackFunction(LUA_SET_NEXT_TARGET_COMMAND,nullptr,LUA_SET_NEXT_TARGET_CALLBACK);

    // GET_NEXT_STEP
    simRegisterScriptCallbackFunction(LUA_GET_NEXT_STEP_COMMAND,nullptr,LUA_GET_NEXT_STEP_CALLBACK);

    // SET_INTERPOLATION_TIME
    simRegisterScriptCallbackFunction(LUA_SET_INTERPOLATION_TIME_COMMAND,nullptr,LUA_SET_INTERPOLATION_TIME_CALLBACK);

    // SELECT_MOTION_TYPE
    simRegisterScriptCallbackFunction(LUA_SELECT_MOTION_TYPE_COMMAND,nullptr,LUA_SELECT_MOTION_TYPE_CALLBACK);

    // SELECT_TARGET_TYPE
    simRegisterScriptCallbackFunction(LUA_SELECT_TARGET_TYPE_COMMAND,nullptr,LUA_SELECT_TARGET_TYPE_CALLBACK);

    // SELECT_TRAJECTORY_MODE
    simRegisterScriptCallbackFunction(LUA_SELECT_TRAJECTORY_MODE_COMMAND,nullptr,LUA_SELECT_TRAJECTORY_MODE_CALLBACK);

    // SELECT_ORIENTATION_INTERPOLATION_MODE
    simRegisterScriptCallbackFunction(LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_COMMAND,nullptr,LUA_SELECT_ORIENTATION_INTERPOLATION_MODE_CALLBACK);

    // SELECT_DOMINANT_INTERPOLATION
    simRegisterScriptCallbackFunction(LUA_SELECT_DOMINANT_INTERPOLATION_COMMAND,nullptr,LUA_SELECT_DOMINANT_INTERPOLATION_CALLBACK);

    // SET_ADVANCE_MOTION
    simRegisterScriptCallbackFunction(LUA_SET_ADVANCE_MOTION_COMMAND,nullptr,LUA_SET_ADVANCE_MOTION_CALLBACK);

    // SET_MOTION_FILTER
    simRegisterScriptCallbackFunction(LUA_SET_MOTION_FILTER_COMMAND,nullptr,LUA_SET_MOTION_FILTER_CALLBACK);

    // SET_OVERRIDE_POSITION
    simRegisterScriptCallbackFunction(LUA_SET_OVERRIDE_POSITION_COMMAND,nullptr,LUA_SET_OVERRIDE_POSITION_CALLBACK);

    // REVERSE_MOTION
    simRegisterScriptCallbackFunction(LUA_REVERSE_MOTION_COMMAND,nullptr,LUA_REVERSE_MOTION_CALLBACK);

    // SET_PAYLOAD_PARAMETER
    simRegisterScriptCallbackFunction(LUA_SET_PAYLOAD_PARAMETER_COMMAND,nullptr,LUA_SET_PAYLOAD_PARAMETER_CALLBACK);

    // SELECT_TIME_COMPENSATION
    simRegisterScriptCallbackFunction(LUA_SELECT_TIME_COMPENSATION_COMMAND,nullptr,LUA_SELECT_TIME_COMPENSATION_CALLBACK);

    // SET_CONFIGURATION_CONTROL
    simRegisterScriptCallbackFunction(LUA_SET_CONFIGURATION_CONTROL_COMMAND,nullptr,LUA_SET_CONFIGURATION_CONTROL_CALLBACK);

    // SET_JOINT_SPEEDS
    simRegisterScriptCallbackFunction(LUA_SET_JOINT_SPEEDS_COMMAND,nullptr,LUA_SET_JOINT_SPEEDS_CALLBACK);

    // SET_CARTESIAN_POSITION_SPEED
    simRegisterScriptCallbackFunction(LUA_SET_CARTESIAN_POSITION_SPEED_COMMAND,nullptr,LUA_SET_CARTESIAN_POSITION_SPEED_CALLBACK);

    // SET_CARTESIAN_ORIENTATION_SPEED
    simRegisterScriptCallbackFunction(LUA_SET_CARTESIAN_ORIENTATION_SPEED_COMMAND,nullptr,LUA_SET_CARTESIAN_ORIENTATION_SPEED_CALLBACK);

    // SET_JOINT_ACCELERATIONS
    simRegisterScriptCallbackFunction(LUA_SET_JOINT_ACCELERATIONS_COMMAND,nullptr,LUA_SET_JOINT_ACCELERATIONS_CALLBACK);

    // SET_CARTESIAN_POSITION_ACCELERATION
    simRegisterScriptCallbackFunction(LUA_SET_CARTESIAN_POSITION_ACCELERATION_COMMAND,nullptr,LUA_SET_CARTESIAN_POSITION_ACCELERATION_CALLBACK);

    // SET_CARTESIAN_ORIENTATION_ACCELERATION
    simRegisterScriptCallbackFunction(LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_COMMAND,nullptr,LUA_SET_CARTESIAN_ORIENTATION_ACCELERATION_CALLBACK);

    // SET_JOINT_JERKS
    simRegisterScriptCallbackFunction(LUA_SET_JOINT_JERKS_COMMAND,nullptr,LUA_SET_JOINT_JERKS_CALLBACK);

    // SET_MOTION_TIME
    simRegisterScriptCallbackFunction(LUA_SET_MOTION_TIME_COMMAND,nullptr,LUA_SET_MOTION_TIME_CALLBACK);

    // SET_OVERRIDE_SPEED
    simRegisterScriptCallbackFunction(LUA_SET_OVERRIDE_SPEED_COMMAND,nullptr,LUA_SET_OVERRIDE_SPEED_CALLBACK);

    // SET_OVERRIDE_ACCELERATION
    simRegisterScriptCallbackFunction(LUA_SET_OVERRIDE_ACCELERATION_COMMAND,nullptr,LUA_SET_OVERRIDE_ACCELERATION_CALLBACK);

    // SELECT_FLYBY_MODE
    simRegisterScriptCallbackFunction(LUA_SELECT_FLYBY_MODE_COMMAND,nullptr,LUA_SELECT_FLYBY_MODE_CALLBACK);

    // SET_FLYBY_CRITERIA_PARAMETER
    simRegisterScriptCallbackFunction(LUA_SET_FLYBY_CRITERIA_PARAMETER_COMMAND,nullptr,LUA_SET_FLYBY_CRITERIA_PARAMETER_CALLBACK);

    // SELECT_FLYBY_CRITERIA
    simRegisterScriptCallbackFunction(LUA_SELECT_FLYBY_CRITERIA_COMMAND,nullptr,LUA_SELECT_FLYBY_CRITERIA_CALLBACK);

    // CANCEL_FLYBY_CRITERIA
    simRegisterScriptCallbackFunction(LUA_CANCEL_FLYBY_CRITERIA_COMMAND,nullptr,LUA_CANCEL_FLYBY_CRITERIA_CALLBACK);

    // SELECT_POINT_ACCURACY
    simRegisterScriptCallbackFunction(LUA_SELECT_POINT_ACCURACY_COMMAND,nullptr,LUA_SELECT_POINT_ACCURACY_CALLBACK);

    // SET_POINT_ACCURACY_PARAMETER
    simRegisterScriptCallbackFunction(LUA_SET_POINT_ACCURACY_PARAMETER_COMMAND,nullptr,LUA_SET_POINT_ACCURACY_PARAMETER_CALLBACK);

    // SET_REST_PARAMETER
    simRegisterScriptCallbackFunction(LUA_SET_REST_PARAMETER_COMMAND,nullptr,LUA_SET_REST_PARAMETER_CALLBACK);

    // GET_CURRENT_TARGETID
    simRegisterScriptCallbackFunction(LUA_GET_CURRENT_TARGETID_COMMAND,nullptr,LUA_GET_CURRENT_TARGETID_CALLBACK);

    // SELECT_TRACKING
    simRegisterScriptCallbackFunction(LUA_SELECT_TRACKING_COMMAND,nullptr,LUA_SELECT_TRACKING_CALLBACK);

    // SET_CONVEYOR_POSITION
    simRegisterScriptCallbackFunction(LUA_SET_CONVEYOR_POSITION_COMMAND,nullptr,LUA_SET_CONVEYOR_POSITION_CALLBACK);

    // DEFINE_EVENT
    simRegisterScriptCallbackFunction(LUA_DEFINE_EVENT_COMMAND,nullptr,LUA_DEFINE_EVENT_CALLBACK);

    // CANCEL_EVENT
    simRegisterScriptCallbackFunction(LUA_CANCEL_EVENT_COMMAND,nullptr,LUA_CANCEL_EVENT_CALLBACK);

    // GET_EVENT
    simRegisterScriptCallbackFunction(LUA_GET_EVENT_COMMAND,nullptr,LUA_GET_EVENT_CALLBACK);

    // STOP_MOTION
    simRegisterScriptCallbackFunction(LUA_STOP_MOTION_COMMAND,nullptr,LUA_STOP_MOTION_CALLBACK);

    // CONTINUE_MOTION
    simRegisterScriptCallbackFunction(LUA_CONTINUE_MOTION_COMMAND,nullptr,LUA_CONTINUE_MOTION_CALLBACK);

    // CANCEL_MOTION
    simRegisterScriptCallbackFunction(LUA_CANCEL_MOTION_COMMAND,nullptr,LUA_CANCEL_MOTION_CALLBACK);

    // GET_MESSAGE
    simRegisterScriptCallbackFunction(LUA_GET_MESSAGE_COMMAND,nullptr,LUA_GET_MESSAGE_CALLBACK);

    // SELECT_WEAVING_MODE
    simRegisterScriptCallbackFunction(LUA_SELECT_WEAVING_MODE_COMMAND,nullptr,LUA_SELECT_WEAVING_MODE_CALLBACK);

    // SELECT_WEAVING_GROUP
    simRegisterScriptCallbackFunction(LUA_SELECT_WEAVING_GROUP_COMMAND,nullptr,LUA_SELECT_WEAVING_GROUP_CALLBACK);

    // SET_WEAVING_GROUP_PARAMETER
    simRegisterScriptCallbackFunction(LUA_SET_WEAVING_GROUP_PARAMETER_COMMAND,nullptr,LUA_SET_WEAVING_GROUP_PARAMETER_CALLBACK);

    // DEBUG
    simRegisterScriptCallbackFunction(LUA_DEBUG_COMMAND,nullptr,LUA_DEBUG_CALLBACK);

    // EXTENDED_SERVICE
    simRegisterScriptCallbackFunction(LUA_EXTENDED_SERVICE_COMMAND,nullptr,LUA_EXTENDED_SERVICE_CALLBACK);

    return(PLUGIN_VERSION);
}

SIM_DLLEXPORT void simCleanup()
{
    unloadSimLibrary(simLib);
}

SIM_DLLEXPORT void simMsg(int message,int*,void*)
{
    if (message==sim_message_eventcallback_simulationended)
    { // simulation ended. End all started RCS servers:
        for (unsigned int i=0;i<allRcsServers.size();i++)
            delete allRcsServers[i].connection;
        allRcsServers.clear();
    }
}

