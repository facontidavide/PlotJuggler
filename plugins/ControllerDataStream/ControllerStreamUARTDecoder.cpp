#include "ControllerStreamUARTDecoder.h"
#include <cstdio>
#include <functional>

#define CALL_MEMBER(object,ptrToMember)  ((object).*(ptrToMember))


ControllerStreamUartDecoder::ControllerStreamUartDecoder()
{
	currentState = &ControllerStreamUartDecoder::synchroLookUp;
}


void ControllerStreamUartDecoder::processBytes(uint8_t *buffer, unsigned int nbBytes)
{
	for(int i=0; i<nbBytes; i++)
		CALL_MEMBER(*this,currentState)(buffer[i]);
}


void ControllerStreamUartDecoder::synchroLookUp(uint8_t byte)
{
	constexpr uint16_t synchroWord = 0x5A5A;
	static int nbSynchroByteFound = 0;

	if( byte == ((uint8_t*)&synchroWord)[nbSynchroByteFound] )
	{
		nbSynchroByteFound++;
	}
	else
	{
		isCurrentSampleValid = false;
		nbSynchroByteFound = 0;
//		printf("drop..\n");
	}

	if( nbSynchroByteFound == sizeof(synchroWord))  // Synchro found !
	{
		nbSynchroByteFound = 0;
		currentState =  &ControllerStreamUartDecoder::getRemainingData;

		if( isCurrentSampleValid)
			decodedSampleQueue.push(currentDecodedSample);
	}
}

void ControllerStreamUartDecoder::getRemainingData(uint8_t byte)
{
	static int nbByteRead = 0;
	static uint8_t *currentDecodedSamplePtr = (uint8_t*)&currentDecodedSample;
	currentDecodedSamplePtr[nbByteRead++] = byte;

	if(nbByteRead == sizeof(currentDecodedSample))
	{
		nbByteRead = 0;
		currentState =  &ControllerStreamUartDecoder::synchroLookUp;
		isCurrentSampleValid = true;
	}
}

bool ControllerStreamUartDecoder::getDecodedSample(StreamSample *sample)
{
	if(decodedSampleQueue.empty() || sample == NULL )
		return false;
	*sample = decodedSampleQueue.front();
	decodedSampleQueue.pop();
	return true;
}