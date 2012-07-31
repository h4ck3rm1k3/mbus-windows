//------------------------------------------------------------------------------
// Copyright (C) 2012, Alexander Rössler
// All rights reserved.
//
//
//------------------------------------------------------------------------------ 

#include <windows.h>
#include <commctrl.h>

#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <strings.h>

#include <errno.h>
#include <string.h>

#include <mbus/mbus.h>
#include <mbus/mbus-serial-windows.h>

#define PACKET_BUFF_SIZE 2048

//------------------------------------------------------------------------------
/// Set up a serial connection handle.
//------------------------------------------------------------------------------
mbus_serial_handle *
mbus_serial_connect(char *device)
{
    mbus_serial_handle *handle;
    char buffer[100];
    
    if (device == NULL)
    {
        return NULL;
    }
    
    if ((handle = (mbus_serial_handle *)malloc(sizeof(mbus_serial_handle))) == NULL)
    {
        fprintf(stderr, "%s: failed to allocate memory for handle\n", __PRETTY_FUNCTION__);
        return NULL;
    }
    
    handle->device = buffer;
    
    //map unix serial device names to windows serial device names
    if (strcmp(device, "/dev/ttyS0") == 0)
      strcpy(handle->device, "COM1");
    else if (strcmp(device, "/dev/ttyS1") == 0)
      strcpy(handle->device, "COM2");
    else if (strcmp(device, "/dev/ttyS2") == 0)
      strcpy(handle->device, "COM3");
    else if (strcmp(device, "/dev/ttyS3") == 0)
      strcpy(handle->device, "COM4");
    else
      strcpy(handle->device, device);

    fprintf(stderr, "Going to use device %s\n", handle->device);

    
    //
    // create a SERIAL connection
    //
    handle->hSerial = CreateFile(handle->device,
                                 GENERIC_READ | GENERIC_WRITE, 
                                 0, 
                                 0,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 0);
    if (handle->hSerial == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            fprintf(stderr, "%s: serial port not found.\n", __PRETTY_FUNCTION__);
            return NULL;
        }
        fprintf(stderr, "%s: failed to open serial device.\n", __PRETTY_FUNCTION__);
        return NULL;
    }
    else {
      fprintf(stderr, "device opened %s\n", handle->device);
    }
    
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength=sizeof(dcbSerialParams);
    
    if (!GetCommState(handle->hSerial, &dcbSerialParams))
    {
        fprintf(stderr, "%s: error getting state.\n", __PRETTY_FUNCTION__);
        return NULL;

    }
    
    dcbSerialParams.BaudRate = CBR_2400;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = EVENPARITY;
    
    if (!SetCommState(handle->hSerial, &dcbSerialParams))
    {
        fprintf(stderr, "%s: error setting serial port state.\n", __PRETTY_FUNCTION__);
        return NULL;
    }
    
    return handle;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int
mbus_serial_set_baudrate(mbus_serial_handle *handle, int baudrate)
{
    if (handle == NULL)
        return -1;

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength=sizeof(dcbSerialParams);
    
    if (!GetCommState(handle->hSerial, &dcbSerialParams))
    {
        fprintf(stderr, "%s: error getting state.\n", __PRETTY_FUNCTION__);
        return -1;
    }
    
    switch (baudrate)
    {
        case 300:
            dcbSerialParams.BaudRate = CBR_300;
            break;

        case 1200:
            dcbSerialParams.BaudRate = CBR_1200;
            break;

        case 2400:
            dcbSerialParams.BaudRate = CBR_2400;
            break;

        case 9600:
            dcbSerialParams.BaudRate = CBR_9600;
            break;

       default:
       return -1; // unsupported baudrate
    }
    
    if (!SetCommState(handle->hSerial, &dcbSerialParams))
    {
        fprintf(stderr, "%s: error setting serial port state.\n", __PRETTY_FUNCTION__);
        return -1;
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_disconnect(mbus_serial_handle *handle)
{
    if (handle == NULL)
    {
        return -1;
    }

    CloseHandle(handle->hSerial);
    
    free(handle);

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_send_frame(mbus_serial_handle *handle, mbus_frame *frame)
{
    u_char buff[PACKET_BUFF_SIZE];
    DWORD dwBytesWritten = 0;
    int len, ret;
    
    if (handle == NULL || frame == NULL)
    {
        return -1;
    }
    
    if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1)
    {
        fprintf(stderr, "%s: mbus_frame_pack failed\n", __PRETTY_FUNCTION__);
        return -1;
    }

    fprintf(stderr, "mbus_serial_send_frame handle %x: buff:%s len:%d\n", handle->hSerial, buff, len);

    // DUMP
    int n=0;
    const char * c=(const char * )&buff;
    for(;n < len; c++)		{	       
      printf("SEND %d %x\n",n,*c);
      n++;
    }
    ///
    
    
    if (!WriteFile(handle->hSerial, buff, len, &dwBytesWritten, NULL))
    {
        fprintf(stderr, "%s: bytes not written\n", __PRETTY_FUNCTION__);
        return -1;
    }
    else if (dwBytesWritten != len)
    {
        char lastError[1024];
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            lastError,
            1024,
            NULL);
        fprintf(stderr, "%s: Failed to write frame to socket: \n", __PRETTY_FUNCTION__, lastError);
        return -1;
    }
    else {
      fprintf(stderr, "%s: bytes written %d\n", __PRETTY_FUNCTION__, len);
    }

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_recv_frame(mbus_serial_handle *handle, mbus_frame *frame)
{
    char buff[PACKET_BUFF_SIZE];
    DWORD dwBytesRead;
    int len, remaining, nread;

    memset((void *)buff, '\0', sizeof(buff));
    
    //
    // read data until a packet is received
    //
    remaining = 1; // start by reading 1 byte
    len = 0;
    
    fprintf(stderr, "%s: Read Started.\n", __PRETTY_FUNCTION__);
    do {

      fprintf(stderr, "%s: Read Loop.\n", __PRETTY_FUNCTION__);

        if (!ReadFile(handle->hSerial, &buff[len], remaining, &dwBytesRead, NULL))
        {
	  fprintf(stderr, "%s: ReadFile Failed.\n", __PRETTY_FUNCTION__);

            return -1;
        }
	else
	  {
	    fprintf(stderr, "%s: Read OK %d.\n", __PRETTY_FUNCTION__,dwBytesRead);
	  }
        
	// DUMP
	int n=0;
	const char * c=(const char * )&buff[len];
	for(;n < dwBytesRead; c++)		{	       
	  printf("read %d %x\n",n,*c);
	  n++;
	}
    ///-------------------

        len += dwBytesRead;
        
    } while ((remaining = mbus_parse(frame, buff, len)) > 0);
    
    if (len == -1)
    {
        fprintf(stderr, "%s: M-Bus layer failed to parse data.\n", __PRETTY_FUNCTION__);
        return -1;
    }
    
    return 0;
}
