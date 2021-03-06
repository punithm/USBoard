/*
 * SerUSBoard.cpp
 *
 *  Created on: Sep 15, 2014
 *      Author: Punith Mallesha
 */

/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Neobotix GmbH
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Neobotix nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/


#include <stdio.h>
#include <math.h>
#include <ros/ros.h>
#include "../include/SerUSBoard.h"


//-----------------------------------------------

#define NUM_BYTE_SEND_USBOARD_08 8
#define NUM_BYTE_REC_USBOARD_11 11

#define RS232_BAUDRATE 19200
#define RS232_RX_BUFFERSIZE 1024
#define RS232_TX_BUFFERSIZE 1024

#define RS232_TIMEOUT 0.5

#define NUM_BYTE_REC_MAX 80
#define NUM_BYTE_REC_HEADER 1
#define NUM_BYTE_REC_CHECKSUM 2
#define NUM_BYTE_REC 8

#define OUTPUTERROR(...)


SerUSBoard::SerUSBoard()
{
	//autoSendRequest= false;
	m_iNumBytesSend = NUM_BYTE_SEND_USBOARD_08;
}

void SerUSBoard::SetPortConfig(std::string sNumComPort)
{
	m_sNumComPort = sNumComPort;
	m_bComInit = false;
}

//-----------------------------------------------

SerUSBoard::~SerUSBoard()
{
	m_SerIO.closeIO();
}

//-----------------------------------------------

int SerUSBoard::eval_RXBuffer()
{
	int errorFlag = NO_ERROR;
		static int siNoMsgCnt = 0;

		int iNumByteRec = NUM_BYTE_REC;

		const int c_iNrBytesMin = NUM_BYTE_REC_HEADER + iNumByteRec + NUM_BYTE_REC_CHECKSUM;  // =NUM_BYTE_REC_USBOARD_11

			int i;
			int iNrBytesInQueue, iNrBytesRead, iDataStart;
			unsigned char cDat[RS232_RX_BUFFERSIZE];
			unsigned char cTest = 0xFF;

			if( !m_bComInit ) return 0;

				//enough data in queue?
			iNrBytesInQueue = m_SerIO.getSizeRXQueue();
			if(iNrBytesInQueue < c_iNrBytesMin)
				{
					siNoMsgCnt++;
					if(siNoMsgCnt > 25)
					{
						siNoMsgCnt = 0;
						errorFlag = NO_MESSAGES;
					}  else errorFlag = TOO_LESS_BYTES_IN_QUEUE;


					return errorFlag;
				}
				else
				{
					siNoMsgCnt = 0;
				}

				// search most recent data from back of queue
				iNrBytesRead = m_SerIO.readBlocking((char*)&cDat[0], iNrBytesInQueue);

				//log
				if(logging == true)
				{
					log_to_file(2, cDat); //direction 1 = transmitted; 2 = recived
				}


			for(i = (iNrBytesRead - c_iNrBytesMin); i >= 0 ; i--)
				{
					//try to find start bytes
					if((cDat[i] == cTest) && (cDat[i+1]==m_iCmdUSBoard))
					 {
						iDataStart = i + 1;

						// checksum ok?
						if( convRecMsgToData(&cDat[iDataStart]))
						{
							errorFlag = NO_ERROR;
							//continue;
						}
						else
						{
							errorFlag = CHECKSUM_ERROR;
							return errorFlag;
						}
				      }

				 }

 return errorFlag;

}

//-----------------------------------------------

bool SerUSBoard::init() { //CareOBots Syntax Wrapper
	return initPltf();
};


bool SerUSBoard::initPltf()
{
	int iRet;
	m_SerIO.setBaudRate(RS232_BAUDRATE);
	m_SerIO.setDeviceName(m_sNumComPort.c_str());  // m_SerIO.setDeviceName("/dev/ttyUSB0"); 
	m_SerIO.setBufferSize(RS232_RX_BUFFERSIZE, RS232_TX_BUFFERSIZE);
	m_SerIO.setTimeout(RS232_TIMEOUT);
	iRet = m_SerIO.openIO();
	m_bComInit = true;
	return true;
}

//-----------------------------------------------

bool SerUSBoard::reset(){
	return resetPltf();
}

bool SerUSBoard::resetPltf()
{
	m_SerIO.closeIO();
	m_bComInit = false;

	init();
	return true;
}

//-----------------------------------------------

bool SerUSBoard::shutdown()
{
	return shutdownPltf();
}


bool SerUSBoard::shutdownPltf()
{
	m_SerIO.closeIO();
	m_bComInit = false;
	return true;
}

//-----------------------------------------------

bool SerUSBoard::isComError()
{
	return false;
}


//-----------------------------------------------

int SerUSBoard::sendCmdConnect()
{

	   m_iCmdUSBoard= CMD_CONNECT;
	   return(sendCmd());

};

//-----------------------------------------------

int SerUSBoard::sendCmdSetChannelActive()
{

	   m_iCmdUSBoard= CMD_SET_CHANNEL_ACTIVE;
	   return(sendCmd());
};

//-----------------------------------------------

int SerUSBoard::sendCmdGetData1To8()
{

	   m_iCmdUSBoard= CMD_GET_DATA_1TO8;
	   return(sendCmd());

};

//-----------------------------------------------

int SerUSBoard::sendCmdGetData9To16()
{

	    m_iCmdUSBoard= CMD_GET_DATA_9TO16;
		return(sendCmd());

};

//-----------------------------------------------

int SerUSBoard::sendCmdGetAnalogIn()
{

		m_iCmdUSBoard= CMD_GET_ANALOGIN;
		return(sendCmd());

};

//-----------------------------------------------

/*
int SerUSBoard::sendCmdReadParaSet()
{

			m_iCmdUSBoard= CMD_READ_PARASET;
			return(sendCmd());

};


int SerUSBoard::sendCmdSetDebugPara()
{

			m_iCmdUSBoard= CMD_SET_DEBUG_PARA;
			return(sendCmd());

};

int SerUSBoard::sendCmdGetDebugPara()
{

			m_iCmdUSBoard= CMD_GET_DEBUG_PARA;
			return(sendCmd());

};

int SerUSBoard::sendCmdUnknown()
{
			m_iCmdUSBoard= CMD_UNKNOWN;
			return(sendCmd());

};

*/

//-----------------------------------------------

int SerUSBoard::sendCmd()
{


			int errorFlag = NO_ERROR;
			int iNrBytesWritten;
			unsigned char cMsg[m_iNumBytesSend];

		//	m_Mutex.lock();

			convDataToSendMsg(cMsg);

			m_SerIO.purgeTx();

			iNrBytesWritten = m_SerIO.writeIO((char*)cMsg,m_iNumBytesSend);

			if(iNrBytesWritten < m_iNumBytesSend) {
			errorFlag = GENERAL_SENDING_ERROR;
			}
					//log
			if(logging == true)
			{
				log_to_file(1, cMsg); //direction 1 = transmitted; 2 = recived
			}

		//	m_Mutex.unlock();
			return errorFlag;

};


//-----------------------------------------------

int SerUSBoard::getSensorData1To4(int *iSensorDistCM)
{
	int i;
	int iSensorState;
		//m_Mutex.lock();

		for(i = 0; i < 4; i++)
		{
			iSensorDistCM[i] = m_iSensorData1To4[i];
		}
		//iSensorState = m_iSensorStatus1To4;

		//m_Mutex.unlock();

		return 0;
}


int SerUSBoard::getSensorData5To8(int *iSensorDistCM)
{
	int i;
	int iSensorState;
		//m_Mutex.lock();

		for(i = 0; i < 4; i++)
		{
			iSensorDistCM[i] = m_iSensorData5To8[i];
		}
		//iSensorState = m_iSensorStatus5To8;

		//m_Mutex.unlock();

		return 0;
}

int SerUSBoard::getSensorData9To12(int *iSensorDistCM)
{
	int i;
	int iSensorState;
		//m_Mutex.lock();

		for(i = 0; i < 4; i++)
		{
			iSensorDistCM[i] = m_iSensorData9To12[i];
		}
		//iSensorState = m_iSensorStatus9To12;

		//m_Mutex.unlock();

		return 0;
}

int SerUSBoard::getSensorData13To16(int *iSensorDistCM)
{
	int i;
	int iSensorState;
		//m_Mutex.lock();

		for(i = 0; i < 4; i++)
		{
			iSensorDistCM[i] = m_iSensorData13To16[i];
		}
		//iSensorState = m_iSensorStatus13To16;

		//m_Mutex.unlock();

		return 0;
}

int SerUSBoard::getAnalogInCh1To4Data(int *iAnalogInCh1To4Data)
{
	int i;
	int iAnalogInCh1To4HighBits[4];
		//m_Mutex.lock();

	iAnalogInCh1To4HighBits[0] = m_iAnalogInDataCh1To4HighBits[0] & 0x0f;
	iAnalogInCh1To4HighBits[1] = (m_iAnalogInDataCh1To4HighBits[0] & 0xf0) >> 4;
	iAnalogInCh1To4HighBits[2] = m_iAnalogInDataCh1To4HighBits[1] & 0x0f;
	iAnalogInCh1To4HighBits[3] = (m_iAnalogInDataCh1To4HighBits[1] & 0xf0) >> 4;

	for(i = 0; i < 4; i++)
		{
			iAnalogInCh1To4Data[i] = (iAnalogInCh1To4HighBits[i] << 8) | m_iAnalogInDataCh1To4LowByte[i];
		}

		//m_Mutex.unlock();

		return 0;
}


//-----------------------------------------------

void SerUSBoard::enable_logging()
{
	logging = true;
}

void SerUSBoard::disable_logging()
{
	logging = false;
}

//-----------------------------------------------

void SerUSBoard::log_to_file(int direction, unsigned char cMsg[])
{
	FILE * pFile;

	//Open Logfile
	pFile = fopen ("neo_usboard_RX_TX_log.log","a");
	//Write Data to Logfile
	if(direction == 1)
	{
		fprintf (pFile, "\n\n Direction: %i", direction);
		for(int i=0; i<NUM_BYTE_SEND_USBOARD_08; i++)
			fprintf(pFile," %.2x", cMsg[i]);
		fprintf(pFile,"\n");

	}
	if(direction == 2)
	{
		fprintf (pFile, "\n\n Direction: %i", direction);
		for(int i=0; i<NUM_BYTE_REC_USBOARD_11; i++)
			fprintf(pFile," %.2x", cMsg[i]);
		fprintf(pFile,"\n");
	}
	//Close Logfile
	fclose (pFile);
}

//-----------------------------------------------

void SerUSBoard::convDataToSendMsg(unsigned char cMsg[])
{
	int i;
	int iCnt = 0;

	if(m_iCmdUSBoard == CMD_CONNECT)
	   {
		cMsg[iCnt++] = 0;
		 do
		 {
		   cMsg[iCnt++] = 0;
	         }
		 while(iCnt < (m_iNumBytesSend - 1));
	   }


	if(m_iCmdUSBoard == CMD_SET_CHANNEL_ACTIVE)
		   {
			cMsg[iCnt++] = 1;
			cMsg[iCnt++]= 0xFF; // Activate channels 1 to 8
			cMsg[iCnt++]= 0xFF; // Activate channels 9 to 16
			 do
			 {
			   cMsg[iCnt++] = 0;
		         }
			 while(iCnt < m_iNumBytesSend-1);
		   }


	 if(m_iCmdUSBoard == CMD_GET_DATA_1TO8)
		   {
			cMsg[iCnt++] = 0x02;
			 do
			 {
			   cMsg[iCnt++] = 0;
		         }
			 while(iCnt < (m_iNumBytesSend - 1));
		   }
	 if(m_iCmdUSBoard == CMD_GET_DATA_9TO16)
		   {
			cMsg[iCnt++] = 3;
			 do
			 {
			   cMsg[iCnt++] = 0;
		         }
			 while(iCnt < (m_iNumBytesSend -1));
		   }
	 if(m_iCmdUSBoard == CMD_GET_ANALOGIN)
		   {
			cMsg[iCnt++] = 7;
			 do
			 {
			   cMsg[iCnt++] = 0;
		         }
			 while(iCnt < (m_iNumBytesSend - 1));
		   }
/*
 if(m_iCmdUSBoard == CMD_READ_PARASET)
		   {
			cMsg[iCnt++] = 6;
			 do
			 {
			   cMsg[iCnt++] = 0;
		     }
			 while(iCnt < (m_iNumBytesSend - 1));
		   }

 if(m_iCmdUSBoard == CMD_SET_DEBUG_PARA)
		   {
			cMsg[iCnt] = 8;
			 do
			 {
			   cMsg[iCnt++] = 0;
		     }
			 while(iCnt < (m_iNumBytesSend-1));
		   }
 if(m_iCmdUSBoard == CMD_GET_DEBUG_PARA)
		   {
			cMsg[iCnt] = 9;
			 do
			 {
			   cMsg[iCnt++] = 0;
		     }
			 while(iCnt < (m_iNumBytesSend - 1));
		   }
 if(m_iCmdUSBoard == CMD_UNKNOWN)
		   {
			cMsg[iCnt] = 10;
			 do
			 {
			   cMsg[iCnt++] = 0;
		     }
			 while(iCnt < (m_iNumBytesSend - 1));
		   }*/

}

//-----------------------------------------------

bool SerUSBoard::convRecMsgToData(unsigned char cMsg[])
{
	unsigned int iNumByteRec = NUM_BYTE_REC;

		int i;
		unsigned int iTxCheckSum;
		unsigned int iCheckSum;

	//	m_Mutex.lock();

		// test checksum
		iCheckSum = getCheckSum(cMsg, iNumByteRec);

		iTxCheckSum = (cMsg[iNumByteRec]<<8)|(cMsg[iNumByteRec+1]);

		if(iCheckSum != iTxCheckSum)
		{
			return false;
		}
		
		// convert data
		int iCnt = 0;

		m_iCmdUSBoard= cMsg[iCnt];

		if(m_iCmdUSBoard == CMD_CONNECT)
		{

			for(i=0; i<8; i++)
			{
				iCnt+=1;
				m_iCmdConnectAns[i]= cMsg[iCnt];
			}
			return true;
		}

/*
		if(m_iCmdUSBoard == CMD_READ_PARASET)
		{
                        iCnt+=1;
			m_iReadAnsFormat = cMsg[iCnt];
			if(m_iReadAnsFormat == 1)
			{
			        iCnt+=1;
				m_iTransMode = cMsg[iCnt];
				return true;
			}
		}
*/
		if(m_iCmdUSBoard == CMD_GET_DATA_1TO8)
		{
			iCnt+=1;
			m_iReadAnsFormat = cMsg[iCnt];

			if(m_iReadAnsFormat == 0 )
			{
				for(i=0; i<4;i++)
				{
					iCnt+=1;
					m_iSensorData1To4[i] = cMsg[iCnt];

				}
				//m_iSensorAcc1To4 = cMsg[iCnt+1];
				//m_iSensorStatus1To4 = cMsg[iCnt+2];
				return true;
			}

			else if(m_iReadAnsFormat == 1 )
			{
				for(i=0; i<4;i++)
				{
					iCnt+=1;
					m_iSensorData5To8[i] = cMsg[iCnt];

				}
				//m_iSensorAcc5To8 = cMsg[iCnt+1];
				//m_iSensorStatus5To8 = cMsg[iCnt+2];
			    return true;
			}

		}

		if(m_iCmdUSBoard == CMD_GET_DATA_9TO16)
		{
			iCnt+=1;
			m_iReadAnsFormat = cMsg[iCnt];

			if(m_iReadAnsFormat == 0 )
			{
		    	for(i=0; i<4;i++)
				{
		    		     iCnt+=1;
				     m_iSensorData9To12[i] = cMsg[iCnt];
				}
				//m_iSensorAcc9To12 = cMsg[iCnt+1];
				//m_iSensorStatus9To12 = cMsg[iCnt+2];
			    return true;
			}

			else if(m_iReadAnsFormat == 1 )
			{
				for(i=0; i<4;i++)
				{
					iCnt+=1;
					m_iSensorData13To16[i] = cMsg[iCnt];
				}
				//m_iSensorAcc13To16 = cMsg[iCnt+1];
				//m_iSensorStatus13To16 = cMsg[iCnt+2];
			    return true;
			}
		 }

		if(m_iCmdUSBoard == CMD_GET_ANALOGIN)
		{

			for(i=0; i<4; i++)
			{
				iCnt+=1;
				m_iAnalogInDataCh1To4LowByte[i] = cMsg[iCnt];
			}
			    m_iAnalogInDataCh1To4HighBits[0] = cMsg[iCnt+1];
			    m_iAnalogInDataCh1To4HighBits[1] = cMsg[iCnt+2];

			    return true;
		}


	   // m_Mutex.unlock();

			if( iCnt >= NUM_BYTE_REC_MAX )
			{
				OUTPUTERROR("msg size too small");

			}
			return true;

}

//-----------------------------------------------

unsigned int SerUSBoard::getCheckSum(unsigned char* cMsg, int iNumBytes)
{
   unsigned int uCrc16;
   unsigned char ucData[2];

   uCrc16=0;
   ucData[0]=0;

   while(iNumBytes--)
   {
	   ucData[1]= ucData[0];
	   ucData[0] = *cMsg++;

	   if(uCrc16 & 0x8000)
	   {
		   uCrc16 = (uCrc16 & 0x7fff)<<1;
		   uCrc16^= 0x1021; //generator polynomial
	   }
	   else
	   {
		   uCrc16 <<= 1;
	   }
       uCrc16^= (unsigned int)(ucData[0])|((unsigned int)(ucData[1]) << 8);
   }
   return uCrc16;
}


/*
void SerUSBoard::USBoardConfig (int CAN_BAUD_RATE,long CAN_ADD, int CAN_EXT_ID, int TRANS_MODE,
		int TRANS_INT, int SENSORS_1TO9, int SENSORS_9TO16, int WARN_DIST_1TO8[7], int WARN_DIST_9TO16[7],
		int SENSOR_SENS, int FIRING_SEQ[7], long BOARD_ID)
{
	m_iCAN_BAUDRATE=CAN_BAUD_RATE;
	m_iCAN_ADD=CAN_ADD;
	m_iCAN_EXT_ID= CAN_EXT_ID;
	m_iTRANS_MODE= TRANS_MODE;
	m_iTRANS_INT= TRANS_INT;
	m_iSENSORS_1TO9= SENSORS_1TO9;
	m_iSENSORS_9TO16 = SENSORS_9TO16;

	for(int i=0; i<8; i++)
	{
		m_iWARN_DIST_1TO8[i]= WARN_DIST_1TO8[i];
	}

	for(int i=0; i<8; i++)
	{
		m_iWARN_DIST_9TO16[i]= WARN_DIST_9TO16[i];
	}

	m_iSENSOR_SENS = SENSOR_SENS;

	for( int j=0; j<8; j++)
	{
		m_iFIRING_SEQ[j]= FIRING_SEQ[j];
	}

	m_iBOARD_ID = BOARD_ID;
}


//-----------------------------------------------

int SerUSBoard::getTransModeData()
{
 if(m_iCmdUSBoard == CMD_READ_PARASET && m_iReadAnsFormat == 1 ) return m_iTransMode;
 else return SEND_ON_REQ;

}

//-----------------------------------------------
*/
