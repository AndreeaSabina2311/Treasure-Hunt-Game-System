#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct{
  int treasure_id;
  char name[30];
  float latitude;
  float longitude;
  char clue[100];
  int value;
}Treasure;

struct stat st={0};

int main(int argc, char **argv)
{
  if(opendir("home/sabina/proiect")==NULL)
    {
      perror("eroare deschidere director proiect\n");
      exit(-1);
    }
  //adauga o noua comoara la directorul hunt specificat ca al doilea argument in linie de comanda
  if(strcmp(argv[1],"add")==0)
    {
      if(stat(argv[2],&st)==-1) //testez daca exista, iar daca nu exista directorul,il adaug
	{
	  mkdir(argv[2],0700);
	}
      int f;
      if ((f=open("treasure.txt", O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))==-1)
	{
	  perror("eroare deschidere fisier de scriere din directorul %s", argv[2]);
	  closedir(argv[2]);
	  exit(-1);
	}
      Treasure x;
      scanf("%d",&x.treasure_id);
      scanf("%29s",&x.name);
      scanf("%f",&x.latitudine);
      scanf("%f",&x.longitudine);
      scanf("%99s",&x.clue);
      scanf("%d",&value);
      char output[2000];
      int len=snprintf(output,sizeof(output),"Tresure_id:%d\nUser name:%s\nLatitudine:%2f\n,Longitudine:%2f\nClue:%s\nValue:%d\n",x.treasure_id,x.name,x.latitudine,x.longitudine,x.clue,x.value);
      if(write(f,output,len)==-1)
	{
	  perror("eroare scriere fisier");
	  close(f);
	  exit(-1);
	}
    }
  return 0;
}
