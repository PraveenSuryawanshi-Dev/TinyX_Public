/************************************************************
*   TinyX Operating System									*
*   @author : Praveen Suresh Suryawanshi					*
*   @mail : suryawanshi.praveen007@gmail.com                *
*************************************************************/


/*H******************************************************
*
  header declaration. 
  http://syque.com/cstyle/ch4.5.htm
*
*H*/

#include <stdio.h>
#include <fcntl.h>
#include<malloc.h>
#include<math.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<time.h>
#include <thread>
#include<iostream>

using namespace std;

#define BLOCK_SIZE 1024

char *device_location = "hd0.bin";

#define BOOT_BLOCK_OFFSET  0
#define SUPER_BLOCK_OFFSET BOOT_BLOCK_OFFSET + sizeof(SUPER_BLOCK)
#define INODE_BLOCK_OFFSET SUPER_BLOCK_OFFSET + BLOCK_SIZE
#define DATA_BLOCK_OFFSET  (ceil((int)(MAX_INODES / (int)((BLOCK_SIZE) / sizeof(DISK_INODE)))) * BLOCK_SIZE) + INODE_BLOCK_OFFSET


/************************************************************/
/*  DISK DEVICE DRIVER										*/
/*  simple 'C' implementation of Disk Device Driver			*/
/*	simulation	using   File IO.		                    */
/************************************************************/

/*/
* global variable for storing current buffer pointer , or current offset of disk arm.
*/

static int  g_current_arm_position = 0;
/**/

/*--------------------------------------------------------------
*   typedef struct _device_ stores the device identification
*	information
*/

typedef struct _device_
{
	char major; /*keeps the major number of device identification*/
	char minor;	/*keeps the minor number of device identification*/

	FILE *fp;  /*keeps the file pointer for above device*/
} DEVICE;

/*--------------------------------------------------------------
*   typedef struct _device_ stores the device identification
*	information
*	packet data to write on device.
*/

typedef struct _packet_
{
	DEVICE device;	/*on which deive to write the packet*/
	int block;		/*on that device which block to write*/
	char *data;		/*data to write on that device*/
} PACKET;

/*******************************************************************
* NAME :           move_arm_of_hardisk(DEVICE device, int block_no)
*
* DESCRIPTION :     Moves the Arm of Disk ie. increment and decrement tthe FILE pointer.
*
* INPUTS :
*       PARAMETERS:
*           DEVICE     device                device on to move the file io operation.
*			int block_no					 on which block to move the file pointer.
* OUTPUTS : move the file pointer to block.
* NOTES :         
*/

void move_arm_of_hardisk(DEVICE device, int block_no)
{
	int status = 1;
	int where_to_move_arm_hand = 0;
	int no_of_bytes_to_move = 0;

	where_to_move_arm_hand = block_no * BLOCK_SIZE;

	if (g_current_arm_position != where_to_move_arm_hand)
	{
		no_of_bytes_to_move = where_to_move_arm_hand - g_current_arm_position;
		status =  fseek(device.fp, no_of_bytes_to_move, SEEK_CUR);
		
		if (status != 0)
		{
			printf("\nError : failed to seek  block %d  on [ major number :  %d  minor number : %d ]", block_no, device.major, device.minor);
			exit(0);
		}

		g_current_arm_position = ftell(device.fp);
		return;
	}
}

/*******************************************************************
* NAME :           controller_write(DEVICE device, int block, char *data)
*
* DESCRIPTION :     write the data to block
*
* INPUTS :
*       PARAMETERS:
*           DEVICE     device                device on to move the file io operation.
*			int block_no					 on which block to move the file pointer.
*			char *data						 data to write on the block
* OUTPUTS : write the data to data block.
* NOTES :
*/

void controller_write(DEVICE device, int block, char *data)
{
	int status = 1;
	int no_of_bytes_written = 0;

	move_arm_of_hardisk(device, block);

	no_of_bytes_written = fwrite( data, strlen(data), strlen(data),device.fp);

	if (no_of_bytes_written == 0)
	{
		printf("\nError : failed to write  block %d  on [ major number :  %d  minor number : %d ]", block, device.major, device.minor);
		printf("\ncurrent disk arm hand position is : %d ", g_current_arm_position);
		exit(0);
		return;
	}


	if (no_of_bytes_written != BLOCK_SIZE)
	{
		status = fseek(device.fp, (BLOCK_SIZE - no_of_bytes_written), SEEK_CUR);

		if (status != 0)
		{
			printf("\nError : failed to seek  block %d  on [ major number :  %d  minor number : %d ]", block, device.major, device.minor);
			exit(0);
		}
	}

	g_current_arm_position = ftell(device.fp);
}

/*******************************************************************
* NAME :           controller_read(DEVICE device, int block, char *data)
*
* DESCRIPTION :     read the data to block
*
* INPUTS :
*       PARAMETERS:
*           DEVICE     device                device on to move the file io operation.
*			int block_no					 on which block to move the file pointer.
*			char *data						 data to write on the block
* OUTPUTS : reads the block and fill in *data.
* NOTES :
*/

void controller_read(DEVICE device, int block, char *data)
{

	int no_of_bytes_readed = 0;

	move_arm_of_hardisk(device, block);

	no_of_bytes_readed = fread( data, BLOCK_SIZE, BLOCK_SIZE,device.fp);

	if (no_of_bytes_readed == 0)
	{
		printf("\nError : failed to read  block %d  on [ major number :  %d  minor number : %d ]", block, device.major, device.minor);
		printf("\ninfo: current disk arm hand position is : %d ", g_current_arm_position);
		exit(0);
		return;
	}


	if (no_of_bytes_readed != BLOCK_SIZE)
	{
		printf("\ninfo: bytes read  %d", no_of_bytes_readed);
	}

}

/*******************************************************************
* NAME :           run_async_controller_write(void *args)
*
* DESCRIPTION :     write the data to block asynchronously
*
* INPUTS :
*       PARAMETERS:
*           void *args thread data parameters type of (PACKET*)args
* OUTPUTS : write the data async.
* NOTES :
*/

void* run_async_controller_write(void *args)
{
	PACKET *packet = { 0 };

	packet = (PACKET*)args;
	controller_write(packet->device, packet->block, packet->data);
	
	return 0;
}

/*******************************************************************
* NAME :           async_controller_write_initialization(void *args)
*
* INPUTS :
*       PARAMETERS:
*           DEVICE     device                device on to move the file io operation.
*			int block_no					 on which block to move the file pointer.
*			char *data						 data to write on the block
* OUTPUTS : intialization and write the block.
* NOTES :
*/

void async_controller_write_initialization(DEVICE device, int block, char *data)
{

	PACKET packet;
	packet.device.major = device.major;
	packet.device.minor = device.minor;
	packet.device.fp = device.fp;
	packet.block = block;
	packet.data = data;


	std::thread thread(&run_async_controller_write, &packet);
}

/*******************************************************************
* NAME :           run_async_controller_read(void *args)
*
* DESCRIPTION :     read the data to block asynchronously
*
* INPUTS :
*       PARAMETERS:
*           void *args thread data parameters type of (PACKET*)args
* OUTPUTS : read the data async.
* NOTES :
*/

void *run_async_controller_read(void *args)
{

	PACKET *packet = { 0 };

	packet = (PACKET*)args;
	controller_read(packet->device, packet->block, packet->data);
	return 0;
}

/*******************************************************************
* NAME :           async_controller_read_initialization(DEVICE device, int block, char *data)
*
* DESCRIPTION :     read the data to block asynchronously intialization
* INPUTS :
*       PARAMETERS:
*           DEVICE     device                device on to move the file io operation.
*			int block_no					 on which block to move the file pointer.
*			char *data						 data to read on the block
* OUTPUTS : read the block asynchronously.
* NOTES :
*/

void async_controller_read_initialization(DEVICE device, int block, char *data)
{

	PACKET packet;
	packet.device.major = device.major;
	packet.device.minor = device.minor;
	packet.device.fp = device.fp;
	packet.block = block;
	packet.data = data;

	std::thread thread(&run_async_controller_read, &packet);
}

/*---- DISK DEVICE DRIVER -----------------------------------------
*   end of implementation of Disk Device Driver.
*   NOTE : it just a simulation of Disk Device Driver using simple 'C' program.
*-------------------------------------------------------------------*/





/************************************************************/
/*  BUFFER CACHE											*/
/*  simple 'C' implementation of buffer cache Driver		*/
/*	simulation	using   doubly circular link list.		    */
/************************************************************/



#define MAX_DISKBLOCK_BUFFER 50
#define MAX_DISKBLOCK_BUFFER_HEADER 4

typedef enum _buffer_status_
{
	BUFFER_LOCKED,
	BUFFER_UNLOCKED,
	BUFFER_DELAY_WRITE,
	BUFFER_READ,
	BUFFER_WRITE,
	BUFFER_INVALID,
	BUFFER_VALID,
	BUFFER_ERROR,
	BUFFER_RESERVED,
	BUFFER_INDEMAND,
	BUFFER_OLD

} BUFFER_STATUS;

/*--------------------------------------------------------------
*   typedef struct _diskblock_buffer_ stores the file  identification
*	information
*/

typedef struct _diskblock_buffer_
{
	/*part 1 header*/
	char data[BLOCK_SIZE]; /*data of file from disk to memory*/

	/*part 2 header DISKBLOCK_BUFFER of below is the auxillary data*/

	unsigned int mod;		/*where the block is kept it buffer cache*/
	DEVICE device;			/*which device to do operation*/
	unsigned int block;		/*which block*/
	BUFFER_STATUS status;	/*status of buffer cache*/


	/*pointer for linkist next and previous buffer linking*/
	struct _diskblock_buffer_ *diskblock_next_buffer;
	struct _diskblock_buffer_ *diskblock_previous_buffer;

	/*linking those buffer only which are free on buffer cache*/
	struct _diskblock_buffer_ *diskblock_freelist_next_buffer;
	struct _diskblock_buffer_ *diskblock_freelist_previous_buffer;

} DISKBLOCK_BUFFER;

DISKBLOCK_BUFFER *diskblock_freelist = NULL;

DISKBLOCK_BUFFER diskblock_buffer[MAX_DISKBLOCK_BUFFER];

DISKBLOCK_BUFFER diskblock_mod_buffer[MAX_DISKBLOCK_BUFFER_HEADER];

/*******************************************************************
* NAME :           put_buffer_to_respective_mod(DISKBLOCK_BUFFER *buffer)
*
* DESCRIPTION :    put's the respective buffer to hash queue
* INPUTS :
*       PARAMETERS:
*           DISKBLOCK_BUFFER     *buffer to put in hash queue.
* OUTPUTS : modify the hash queue buffer pool.
* NOTES :
*/

void put_buffer_to_respective_mod(DISKBLOCK_BUFFER *buffer)
{
	int _goto = 0;
	DISKBLOCK_BUFFER *last = NULL;

	_goto = buffer->block % MAX_DISKBLOCK_BUFFER_HEADER;

	last = diskblock_mod_buffer[_goto].diskblock_previous_buffer;

	if (diskblock_mod_buffer[_goto].diskblock_next_buffer == NULL)
	{

		diskblock_mod_buffer[_goto].diskblock_next_buffer = buffer;
		buffer->diskblock_previous_buffer = &diskblock_mod_buffer[_goto];


		buffer->diskblock_next_buffer = &diskblock_mod_buffer[_goto];
		diskblock_mod_buffer[_goto].diskblock_previous_buffer = buffer;
	}

	else
	{
		last->diskblock_next_buffer = buffer;
		buffer->diskblock_previous_buffer = last;

		buffer->diskblock_next_buffer = &diskblock_mod_buffer[_goto];
		diskblock_mod_buffer[_goto].diskblock_previous_buffer = buffer;

	}

}

/*******************************************************************
* NAME :           put_buffer_to_freelist(DISKBLOCK_BUFFER *buffer)
*
* DESCRIPTION :    put's the respective buffer to free list
* INPUTS :
*       PARAMETERS:
*           DISKBLOCK_BUFFER     *buffer  put to freelist in hash queue.
* OUTPUTS : modify the free list.
* NOTES :
*/

void put_buffer_to_freelist(DISKBLOCK_BUFFER *buffer)
{
	DISKBLOCK_BUFFER *last = NULL;
	if (diskblock_freelist == NULL)
	{
		diskblock_freelist = buffer;
		buffer->diskblock_freelist_next_buffer = diskblock_freelist;
		buffer->diskblock_freelist_previous_buffer = diskblock_freelist;
	}
	else
	{
		last = diskblock_freelist->diskblock_freelist_previous_buffer;

		last->diskblock_freelist_next_buffer = buffer;
		buffer->diskblock_freelist_previous_buffer = last;

		buffer->diskblock_freelist_next_buffer = diskblock_freelist;
		diskblock_freelist->diskblock_freelist_previous_buffer = buffer;
	}

}

void init_hash_queue_pool()
{
	int i = 0;

	/*initialization for buffer all
	hash queue header*/
	for (i = 0; i < MAX_DISKBLOCK_BUFFER_HEADER; i++)
	{
		diskblock_mod_buffer[i].mod = i;

		diskblock_mod_buffer[i].status = BUFFER_RESERVED;
		diskblock_mod_buffer[i].block = 0;
		memset(diskblock_mod_buffer[i].data, 0, BLOCK_SIZE);

		diskblock_mod_buffer[i].device.major = '0';
		diskblock_mod_buffer[i].device.minor = '0';
		diskblock_mod_buffer[i].device.fp = NULL;

		diskblock_mod_buffer[i].diskblock_freelist_next_buffer = NULL;
		diskblock_mod_buffer[i].diskblock_freelist_previous_buffer = NULL;

		diskblock_mod_buffer[i].diskblock_next_buffer = NULL;
		diskblock_mod_buffer[i].diskblock_previous_buffer = NULL;
	}


	/*initialization for all disk block buffer*/
	for (i = 0; i < MAX_DISKBLOCK_BUFFER; i++)
	{
		diskblock_buffer[i].mod = 0;

		diskblock_buffer[i].status = BUFFER_UNLOCKED;
		diskblock_buffer[i].block = i;

		memset(diskblock_buffer[i].data, 0, BLOCK_SIZE);

		diskblock_buffer[i].device.major = '0';
		diskblock_buffer[i].device.minor = '0';
		diskblock_buffer[i].device.fp = NULL;

		diskblock_buffer[i].diskblock_freelist_next_buffer = NULL;
		diskblock_buffer[i].diskblock_freelist_previous_buffer = NULL;

		diskblock_buffer[i].diskblock_next_buffer = NULL;
		diskblock_buffer[i].diskblock_previous_buffer = NULL;

	}

	for (i = 0; i < MAX_DISKBLOCK_BUFFER; i++)
	{
		put_buffer_to_respective_mod(&diskblock_buffer[i]);

	}

	for (i = 0; i < MAX_DISKBLOCK_BUFFER; i++)
	{
		put_buffer_to_freelist(&diskblock_buffer[i]);

	}

}

/*******************************************************************
* NAME :           display_diskblock_hashpool(void)
*
* DESCRIPTION :   display the buffer cache pool list
* INPUTS :	void
* OUTPUTS : prints the buffer pool list.
* NOTES :
*/

void display_diskblock_hashpool(void)
{

	int i = 0;
	DISKBLOCK_BUFFER *temp = NULL;

	printf("\n\n\nBUFFER CACHE POOL\n");
	for (i = 0; i < MAX_DISKBLOCK_BUFFER_HEADER; i++)
	{
		temp = diskblock_mod_buffer[i].diskblock_next_buffer;

		printf("\n[MOD %d]<-->", i);
		do
		{
			if (diskblock_mod_buffer[i].diskblock_next_buffer == NULL) 
				break;

			printf("[%d]<-->", temp->block);
			temp = temp->diskblock_next_buffer;

		} while (temp != &diskblock_mod_buffer[i]);
		printf("[MOD %d]\n", i);

	}
}

/*******************************************************************
* NAME :           display_diskblock_freelist(void)
*
* DESCRIPTION :   display the free buffer pool list
* INPUTS :	void
* OUTPUTS : prints the free buffer pool list.
* NOTES :
*/
void display_diskblock_freelist(void)
{
	DISKBLOCK_BUFFER *temp = diskblock_freelist;
	printf("\n\n\nBUFFER CACHE FREE LIST\n");
	printf("\nFREELIST-->");
	do
	{
		if (temp == NULL) break;

		printf("[%d]<-->", temp->block);
		temp = temp->diskblock_freelist_next_buffer;
	} while (temp != diskblock_freelist);
	printf("FREELIST\n");

}

DISKBLOCK_BUFFER * serach_block_in_hash_queue(DEVICE *device, int block)
{
	DISKBLOCK_BUFFER *current = NULL;

	int _goto = block % MAX_DISKBLOCK_BUFFER;

	if (diskblock_mod_buffer[_goto].diskblock_next_buffer == NULL)
	{
		return NULL;
	}
	else
	{
		for (current = diskblock_mod_buffer[_goto].diskblock_next_buffer; current != &diskblock_mod_buffer[_goto]; current = current->diskblock_next_buffer)
		{
			if (current->block == block && current->device.major == device->major && current->device.minor == device->minor)
				return current;
		}

	}
	return NULL;
}

void remove_buffer_from_free_list(DISKBLOCK_BUFFER *buffer)
{
	DISKBLOCK_BUFFER *next = buffer->diskblock_freelist_next_buffer;
	DISKBLOCK_BUFFER *prev = buffer->diskblock_freelist_previous_buffer;
	DISKBLOCK_BUFFER *last = NULL;

	if (diskblock_freelist->diskblock_freelist_next_buffer == diskblock_freelist)
	{
		printf("\nscen0");
		diskblock_freelist = NULL;
		return;
	}
	else
	{

		/* first node logic*/
		if (diskblock_freelist == buffer)
		{
			printf("\nscen1");
			last = diskblock_freelist->diskblock_freelist_previous_buffer;
			diskblock_freelist = next;
			next->diskblock_freelist_previous_buffer = last;
			last->diskblock_freelist_next_buffer = diskblock_freelist;
			return;
		}
		/**/

		/*last node logic*/
		if (buffer->diskblock_freelist_next_buffer == diskblock_freelist)
		{
			printf("\nscen2");
			prev->diskblock_freelist_next_buffer = diskblock_freelist;
			diskblock_freelist->diskblock_freelist_previous_buffer = prev;
			return;
		}
		/**/

		/*middle node logic*/
		printf("\nscen3");
		prev->diskblock_freelist_next_buffer = next;
		next->diskblock_freelist_previous_buffer = prev;
		/**/
	}

}

DISKBLOCK_BUFFER *get_first_buffer_diskblock_freelist()
{
	DISKBLOCK_BUFFER *first = diskblock_freelist;
	DISKBLOCK_BUFFER *next = NULL;
	DISKBLOCK_BUFFER *last = NULL;

	if (diskblock_freelist->diskblock_freelist_next_buffer == diskblock_freelist)
	{
		diskblock_freelist = NULL;
		return first;
	}

	next = diskblock_freelist->diskblock_freelist_next_buffer;
	last = diskblock_freelist->diskblock_freelist_previous_buffer;

	diskblock_freelist = next;
	next->diskblock_freelist_previous_buffer = last;
	last->diskblock_freelist_next_buffer = diskblock_freelist;

	return first;

}

void remove_buffer_from_old_hashQueue(DISKBLOCK_BUFFER *buffer)
{

	DISKBLOCK_BUFFER * prev = buffer->diskblock_previous_buffer;
	DISKBLOCK_BUFFER * next = buffer->diskblock_next_buffer;
	int _goto = buffer->block % MAX_DISKBLOCK_BUFFER;

	if (buffer->diskblock_next_buffer == &diskblock_mod_buffer[_goto] && buffer->diskblock_previous_buffer == &diskblock_mod_buffer[_goto])
	{
		// only one node
		printf("\n1remove_buffer_from_old_hashQueue");
		diskblock_mod_buffer[_goto].diskblock_next_buffer = NULL;
		diskblock_mod_buffer[_goto].diskblock_previous_buffer = NULL;
		return;
	}
	else
	{

		// last node

		if (buffer->diskblock_next_buffer == &diskblock_mod_buffer[_goto] && diskblock_mod_buffer[_goto].diskblock_previous_buffer == buffer)
		{
			printf("\n2remove_buffer_from_old_hashQueue");
			prev->diskblock_next_buffer = &diskblock_mod_buffer[_goto];
			diskblock_mod_buffer[_goto].diskblock_previous_buffer = prev;
			return;
		}
	}
	printf("\n3remove_buffer_from_old_hashQueue");
	prev->diskblock_next_buffer = next;
	next->diskblock_previous_buffer = prev;

}

void put_buffer_on_new_hashQueue(DISKBLOCK_BUFFER *buffer)
{
	put_buffer_to_respective_mod(buffer);
}

void put_the_buffer_at_begining_of_the_freelist(DISKBLOCK_BUFFER *buffer)
{

	if (diskblock_freelist == NULL)
	{
		diskblock_freelist = buffer;
		buffer->diskblock_freelist_next_buffer = diskblock_freelist;
		buffer->diskblock_freelist_previous_buffer = diskblock_freelist;
		return;
	}


	DISKBLOCK_BUFFER *last = diskblock_freelist->diskblock_freelist_previous_buffer;

	buffer->diskblock_freelist_next_buffer = diskblock_freelist;
	diskblock_freelist->diskblock_freelist_previous_buffer = buffer;

	diskblock_freelist = buffer;
	buffer->diskblock_freelist_previous_buffer = diskblock_freelist;
	last->diskblock_freelist_next_buffer = diskblock_freelist;
}

void put_the_buffer_at_the_end_of_freelist(DISKBLOCK_BUFFER *buffer)
{

	if (diskblock_freelist == NULL)
	{
		diskblock_freelist = buffer;
		buffer->diskblock_freelist_next_buffer = diskblock_freelist;
		buffer->diskblock_freelist_previous_buffer = diskblock_freelist;
		return;
	}

	DISKBLOCK_BUFFER *last = diskblock_freelist->diskblock_freelist_previous_buffer;

	last->diskblock_freelist_next_buffer = buffer;
	buffer->diskblock_freelist_previous_buffer = last;

	buffer->diskblock_freelist_next_buffer = diskblock_freelist;
	diskblock_freelist->diskblock_freelist_previous_buffer = buffer;

}

DISKBLOCK_BUFFER *getblk(DEVICE device, int block)
{

	while (1)
	{
		DISKBLOCK_BUFFER *buffer = serach_block_in_hash_queue(&device, block);

		if (buffer != NULL)
		{
			printf("buffer is present");
			if (buffer->status == BUFFER_LOCKED)
			{
				//sleep till buffer becomes free
				continue;
			}

			buffer->status = BUFFER_LOCKED;
			remove_buffer_from_free_list(buffer);
			return buffer;

		}
		else
		{
			printf("buffer is  not present");
			if (diskblock_freelist == NULL)
			{
				//sleep for any buffer becomes free
				printf("\n\nfreelist is empty");
				continue;
			}

			buffer = get_first_buffer_diskblock_freelist();

			if (buffer->status == BUFFER_DELAY_WRITE)
			{
				buffer->status = BUFFER_WRITE;
				async_controller_write_initialization(buffer->device, buffer->block, buffer->data);
				buffer->status = BUFFER_OLD; // check
				continue;
			}


			buffer->status = BUFFER_LOCKED;
			buffer->block = block;
			buffer->device.major = device.major;
			buffer->device.minor = device.minor;

			remove_buffer_from_old_hashQueue(buffer);
			put_buffer_on_new_hashQueue(buffer);


			return buffer;
		}
	}
}

void brelse(DISKBLOCK_BUFFER *buffer) //input locked buffer
{
	// wakeup all processess which are waiting for any buffer
	//wakeupa all process which are waiting for this buffer
	// raise processor execution level to block interrupts

	if (buffer->status == BUFFER_VALID && buffer->status != BUFFER_OLD) //valid & new
	{
		put_the_buffer_at_the_end_of_freelist(buffer);
	}
	else
	{
		put_the_buffer_at_begining_of_the_freelist(buffer);
	}

	//lower the process execution level to allow interrupts
	buffer->status = BUFFER_UNLOCKED;

}

DISKBLOCK_BUFFER * bread(DEVICE device, int block)
{

	DISKBLOCK_BUFFER *buffer = getblk(device, block);

	if (buffer->status == BUFFER_VALID)
	{
		return buffer;
	}

	buffer->status = BUFFER_READ;
	controller_read(buffer->device, buffer->block, buffer->data);
	//sleep event: till hardware controlling reading  /*where to sleep ?*/

	return buffer;
}

void bwrite(DISKBLOCK_BUFFER *buffer)
{

	/*if(buffer->io == SYNC)
	{
	controller_write(buffer);
	//sleep : event until i/o completes
	brelease(buffer);
	}
	else if(buffer->status == DELAY_WRITE)
	{
	put_the_buffer_at_begining_of_the_freelist(buffer);
	}*/
}



/*---- BUFFER CACHE DRIVER -----------------------------------------
*   end of implementation of buffer cache Driver.
*   NOTE : it just a simulation of buffer cache Driver using simple
*	'C' program.
*-------------------------------------------------------------------*/






/*---- INTERNAL REPRESENTATION OF FILES-----------------------------------------
*   NOTE : it just a simulation of internal representation of file Driver using simple
*	'C' program.
*-------------------------------------------------------------------*/



#define MAX_BOOT_BLOCK_SIZE 1024

#define MAX_FREE_INODES_AVAILABLE  30//30
#define MAX_INODES  100//100 //(BLOCK_SIZE/sizeof(DISK_INODE));

#define MAX_FREE_BLOCKS_AVAILABLE 30//30
#define MAX_DATABLOCKS 100//1024

//102400//100 mb

#define MAX_LENGTH 14
#define THIRTEEN_MEMBER_ARRAY 13

#define MAX_INCORE_INODE_BUFFER 30
#define MAX_INCORE_INODE_HEADER 4

#define NO_INODE NULL;

typedef struct _directory_info
{
	unsigned short int inode_number;
	char filename[MAX_LENGTH];

}DIRECTORY_INFO;

typedef struct _owner_
{
	unsigned int id;

} OWNER;

typedef enum _file_type_
{
	FILE_ZERO,
	FILE_REGULAR,
	FILE_DIRECTORY,
	FILE_CHARACTER,
	FILE_BLOCK,
	FILE_PIPE

} FILE_TYPE;

typedef struct _TIME_
{
	unsigned short int time_format_am_pm : 1;

	unsigned short int hours : 4;
	unsigned short int minutes : 6;
	unsigned short int seconds : 6;

	unsigned short int day : 5;
	unsigned short int month : 4;
	unsigned short int year;

} TIME;

typedef struct _file_access_permission_
{
	unsigned short int owner_read : 1;
	unsigned short int owner_write : 1;
	unsigned short int owner_execute : 1;

	unsigned short int group_read : 1;
	unsigned short int group_write : 1;
	unsigned short int group_execute : 1;

	unsigned short int other_read : 1;
	unsigned short int other_write : 1;
	unsigned short int other_execute : 1;

}FILE_ACCESS_PERMISSION;

typedef struct _disk_inode_
{
	OWNER owner;
	OWNER group;

	FILE_ACCESS_PERMISSION file_access_permission;

	FILE_TYPE file_type;

	TIME file_access_time;
	TIME file_modified_time;
	TIME inode_modified_time;

	unsigned short int link_count;
	unsigned short int disk_addresses[THIRTEEN_MEMBER_ARRAY];
	unsigned short int file_size;

} DISK_INODE;

typedef struct _boot_block_
{
	char data[MAX_BOOT_BLOCK_SIZE];

} BOOT_BLOCK;
BOOT_BLOCK boot_block;

typedef struct _super_block_
{

	int sizeof_filesystem;
	int number_of_free_blocks_in_filesystem;
	int list_of_free_blocks_in_filesystem[MAX_FREE_BLOCKS_AVAILABLE];
	int index_of_next_free_block;


	int sizeof_inod_list;
	int number_of_free_inodes_in_file_system;
	int list_of_free_inodes_in_file_system[MAX_FREE_INODES_AVAILABLE];
	int index_of_next_free_inode;

	int is_super_block_locked;
	int is_super_block_modified;

} SUPER_BLOCK;
SUPER_BLOCK super_block;

typedef struct _inode_list_
{
	DISK_INODE inode_list[MAX_INODES];
} INODE_LIST;
INODE_LIST inode_list;

typedef struct _data_block_
{
	char data[BLOCK_SIZE];
} DATA_BLOCK;

DATA_BLOCK data_block;

typedef enum _incore_inode_status_
{
	INODE_LOCKED,
	INODE_UNLOCKED,
	INODE_RESERVED,
	INODE_UNRESERVED,
	INODE_CHANGED,
	INODE_UNCHANGED,
	INODE_FILE_FIELD_CHANGED,
	INODE_FILE_FIELD_NOT_CHANGED,
	INODE_MOUNT_POINT,
	INODE_NOT_MOUNT_POINT,

} INCORE_INODE_STATUS;

typedef struct _incore_inode_
{
	DISK_INODE inode;
	INCORE_INODE_STATUS incore_inode_status;
	DEVICE device;

	unsigned int inode_number;
	unsigned int reference_count;
	unsigned int is_inode_modified;

	struct _incore_inode_ *inode_next_buffer;
	struct _incore_inode_ *inode_previous_buffer;
	struct _incore_inode_ *inode_freelist_next_buffer;
	struct _incore_inode_ *inode_freelist_previous_buffer;

} INCORE_INODE;

INCORE_INODE incore_inode[MAX_INCORE_INODE_BUFFER];
INCORE_INODE incore_inode_mod_buffer[MAX_INCORE_INODE_HEADER];
INCORE_INODE *incore_inode_freelist = NULL;

void put_buffer_to_respective_incore_inode_mod(INCORE_INODE *buffer)
{
	int _goto = buffer->inode_number % MAX_INCORE_INODE_HEADER;

	INCORE_INODE *last = incore_inode_mod_buffer[_goto].inode_previous_buffer;

	if (incore_inode_mod_buffer[_goto].inode_next_buffer == NULL)
	{

		incore_inode_mod_buffer[_goto].inode_next_buffer = buffer;
		buffer->inode_previous_buffer = &incore_inode_mod_buffer[_goto];


		buffer->inode_next_buffer = &incore_inode_mod_buffer[_goto];
		incore_inode_mod_buffer[_goto].inode_previous_buffer = buffer;
	}

	else
	{
		last->inode_next_buffer = buffer;
		buffer->inode_previous_buffer = last;

		buffer->inode_next_buffer = &incore_inode_mod_buffer[_goto];
		incore_inode_mod_buffer[_goto].inode_previous_buffer = buffer;

	}

}

void put_buffer_to_incore_inode_freelist(INCORE_INODE *buffer)
{


	if (incore_inode_freelist == NULL)
	{
		incore_inode_freelist = buffer;
		buffer->inode_freelist_next_buffer = incore_inode_freelist;
		buffer->inode_freelist_previous_buffer = incore_inode_freelist;
	}
	else
	{
		INCORE_INODE *last = incore_inode_freelist->inode_freelist_previous_buffer;

		last->inode_freelist_next_buffer = buffer;
		buffer->inode_freelist_previous_buffer = last;

		buffer->inode_freelist_next_buffer = incore_inode_freelist;
		incore_inode_freelist->inode_freelist_previous_buffer = buffer;
	}

}

void init_incore_inode_table()
{
	int i = 0;

	for (i = 0; i < MAX_INCORE_INODE_HEADER; i++)
	{
		memset(incore_inode_mod_buffer, 0, sizeof(INCORE_INODE));

		incore_inode_mod_buffer[i].incore_inode_status = INODE_RESERVED;
		incore_inode_mod_buffer[i].inode_number = i;
		incore_inode_mod_buffer[i].reference_count = 0;


		incore_inode_mod_buffer[i].device.major = 0;
		incore_inode_mod_buffer[i].device.minor = 0;

		incore_inode_mod_buffer[i].inode_freelist_next_buffer = NULL;
		incore_inode_mod_buffer[i].inode_freelist_previous_buffer = NULL;

		incore_inode_mod_buffer[i].inode_next_buffer = NULL;
		incore_inode_mod_buffer[i].inode_previous_buffer = NULL;
	}

	for (i = 0; i < MAX_INCORE_INODE_BUFFER; i++)
	{
		memset(incore_inode, 0, sizeof(INCORE_INODE));

		incore_inode[i].incore_inode_status = INODE_UNLOCKED;
		incore_inode[i].inode_number = i;
		incore_inode[i].reference_count = 0;


		incore_inode[i].device.major = 0;
		incore_inode[i].device.minor = 0;

		incore_inode[i].inode_freelist_next_buffer = NULL;
		incore_inode[i].inode_freelist_previous_buffer = NULL;

		incore_inode[i].inode_next_buffer = NULL;
		incore_inode[i].inode_previous_buffer = NULL;

	}

	for (i = 0; i < MAX_INCORE_INODE_BUFFER; i++)
	{
		put_buffer_to_respective_incore_inode_mod(&incore_inode[i]);

	}

	for (i = 0; i < MAX_INCORE_INODE_BUFFER; i++)
	{
		put_buffer_to_incore_inode_freelist(&incore_inode[i]);

	}

}

void display_incore_inode_hashpool(void)
{

	int i = 0;
	INCORE_INODE *current = NULL;

	printf("\n\nINCORE INODE CACHE POOL\n");
	for (i = 0; i < MAX_INCORE_INODE_HEADER; i++)
	{
		current = incore_inode_mod_buffer[i].inode_next_buffer;

		printf("\n[MOD %d]<-->", i);
		do
		{
			if (incore_inode_mod_buffer[i].inode_next_buffer == NULL) break;

			printf("[%d]<-->", current->inode_number);
			current = current->inode_next_buffer;

		} while (current != &incore_inode_mod_buffer[i]);
		printf("[MOD %d]\n", i);

	}
}

void display_incore_inode_freelist(void)
{
	INCORE_INODE *current = incore_inode_freelist;
	printf("\n\n\nINCORE INODE FREE LIST\n\n\n");

	printf("INCORE INODE FREE LIST-->");
	do
	{
		if (current == NULL) break;

		printf("[%d]<-->", current->inode_number);
		current = current->inode_freelist_next_buffer;
	} while (current != incore_inode_freelist);
	printf("INCORE INODE FREELIST\n");

}

INCORE_INODE * serach_block_in_incore_inode_hash_queue(DEVICE *device, int inode_number)
{
	INCORE_INODE *current = NULL;

	int _goto = inode_number % MAX_INCORE_INODE_HEADER;

	if (incore_inode_mod_buffer[_goto].inode_next_buffer == NULL)
	{
		return NULL;
	}
	else
	{
		for (current = incore_inode_mod_buffer[_goto].inode_next_buffer; current != &incore_inode_mod_buffer[_goto]; current = current->inode_next_buffer)
		{
			if (current->inode_number == inode_number && current->device.major == device->major && current->device.minor == device->minor)
				return current;
		}

	}
	return NULL;
}

void remove_incore_inode_from_free_list(INCORE_INODE *buffer)
{
	INCORE_INODE *next = buffer->inode_freelist_next_buffer;
	INCORE_INODE *prev = buffer->inode_freelist_previous_buffer;
	INCORE_INODE *last = NULL;

	if (incore_inode_freelist->inode_freelist_next_buffer == incore_inode_freelist)
	{
		printf("\nscen0");
		incore_inode_freelist = NULL;
		return;
	}
	else
	{

		/* first node logic*/
		if (incore_inode_freelist == buffer)
		{
			printf("\nscen1");
			last = incore_inode_freelist->inode_freelist_previous_buffer;
			incore_inode_freelist = next;
			next->inode_freelist_previous_buffer = last;
			last->inode_freelist_next_buffer = incore_inode_freelist;
			return;
		}
		/**/

		/*last node logic*/
		if (buffer->inode_freelist_next_buffer == incore_inode_freelist)
		{
			printf("\nscen2");
			prev->inode_freelist_next_buffer = incore_inode_freelist;
			incore_inode_freelist->inode_freelist_previous_buffer = prev;
			return;
		}
		/**/

		/*middle node logic*/
		printf("\nscen3");
		prev->inode_freelist_next_buffer = next;
		next->inode_freelist_previous_buffer = prev;
		/**/
	}

}

INCORE_INODE *get_first_incore_inode_from_freelist()
{
	INCORE_INODE *first = incore_inode_freelist;
	INCORE_INODE *next = NULL;
	INCORE_INODE *last = NULL;

	if (incore_inode_freelist->inode_freelist_next_buffer == incore_inode_freelist)
	{
		incore_inode_freelist = NULL;
		return first;
	}

	next = incore_inode_freelist->inode_freelist_next_buffer;
	last = incore_inode_freelist->inode_freelist_previous_buffer;

	incore_inode_freelist = next;
	next->inode_freelist_previous_buffer = last;
	last->inode_freelist_next_buffer = incore_inode_freelist;

	return first;

}

void remove_incore_inode_from_old_hashQueue(INCORE_INODE *buffer)
{

	INCORE_INODE * prev = buffer->inode_previous_buffer;
	INCORE_INODE * next = buffer->inode_next_buffer;

	int _goto = buffer->inode_number % MAX_INCORE_INODE_HEADER;

	if (buffer->inode_next_buffer == &incore_inode_mod_buffer[_goto] && buffer->inode_previous_buffer == &incore_inode_mod_buffer[_goto])
	{
		// only one node
		printf("\n1remove_buffer_from_old_hashQueue");
		incore_inode_mod_buffer[_goto].inode_next_buffer = NULL;
		incore_inode_mod_buffer[_goto].inode_previous_buffer = NULL;
		return;
	}
	else
	{

		// last node

		if (buffer->inode_next_buffer == &incore_inode_mod_buffer[_goto] && incore_inode_mod_buffer[_goto].inode_previous_buffer == buffer)
		{
			printf("\n2remove_buffer_from_old_hashQueue");
			prev->inode_next_buffer = &incore_inode_mod_buffer[_goto];
			incore_inode_mod_buffer[_goto].inode_previous_buffer = prev;
			return;
		}
	}

	printf("\n3remove_buffer_from_old_hashQueue");
	prev->inode_next_buffer = next;
	next->inode_previous_buffer = prev;

}

void put_incore_inode_on_new_hashQueue(INCORE_INODE *buffer)
{
	put_buffer_to_respective_incore_inode_mod(buffer);
}

void put_the_incore_inode_at_begining_of_the_freelist(INCORE_INODE *buffer)
{

	if (incore_inode_freelist == NULL)
	{
		incore_inode_freelist = buffer;
		buffer->inode_freelist_next_buffer = incore_inode_freelist;
		buffer->inode_freelist_previous_buffer = incore_inode_freelist;
		return;
	}


	INCORE_INODE *last = incore_inode_freelist->inode_freelist_previous_buffer;

	buffer->inode_freelist_next_buffer = incore_inode_freelist;
	incore_inode_freelist->inode_freelist_previous_buffer = buffer;

	incore_inode_freelist = buffer;
	buffer->inode_freelist_previous_buffer = incore_inode_freelist;
	last->inode_freelist_next_buffer = incore_inode_freelist;
}

void put_the_incore_inode_at_the_end_of_freelist(INCORE_INODE *buffer)
{

	if (incore_inode_freelist == NULL)
	{
		incore_inode_freelist = buffer;
		buffer->inode_freelist_next_buffer = incore_inode_freelist;
		buffer->inode_freelist_previous_buffer = incore_inode_freelist;
		return;
	}

	INCORE_INODE *last = incore_inode_freelist->inode_freelist_previous_buffer;

	last->inode_freelist_next_buffer = buffer;
	buffer->inode_freelist_previous_buffer = last;

	buffer->inode_freelist_next_buffer = incore_inode_freelist;
	incore_inode_freelist->inode_freelist_previous_buffer = buffer;

}

INCORE_INODE *iget(DEVICE device, int inode_number)
{

	while (1)
	{
		INCORE_INODE *inode = serach_block_in_incore_inode_hash_queue(&device, inode_number);

		if (inode != NULL)
		{
			printf("inode is present");
			if (inode->incore_inode_status == INODE_LOCKED)
			{
				printf("inode is present and LOCKED");
				//sleep (event Mode becomes unlocked);
				continue;
			}


			if (inode->reference_count == 0) /*Inode on Freelist*/
			{
				remove_incore_inode_from_free_list(inode);
			}

			inode->incore_inode_status = INODE_LOCKED;
			inode->reference_count++;

			printf("inode is present and UNLOCKED");

			return inode;
		}

		printf("buffer is  not present");
		if (incore_inode_freelist == NULL)
		{
			//error
			return NO_INODE;
		}

		inode = get_first_incore_inode_from_freelist();

		printf("initialize inode");
		inode->incore_inode_status = INODE_LOCKED;
		inode->inode_number = inode_number;
		inode->device.major = device.major;
		inode->device.minor = device.minor;

		remove_incore_inode_from_old_hashQueue(inode);
		put_incore_inode_on_new_hashQueue(inode);

		//read inode bread

		char data[BLOCK_SIZE];
		int block_number = ((inode->inode_number - 1) / (BLOCK_SIZE / sizeof(DISK_INODE))) + (INODE_BLOCK_OFFSET*BLOCK_SIZE);
		DISKBLOCK_BUFFER* buffer = bread(device, block_number);
		memcpy((void*)&data, (void*)&buffer->data, BLOCK_SIZE);
		brelse(buffer);

		int inode_offset = ((inode->inode_number - 1) % (BLOCK_SIZE / sizeof(DISK_INODE))) * sizeof(DISK_INODE);
		DISK_INODE *in = (DISK_INODE*)(data + inode_offset);
		//

		//init inode

		inode->inode.file_type = in->file_type;

		inode->inode.file_access_time = in->file_access_time;
		inode->inode.file_modified_time = in->file_modified_time;
		inode->inode.inode_modified_time = in->inode_modified_time;

		inode->inode.link_count = in->link_count;
		memcpy(inode->inode.disk_addresses, in->disk_addresses, sizeof(in->disk_addresses));
		inode->inode.file_size = in->file_size;
		//

		inode->reference_count = 1;
		return inode;

	}
}

void _free(int block_number)
{
	super_block.number_of_free_blocks_in_filesystem++;

	if (super_block.is_super_block_locked == 1)
		return;

	if (super_block.number_of_free_blocks_in_filesystem == MAX_FREE_BLOCKS_AVAILABLE)
	{

		if (block_number < super_block.list_of_free_blocks_in_filesystem[0])
		{
			super_block.list_of_free_blocks_in_filesystem[0] = block_number;
		}
	}
	else
	{
		super_block.index_of_next_free_block++;
		super_block.list_of_free_blocks_in_filesystem[super_block.index_of_next_free_block] = block_number;
	}

	return;
}

void ifree(int inode_number)
{

	super_block.number_of_free_inodes_in_file_system++;

	if (super_block.is_super_block_locked == 1)
		return;

	if (super_block.number_of_free_inodes_in_file_system == MAX_FREE_INODES_AVAILABLE)
	{

		if (inode_number < super_block.list_of_free_inodes_in_file_system[0])
		{
			super_block.list_of_free_inodes_in_file_system[0] = inode_number;
		}
	}
	else
	{
		super_block.index_of_next_free_inode++;
		super_block.list_of_free_inodes_in_file_system[super_block.index_of_next_free_inode] = inode_number;
	}

	return;
}

void iput(INCORE_INODE *inode)
{
	if (inode->incore_inode_status != INODE_LOCKED)
	{
		inode->incore_inode_status = INODE_LOCKED; // inode is never loacked across two system call
	}

	inode->reference_count--; // prevent kernel to allocate differnt incore inode of same inode_node number

	if (inode->reference_count == 0)
	{

		if (inode->inode.link_count == 0)
		{
			int i = 0;
			for (i = 0; i < THIRTEEN_MEMBER_ARRAY; i++)
			{
				int blk_no = inode->inode.disk_addresses[i];

				if (blk_no == -1)
					continue;

				_free(inode->inode.disk_addresses[i]);
			}


			inode->inode.file_type = FILE_ZERO;

			ifree(inode->inode_number);
		}

		if (inode->is_inode_modified == 1)
		{

			int block_number = ((inode->inode_number - 1) / (BLOCK_SIZE / sizeof(DISK_INODE))) + (INODE_BLOCK_OFFSET*BLOCK_SIZE);
			DISKBLOCK_BUFFER* buffer = bread(inode->device, block_number);

			int inode_offset = ((inode->inode_number - 1) % (BLOCK_SIZE / sizeof(DISK_INODE))) * sizeof(DISK_INODE);
			memcpy((void*)(buffer->data + inode_offset), (void*)&inode->inode, sizeof(DISK_INODE));

			bwrite(buffer);
			brelse(buffer);

		}

		put_the_incore_inode_at_the_end_of_freelist(inode);//check if end or front;
	}

	inode->incore_inode_status = INODE_UNLOCKED;
}

void bmap(INCORE_INODE *inode, int offset, int *out_block_number, int *out_bytes_offset_in_block, int *out_read_head_block)
{
	char data[BLOCK_SIZE];

}

INCORE_INODE* namei(char *path)
{
	INCORE_INODE *working_inode = NULL;
	if (path[0] != '\\') //root
	{
		//working_inode =  iget(device,process.root_inode);
	}
	else
	{
		//working_inode = iget(device,process.current_directory_inode);
	}

	char *file = strtok(path, "\\");
	int offset, *out_block_number, *out_bytes_offset_in_block, *out_read_head_block;
	while (file != NULL)
	{
		/* if(working_inode->inode->file_type != DIRECTORY)
		return NO_INODE;
		//check for access permission;
		bmap( device,working_inode,int offset,out_block_number,out_bytes_offset_in_block,out_read_head_block);


		file = strtok(NULL,'\')*/
	}

	return NULL;
}

INCORE_INODE *ialloc(DEVICE device)
{
	while (1)
	{
		if (super_block.is_super_block_locked == 1)
		{
			//sleep till super block become unlock
			continue;
		}

		if (super_block.index_of_next_free_inode == 0)
		{

		}

		super_block.is_super_block_locked = 1;
		int free_inode = super_block.list_of_free_inodes_in_file_system[super_block.index_of_next_free_inode];
		super_block.index_of_next_free_inode--;
		super_block.is_super_block_locked = 0; //check when to save super block

		INCORE_INODE *inode = iget(device, free_inode);

		if (inode->reference_count != 0)//inode not free
		{

			int block_number = ((inode->inode_number - 1) / (BLOCK_SIZE / sizeof(DISK_INODE))) + (INODE_BLOCK_OFFSET*BLOCK_SIZE);
			DISKBLOCK_BUFFER* buffer = bread(device, block_number);

			int inode_offset = ((inode->inode_number - 1) % (BLOCK_SIZE / sizeof(DISK_INODE))) * sizeof(DISK_INODE);
			memcpy((void*)(buffer->data + inode_offset), (void*)&inode->inode, sizeof(DISK_INODE));

			bwrite(buffer);
			brelse(buffer);

			iput(inode);
			continue;
		}

		//*initialize inode code*

		//


		//write inode to disk
		int block_number = ((inode->inode_number - 1) / (BLOCK_SIZE / sizeof(DISK_INODE))) + (INODE_BLOCK_OFFSET*BLOCK_SIZE);
		DISKBLOCK_BUFFER* buffer = bread(device, block_number);

		int inode_offset = ((inode->inode_number - 1) % (BLOCK_SIZE / sizeof(DISK_INODE))) * sizeof(DISK_INODE);
		memcpy((void*)(buffer->data + inode_offset), (void*)&inode->inode, sizeof(DISK_INODE));

		bwrite(buffer);
		brelse(buffer);
		//

		super_block.number_of_free_inodes_in_file_system--;
		return inode;
	}

}

DISKBLOCK_BUFFER *alloc(DEVICE device)
{

	DISKBLOCK_BUFFER *buffer = NULL;

	while (super_block.is_super_block_locked == 1)
	{
		//sleep event super block locked;
	}

	int data_block = super_block.list_of_free_blocks_in_filesystem[super_block.index_of_next_free_block];

	if (super_block.index_of_next_free_block == 0)
	{
		super_block.is_super_block_locked = 1;

		buffer = bread(device, data_block);



		super_block.is_super_block_locked = 0;

		//wake up processes (event super block not locked )
	}


	buffer = getblk(device, data_block);
	memset((void*)buffer->data, 0, BLOCK_SIZE);
	super_block.index_of_next_free_block--;
	super_block.is_super_block_modified = 1;
	return buffer;
}

void mkfs(char *device_location)
{
	
	int i = 0;
	int j = 0;
	int next_position = 0;
	char buffer_data[BLOCK_SIZE];
	DEVICE device;

		
	FILE *file = fopen(device_location, "wb+");
	if (file == NULL)
	{
		printf("\nError: Unable to Create Disk %s",device_location);
		exit(0);
	}
	device.fp = file;

	/*Create Boot block*/
	char *message = "this is boot block";
	memcpy(boot_block.data, message, strlen(message));
	controller_write(device, BOOT_BLOCK_OFFSET, (char*)&boot_block);
	/**/
	


	/*Create Super Block*/
	super_block.sizeof_filesystem = sizeof(boot_block) + sizeof(super_block) + sizeof(inode_list) + (sizeof(data_block)*MAX_DATABLOCKS);
	super_block.number_of_free_blocks_in_filesystem = MAX_DATABLOCKS;
	super_block.list_of_free_blocks_in_filesystem[MAX_FREE_BLOCKS_AVAILABLE];
	memset(super_block.list_of_free_blocks_in_filesystem, -1, sizeof(super_block.list_of_free_blocks_in_filesystem));

	j = 1;
	for (i = MAX_FREE_BLOCKS_AVAILABLE - 1; i >= 0;i--)
	{
		super_block.list_of_free_blocks_in_filesystem[i] = j;
		j++;
	}
	super_block.index_of_next_free_block = MAX_FREE_BLOCKS_AVAILABLE;
	super_block.sizeof_inod_list = (int)sizeof(inode_list);
	super_block.number_of_free_inodes_in_file_system = MAX_INODES;
	super_block.list_of_free_inodes_in_file_system[MAX_FREE_INODES_AVAILABLE];
	memset(super_block.list_of_free_inodes_in_file_system, -1, sizeof(super_block.list_of_free_inodes_in_file_system));
	j = 1;
	for (i = MAX_FREE_INODES_AVAILABLE - 1; i >= 0;i--)
	{
		super_block.list_of_free_inodes_in_file_system[i] = j;
		j++;
	}
	super_block.index_of_next_free_inode = MAX_FREE_INODES_AVAILABLE;
	super_block.is_super_block_locked = 0;
	super_block.is_super_block_modified = 0;

	memset((void*)&buffer_data, -1, sizeof(DATA_BLOCK));
	memcpy((void*)&buffer_data, (void*)&super_block, sizeof(SUPER_BLOCK));
	controller_write(device, SUPER_BLOCK_OFFSET, (char*)&buffer_data);
	/**/



	/*Create Root Directory*/
	time_t current_time = time(NULL);
	struct tm local_time = *localtime(&current_time);
	DISK_INODE inode;

	inode.file_type = FILE_DIRECTORY;
	inode.file_access_time.hours = local_time.tm_hour;
	inode.file_access_time.minutes = local_time.tm_min;
	inode.file_access_time.seconds = local_time.tm_sec;
	inode.file_access_time.time_format_am_pm = 1;

	inode.file_modified_time.hours = local_time.tm_hour;
	inode.file_modified_time.minutes = local_time.tm_min;
	inode.file_modified_time.seconds = local_time.tm_sec;
	inode.file_modified_time.time_format_am_pm = 1;
	inode.inode_modified_time.hours = local_time.tm_hour;
	inode.inode_modified_time.minutes = local_time.tm_min;
	inode.inode_modified_time.seconds = local_time.tm_sec;
	inode.inode_modified_time.time_format_am_pm = 1;
	inode.link_count = 1;
	inode.disk_addresses[THIRTEEN_MEMBER_ARRAY];
	memset(inode.disk_addresses, -1, sizeof(inode.disk_addresses));
	inode.disk_addresses[0] = DATA_BLOCK_OFFSET;
	inode.file_size = BLOCK_SIZE;
	/**/



	/*Create N inodes per block*/
	printf("\nNOTE: %d INODES/BLOCK", (BLOCK_SIZE / sizeof(DISK_INODE)));
	for (i = 0; i < (BLOCK_SIZE / sizeof(DISK_INODE)); i++)
	{
		if (i != 0)
		{
			/*make other file type zero so, it refer as free*/
			inode.file_type = FILE_ZERO;
		}

		memcpy(buffer_data + next_position, &inode, sizeof(DISK_INODE));
		next_position = next_position + sizeof(DISK_INODE);
	}
	/**/


	/*write inodes to inode block*/
	for (i = 0; i < ((1 * MAX_INODES) / (BLOCK_SIZE / sizeof(DISK_INODE))); i++)
	{
		controller_write(device, i + INODE_BLOCK_OFFSET, (char*)&buffer_data);
	}
	/**/


	/*Data blocks initialization*/
	memset(&buffer_data, -1, sizeof(DATA_BLOCK));
	for (i = 0;i< MAX_DATABLOCKS; i++)
	{
		controller_write(device, i + DATA_BLOCK_OFFSET, (char*)&buffer_data);
	}
	/**/


	/*Write root directory to block 0*/
	memset(&buffer_data, -1, sizeof(DATA_BLOCK));
	DIRECTORY_INFO directory_info;

	directory_info.inode_number = 0;
	strcpy(directory_info.filename, ".");
	memcpy(&buffer_data, (void*)&directory_info, sizeof(DIRECTORY_INFO));

	directory_info.inode_number = -1;
	strcpy(directory_info.filename, "..");
	memcpy(buffer_data + sizeof(DIRECTORY_INFO), &directory_info, sizeof(DIRECTORY_INFO));

	controller_write(device, DATA_BLOCK_OFFSET, (char*)&buffer_data);



	/********************************************/
	/*Read Super Block*/
	SUPER_BLOCK *di = NULL;
	char temp[BLOCK_SIZE];
	controller_read(device, SUPER_BLOCK_OFFSET, temp);


	di = (SUPER_BLOCK*)temp;
	printf("\n number_of_free_blocks_in_filesystem block %d", di->number_of_free_blocks_in_filesystem);
	printf("\n number_of_free_inodes_in_file_system in_file_system block %d", di->number_of_free_inodes_in_file_system);
	for (i = 0; i<MAX_FREE_BLOCKS_AVAILABLE;i++)
	{
		printf("\n free block %d", di->list_of_free_blocks_in_filesystem[i]);

	}

	for (i = 0; i<MAX_FREE_INODES_AVAILABLE;i++)
	{
		printf("\n free inodes %d", di->list_of_free_inodes_in_file_system[i]);
	}
	/**/


	/***************************************************************************/
	/*Read 0 th INODE Block*/
	char temps[BLOCK_SIZE];
	controller_read(device, INODE_BLOCK_OFFSET, temps);


	INCORE_INODE *ii = (INCORE_INODE*)temps;
	printf("\n file_type %d", ii->inode.file_type);
	printf("\n reference_count %d", ii->reference_count);
	printf("\n file_size %d", ii->inode.file_size);
	printf("\n inode_number %d", ii->inode_number);
	/**/



	/***************************************************************************/
	/*Read 0 th Data Block*/
	DIRECTORY_INFO *dis = NULL;
	char db[BLOCK_SIZE];
	controller_read(device, 3, db);

	dis = (DIRECTORY_INFO*)db;

	printf("\n filename %s  and inode %d ", dis->filename, dis->inode_number);

	dis = (DIRECTORY_INFO*)(db + sizeof(DIRECTORY_INFO));

	printf("\n filename %s  and inode %d ", dis->filename, dis->inode_number);

	/**/

	fclose(file);

}


/*---- INTERNAL REPRESENTATION OF FILES-----------------------------------------
*	end of implementation of internal represenation of file Driver.
*   NOTE : it just a simulation of internal representation of file Driver using simple
*	'C' program.
*-------------------------------------------------------------------*/











/*************************************************** SYSTEM CALLS FOR THE FILE SYSTEM **********************************/

int Open(char *filePath, FILE_ACCESS_PERMISSION permission, int filePermission)
{
	INCORE_INODE *inode = namei(filePath);

	if (inode == NULL)//|| inode->inode->file_access_permission!=)
		return errno;

	return 1;
}
/*************************************************** SYSTEM CALLS FOR THE FILE SYSTEM **********************************/



int main()
{

	printf("INCORE INODE SIZE %d\n", sizeof(INCORE_INODE));
	printf("DISK INODE %d\n", sizeof(DISK_INODE));
	printf("hello world %d \n", BLOCK_SIZE / sizeof(DISK_INODE));

	printf("\nBOOT_BLOCK_OFFSET %d", BOOT_BLOCK_OFFSET);
	printf("\nSUPER_BLOCK_OFFSET %d", SUPER_BLOCK_OFFSET);
	printf("\nINODE_BLOCK_OFFSET %d", INODE_BLOCK_OFFSET);
	printf("\nDATA_BLOCK_OFFSET %d", DATA_BLOCK_OFFSET);

	
	mkfs(device_location);


	//int fd = open(device_location, O_CREAT | O_RDWR | O_BINARY, 0);
	  FILE* file = fopen(device_location, "wb+");

	  if (file == NULL)
	  {
		  printf("\n%d file fd ", file);
	  }




	/*DEBUG FOR BUFFER CACHE*/
	/*init_hashQueuepool();

	display_diskblock_hashpool();
	display_diskblock_freelist();



	DEVICE dev;
	dev.major=0;
	dev.minor=0;
	getblk(dev,11);

	display_diskblock_hashpool();
	display_diskblock_freelist();*/
	/*END OF DEBUG FOR BUFFER CACHE*/


	/*DEBUG FOR INCORE INODE TABLE*/

	/* init_incore_inode_table();
	display_incore_inode_hashpool();
	display_incore_inode_freelist();

	DEVICE dev;
	dev.major=0;
	dev.minor=0;
	iget(dev,12);

	display_incore_inode_hashpool();
	display_incore_inode_freelist();*/
	/*END OF DEBUG FOR INCORE INODE TABLE*/


	printf("\n\n\nThanks!!!\n\n");
	getchar();

	return 1;
}
