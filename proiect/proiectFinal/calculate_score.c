#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_USERS 100

struct dirent *entry;

typedef struct{
  int treasure_id;
  char name[30];
  float latitude;
  float longitude;
  char clue[100];
  int value;
}Treasure;

typedef struct{
  char name[30];
  int score;
}UserScore;

UserScore score_array[MAX_USERS];
int user_count=0;

void update_score(char name[30], int value)
{
  int i;
  for(i=0;i<user_count;i++)  //daca am utilizatori, ii parcurg sa verific daca nu il am deja in lista
    {
      if(strcmp(score_array[i].name,name)==0) //daca exista deja in lista, ii modific scorul
	{
	  score_array[i].score=score_array[i].score+value;
	  return;
	}
    }
  //daca s-a terminat for-ul si nu am iesit din functie cu acel return; inseamna ca nu exista in lista si il adaug
  strcpy(score_array[user_count].name,name);
  score_array[user_count].score=value;
  user_count++;
}

int main(int argc, char **argv)
{
  int i;
  char path[512];
  DIR *dir;
  if(argc!=2)
    {
      perror("Numarul de argumente nu este 2!");
      exit(-1);
    }

  dir=opendir(argv[1]);
  if(!dir)
    {
      perror("eroare deschidere director");
      exit(-1);
    }

  //parcurg toate fisierele din director
  while((entry=readdir(dir))!=NULL)
    {
      if(strstr(entry->d_name,".dat"))
	{
	  snprintf(path, sizeof(path),"%s/%s",argv[1],entry->d_name); //construiesc calea completa catre fisier
	  int fd;
	  fd=open(path, O_RDONLY);
	  if(fd==-1)
	    {
	      perror("Eroare deschidere fisier pentru citirea datelor pentru a calcula scorul");
	      continue;
	    }

	  Treasure x;
	  while(read(fd,&x,sizeof(Treasure))==sizeof(Treasure))
	    {
	      update_score(x.name,x.value);
	    }
	  close(fd);
	}
    }
  
  closedir(dir);

  for(i=0;i<user_count;i++)
    {
      printf("%s: %d\n", score_array[i].name, score_array[i].score);
    }
  return 0;
}
