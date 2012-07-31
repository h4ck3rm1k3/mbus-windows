//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// FreeSCADA 
// http://www.FreeSCADA.com
// freescada@freescada.com
//
//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <mbus/mbus.h>

//------------------------------------------------------------------------------
// Primary addressing scanning of mbus devices.
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_handle *handle;
    char *device;
    int address, baudrate = 2400;

    fprintf(stderr, "Hello, welcome to mbus\n");

    if (argc == 2)
    {
        device = argv[1];
    }
    else if (argc == 4 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atoi(argv[2]); 
        device = argv[3];
    }
    else
    {
        fprintf(stderr, "usage: %s [-b BAUDRATE] device\n", argv[0]);
        return 0;
    }

    fprintf(stderr, "Going to use device %s at baud %d \n", device, baudrate);
    


    if ((handle = mbus_connect_serial(device)) == NULL)
    {
        printf("Failed to setup connection to M-bus gateway\n");
        return -1;

    }

    if (mbus_serial_set_baudrate(handle->m_serial_handle, baudrate) == -1)
    {
        printf("Failed to set baud rate.\n");
        return -1;
    }



    for (address = 0; address < 254; address++)
    {
        mbus_frame reply;

        memset((void *)&reply, 0, sizeof(mbus_frame));

        if (mbus_send_ping_frame(handle, address) == -1)
        {
	  printf("Scan failed. Could not send ping frame to handle %d and address %d: %s\n", handle, address, mbus_error_str());
	  continue;
	    //            return -1;
        } 

        if (mbus_recv_frame(handle, &reply) == -1)
        {

            printf("No reply address %d\n", address);

            continue;
        } 
	else  {
	      printf("Got a reply\n");
	      int n=0;
	      const char * c=(const char * )&reply;
	      //	      for(;n < sizeof(mbus_frame)-1; c++)		{	       
	      for(;n < 10; c++)		{	       
		printf("char %d %d\n",n,*c);
		n++;
	      }
	}

	int frame_type= mbus_frame_type(&reply);
        if (frame_type == MBUS_FRAME_TYPE_ACK)
        {
            printf("Found a M-Bus device at address %d \n", address);
        }
	else 
	{
	  printf("not found addr:%d got frame type %d\n", address, frame_type);
	}
    }

    mbus_disconnect(handle);
    return 0;
}

