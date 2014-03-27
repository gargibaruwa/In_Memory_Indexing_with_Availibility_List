#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

typedef struct{
	int key;
	long off;
} index_s;

index_s prim[10000];

typedef struct{
	int size;
	long off;
} avail_s;

avail_s avail[10000];

void add_cmd(char *cmd,char *list_order,FILE *fp);
void del_cmd(char *cmd,char *list_order,FILE *fp);
void find_cmd(char *cmd,char *list_order,FILE *fp);
void end_cmd(char *cmd,char *list_order,FILE *fp);
long firstfit_bestfit_offset(int size,char *list_order);
long worst_fit_get_offset(int size);
long write_rec_to_eof(char *buf,FILE *fp);
int compare_best_fit(const void *a, const void *b);
int compare_worst_fit(const void *a, const void *b);

void main(int argc, char*argv[])
{
	char *list_order;
	char *file_name;
	list_order = argv[1];
	file_name = argv[2];

	FILE *fprim; 
	//check if primary index file exists
	if(( fprim = fopen( "index.bin", "rb" )) != NULL )
	{
		//load array from primary index file
		long rec_off;
		int key;
		int i = 0;
		while(fread( &key,sizeof(int),1,fprim)==1)
		{
			prim[i].key = key;
			fread( &rec_off,sizeof(long),1,fprim);	
			prim[i].off = rec_off;
			i++;		
		}
		fclose(fprim);	
	}

	FILE *f_avail; 
	//check if availibility list file exists
	if(( f_avail = fopen( "avail.bin", "rb" )) != NULL )
	{
		//load array from primary index file
		long hole_off;
		int size;
		int j = 0;
		while(fread( &size,sizeof(int),1,f_avail)==1)
		{
			avail[j].size = size;
			fread( &hole_off,sizeof(long),1,f_avail);	
			avail[j].off = hole_off;
			j++;		
		}
		fclose(f_avail);	
	}

	FILE *fp;
	//if file does not exists, create a new file for adding record
	if(( fp = fopen( file_name, "r+b" )) == NULL ) 
	{
		fp = fopen( file_name, "w+b" );
	}

	/*
	fp = fopen("student.db","w+b");
	char *buf;
	buf = "712412913|Ford|Rob|Phi";
	int rec_size = strlen(buf);
	fwrite( &rec_size,sizeof(int),1,fp);
	fwrite( buf,sizeof(char),rec_size,fp);
	fclose(fp);

	fp = fopen("student.db","r+b");
	long rec_off = 0;
	fseek(fp,rec_off,SEEK_SET);
	fread( &rec_size,sizeof(int),1,fp);
	buf = malloc(rec_size);
	fread( buf,sizeof(char),rec_size,fp);
	printf("%s\n",buf);
	fclose(fp);
	*/

	char cmd[100];
	while(1)
	{
		//printf("enter a command\n");
		fgets(cmd, sizeof(cmd), stdin);
		int i;
		for(i=0;i<strlen(cmd);i++)				
		{
			
			if(cmd[i]=='\n')
			{
				cmd[i]='\0';
				break;
			}
		}
		if(strstr(cmd,"add"))
		{
			add_cmd(cmd,list_order,fp);			
		}
		else if(strstr(cmd,"find"))
		{
			find_cmd(cmd,list_order,fp);
		}
		else if(strstr(cmd,"del"))
		{
			del_cmd(cmd,list_order,fp);
		}
		else if(strcmp(cmd,"end") == 0)
		{	
			end_cmd(cmd,list_order,fp);			
			break;
		}
		else
		{
			//printf("not a command\n");
			break;
		}	
	}	
}

void add_cmd(char *cmd,char *list_order,FILE *fp)
{

	char *cmd_copy = cmd;
	char *command;
	char *key;
	char *rec;
	command = strtok(cmd_copy," ");
	//separate the key and record
	key = strtok(0," ");
	rec = strtok(0," ");
	
	rec[strlen(rec)] = '\0';
	key[strlen(key)] = '\0';
	
	//find the index of the last entry in primary key array	
	int i;
	for(i=0;i<10000;i++)
	{
		if(prim[i].key==0)
		{
			break;
		}
	}
	int end = i;

	//check if the primary key already exists
	int low,high,mid,flag;
	low = 0;
	high = end-1;
	mid = 0;
	flag = 0;
		
	while(low <= high)
	{			
		mid = (low+high)/2;
		if( atoi(key) < prim[mid].key )
		{
			high = mid - 1;
		}
		else if( atoi(key) > prim[mid].key )
		{
			low = mid + 1;
		}
		else
		{
			flag = 1;
			break;
		}			
	}
	if(flag==1)	
	{
		printf("Record with SID=%s exists\n",key);		
	}
	else
	{
		long rec_off=0;

		if(strcmp(list_order,"--first-fit") == 0 || strcmp(list_order,"--best-fit") == 0 )		
		{
			//search for hole, get offset, if size > incoming record+offset, create new hole and append to end of avail list
			long hole_offset = firstfit_bestfit_offset(strlen(rec),list_order);
			if(hole_offset!=-1)//hole exists
			{
				//write record in hole
				char *buf = rec;				
				int rec_size = strlen(buf);
				buf[rec_size] = '\0';				
				rec_off = hole_offset;
				
				fseek(fp, rec_off, SEEK_SET);
				fwrite( &rec_size,sizeof(int),1,fp);
				fwrite( buf,sizeof(char),rec_size,fp);				
			}
			else
			{
				rec_off = write_rec_to_eof(rec,fp);
			}			
		}

		else if(strcmp(list_order,"--worst-fit") == 0 )		
		{
			//search for hole, get offset, if size > incoming record+offset, create new hole and append to end of avail list
			long hole_offset = worst_fit_get_offset(strlen(rec));
			if(hole_offset!=-1)//hole exists
			{
				//write record in hole
				char *buf = rec;				
				int rec_size = strlen(buf);
				buf[rec_size] = '\0';				
				rec_off = hole_offset;
				
				fseek(fp, rec_off, SEEK_SET);
				fwrite( &rec_size,sizeof(int),1,fp);
				fwrite( buf,sizeof(char),rec_size,fp);				
			}
			else
			{
				rec_off = write_rec_to_eof(rec,fp);
			}			
		}
		
		//add entry in primary index at the last position
		prim[end].key = atoi(key);
		prim[end].off = rec_off;

		//sort the primary index array
		for(i = 1; i<=end; i++)
		{
			int j = i-1;
			int cur_key = prim[i].key;
			long cur_off = prim[i].off;
		
			while(j>=0 && prim[j].key>cur_key)
			{
				prim[j+1].key = prim[j].key;
				prim[j+1].off = prim[j].off;
				j--;
			}
			prim[j+1].key = cur_key;
			prim[j+1].off = cur_off;			
		}
	}
}

void find_cmd(char *cmd,char *list_order,FILE *fp)
{

	char *cmd_copy = cmd;
	char *command;
	char *key;
	//separate the key from command
	command = strtok(cmd_copy," ");
	key = strtok(0," ");
	key[strlen(key)] = '\0';

	//find the index of the last entry in primary key array	
	int i;
	for(i=0;i<10000;i++)
	{
		if(prim[i].key==0)
		{
			break;
		}
	}
	int end = i;

	//search for the primary key	
	int low,high,mid,flag;
	low = 0;
	high = end-1;
	mid = 0;
	flag = 0;
		
	while(low <= high)
	{			
		mid = (low+high)/2;
		if( atoi(key) < prim[mid].key )
		{
			high = mid - 1;
		}
		else if( atoi(key) > prim[mid].key )
		{
			low = mid + 1;
		}
		else
		{
			flag = 1;
			break;
		}			
	}
	if(flag==1)	
	{	//key exists, read the record
		long rec_off = prim[mid].off;
		int rec_size;
		char *buf;

		fseek(fp,rec_off,SEEK_SET);
		fread( &rec_size,sizeof(int),1,fp);
		buf = malloc(rec_size);
		fread( buf,sizeof(char),rec_size,fp);
		buf[rec_size]='\0';
		printf("%s\n",buf);	
		free(buf);	
	}
	else
	{
		printf("No record with SID=%s exists\n",key);
	}
}

void del_cmd(char *cmd,char *list_order,FILE *fp)
{

	char *cmd_copy = cmd;
	//separate the key from command
	char *command;
	char *key;
	command = strtok(cmd_copy," ");
	key = strtok(0," ");
	
	key[strlen(key)] = '\0';
	
	//find the index of the last entry in primary key array	
	int i;
	for(i=0;i<10000;i++)
	{
		if(prim[i].key==0)
		{
			break;
		}
	}
	int end = i;
	
	//search for the primary key	
	int low,high,mid,flag;
	low = 0;
	high = end-1;
	mid = 0;
	flag = 0;
		
	while(low <= high)
	{			
		mid = (low+high)/2;
		if( atoi(key) < prim[mid].key )
		{
			high = mid - 1;
		}
		else if( atoi(key) > prim[mid].key )
		{
			low = mid + 1;
		}
		else
		{
			flag = 1;
			break;
		}			
	}
	if(flag==1)
	{
		//update availibility list
		long rec_off = prim[mid].off;
		int rec_size,hole_size;

		fseek(fp,rec_off,SEEK_SET);
		fread( &rec_size,sizeof(int),1,fp);
		hole_size = rec_size*sizeof(char) + sizeof(int);
		
		//add hole to end of availibility list
		for(i=0;i<10000;i++)
		{
			if(avail[i].size==0)
			{
				break;
			}
		}
		avail[i].size = hole_size;
		avail[i].off = rec_off;
	
		if(strcmp(list_order,"--best-fit") == 0 )
		{
			//sort availibilty list in ascending order
			qsort(avail, i+1, sizeof(avail_s), compare_best_fit);
		}
		else if(strcmp(list_order,"--worst-fit") == 0 )
		{
			//sort availibilty list in descending order
			qsort(avail, i+1, sizeof(avail_s), compare_worst_fit);
		}

		//delete entry from primary key array
		for(i=mid;i<end-1;i++)
		{
			prim[i].key = prim[i+1].key;
			prim[i].off = prim[i+1].off;
		}
		prim[i].key = 0;
		prim[i].off = 0;		
	}
	else
	{
		printf("No record with SID=%s exists\n",key);
	}
}

void end_cmd(char *cmd,char *list_order,FILE *fp)
{

	int i;
	printf("Index:\n");
	for(i=0;i<10000;i++)
	{
		if(prim[i].key==0)
		{
			break;
		}
		printf("key=%d: offset=%ld\n",prim[i].key,prim[i].off);	
	}
	int prim_end = i;

	if(prim_end>0)
	{
		FILE *p_out; 
		p_out = fopen( "index.bin", "wb" ); 
		for(i=0;i<prim_end;i++)
		{
			fwrite( &prim[i].key, sizeof(int), 1, p_out ); 
			fwrite( &prim[i].off, sizeof(long), 1, p_out ); 
		}
		fclose( p_out );
	}

	printf("Availability:\n");
	int hole_siz = 0;
	for(i=0;i<10000;i++)
	{
		if(avail[i].size==0)
		{
			break;
		}
		printf("size=%d: offset=%ld\n",avail[i].size,avail[i].off);	
		hole_siz = hole_siz + avail[i].size;		
	}
	int hole_n = i;	

	printf( "Number of holes: %d\n", hole_n );
	printf( "Hole space: %d\n", hole_siz );

	if(hole_n>0)
	{
		FILE *a_out; 
		a_out = fopen( "avail.bin", "wb" ); 
		for(i=0;i<hole_n;i++)		
		{
			fwrite(&avail[i].size,sizeof(int),1,a_out); 
			fwrite(&avail[i].off,sizeof(long),1,a_out); 
		}		
		fclose(a_out);
	}

	//printf("end of program\n");
	fclose(fp);
}

long firstfit_bestfit_offset(int size,char *list_order)
{
	int i;
	int flag=0;
	int hole_index = -1;
	int new_rec_off = -1;
	int new_rec_size = -1;

	//size of record + record
	size = size + sizeof(int);
	for(i=0;i<10000;i++)
	{
		if(avail[i].size==0)
		{
			break;
		}
		if(avail[i].size>=size)
		{
			new_rec_off = avail[i].off;
			new_rec_size = avail[i].size;
			hole_index = i;
			break;
		}		
	}
	//if hole present
	if(hole_index!=-1)
	{
		for(i=0;i<10000;i++)
		{
			if(avail[i].size==0)
			{
				break;
			}
		}
		int last_hole_index = i-1;

		//delete entry from availibility list array
		for(i=hole_index;i<last_hole_index;i++)
		{
			avail[i].size = avail[i+1].size;
			avail[i].off = avail[i+1].off;
		}
		avail[last_hole_index].size = 0;
		avail[last_hole_index].off = 0;

		//if hole larger than record, divide it and append new hole to end of availibility list
		if(new_rec_size > size)
		{
			avail[last_hole_index].size = new_rec_size - size;
			avail[last_hole_index].off = new_rec_off + size;					
		}

		if(strcmp(list_order,"--best-fit") == 0)
		{
			qsort(avail, last_hole_index+1, sizeof(avail_s), compare_best_fit);
		}
		return new_rec_off;
	}
	return -1;
}

long worst_fit_get_offset(int size)
{
	size = size + sizeof(int);
	int i;
	if(avail[0].size>=size)
	{
		int new_rec_size = avail[0].size;
		long new_rec_off = avail[0].off;
		//delete entry from availibility list array
		for(i=0;i<9999;i++)
		{
			if(avail[i].size==0)
			{
				break;
			}
			avail[i].size = avail[i+1].size;
			avail[i].off = avail[i+1].off;
		}
		int last_hole_index = i;
		avail[last_hole_index].size = 0;
		avail[last_hole_index].off = 0;

		//if hole larger than record, divide it and append new hole to availibility list. then sort the list
		if(new_rec_size > size)
		{
			avail[last_hole_index].size = new_rec_size - size;
			avail[last_hole_index].off = new_rec_off + size;					
		}
		qsort(avail, last_hole_index+1, sizeof(avail_s), compare_worst_fit);		
		return new_rec_off;
	}
	return -1;
}		
	
long write_rec_to_eof(char *buf,FILE *fp)
{
	//write the record to end of file
	int rec_size;
	long rec_off=0;
	
	fseek( fp, 0, SEEK_SET);
	while(fread( &rec_size,sizeof(int),1,fp)==1)
	{
		rec_off = rec_off + sizeof(int) + (rec_size)*sizeof(char);
		fseek(fp,(rec_size)*sizeof(char),SEEK_CUR);	
	}
		
	//insert the new record
	rec_size = strlen(buf);
	buf[strlen(buf)] = '\0';
	fwrite( &rec_size,sizeof(int),1,fp);
	fwrite( buf,sizeof(char),rec_size,fp);

	return rec_off;	
}

int compare_best_fit(const void *a, const void *b)
{	
	int size1 = (*(avail_s*)a).size;
	int size2 = (*(avail_s*)b).size;

	if(size1<size2)	
	{
		return -1;
	}
	else if(size1>size2)
	{
		return 1;
	}
	return 0;	
}

int compare_worst_fit(const void *a, const void *b)
{	
	int size1 = (*(avail_s*)a).size;
	int size2 = (*(avail_s*)b).size;

	if(size1<size2)	
	{
		return 1;
	}
	else if(size1>size2)
	{
		return -1;
	}
	return 0;	
}
