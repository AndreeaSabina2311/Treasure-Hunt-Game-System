#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

typedef struct{
  int treasure_id;
  char name[30];
  float latitude;
  float longitude;
  char clue[100];
  int value;
}Treasure;

struct stat st={0};
struct stat file_stat;
struct dirent *entry;

int main(int argc, char **argv)
{
  //fisier pentru logare actiuni
  
  char path[256];
  snprintf(path, sizeof(path), "./%s", argv[2]);  // folderul game1, etc.

  char log_file[300];
  snprintf(log_file, sizeof(log_file), "%s/logged_hunt.txt", path);
  int log_fd=open(log_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if(log_fd==-1)
    {
      perror("eroare deschidere fisier de log");
      exit(-1);
    }
	
  //al doilea argument din linia de comanda este ADD
  //adauga o noua comoara la directorul hunt specificat ca al doilea argument in linie de comanda
  if(strcmp(argv[1],"add")==0)
    {
      {
	char path[256];
	snprintf(path, sizeof(path),"./%s",argv[2]);
      
	if(stat(argv[2],&st)==-1) //testez daca exista, iar daca nu exista directorul,il adaug
	  {
	    if(mkdir(argv[2],0700)==-1)
	      {
		perror("eroare creare director");
		exit(-1);
	      }
	  }
	//cer numele fisierului de la utilizator
	char filename[100];
	printf("Introdu numele fisierului (ex: treasure_1.dat): ");
	scanf("%99s", filename);
	//construieste path-ul catre fisierul cu comori
	char treasure_file[300];
	snprintf(treasure_file, sizeof(treasure_file), "%s/%s",argv[2],filename);
      
	int f;
	if ((f=open(treasure_file, O_WRONLY | O_CREAT | O_APPEND , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))==-1)
	  {
	    perror("eroare deschidere fisier de scriere din directorul adaugat");
	    exit(-1);
	  }
	Treasure x;
	printf("ID: "); 
	scanf("%d", &x.treasure_id);
	while(getchar() != '\n'); // curăță newline-ul din buffer

	printf("Username: "); 
	scanf("%29s", x.name);
	while(getchar() != '\n');

	printf("Latitude: "); 
	scanf("%f", &x.latitude);
	while(getchar() != '\n');

	printf("Longitude: "); 
	scanf("%f", &x.longitude);
	while(getchar() != '\n');

	printf("Clue: "); 
	scanf(" %[^\n]", x.clue);  // citește linie cu spații
	printf("Value: "); 
	scanf("%d", &x.value);
      
      
	//scriem in fisier 
	if(write(f,&x,sizeof(Treasure))==-1)
	  {
	    perror("eroare scriere fisier");
	    close(f);
	    exit(-1);
	  }
	if(close(f)==-1)
	  {
	    perror("Eroare inchidere fisier");
	    exit(-1);
	  }

	//logam actiunea

	char log_entry[512];
	int len=snprintf(log_entry, sizeof(log_entry),"Added treasure %d by user %s\n",x.treasure_id,x.name);
	if(write(log_fd, log_entry, len) ==-1)
	  {
	    perror("eroare la scriere in log");
	    close(log_fd);
	    exit(-1);
	  }
	if(close(log_fd)==-1)
	  {
	    perror("eroare inchidere fisier de log");
	    exit(-1);
	  }
	printf("Comoara a fost adaugata cu succes!\n");
      }
    }
  //daca al doilea argument din linia de comanda este LIST
  //afiseaza toate comorile din vanatoarea specificata prin hunt_id (toate datele din director)
  //nume director, (total) file size si ultima modificare al fisierului(fisierelor) al comorii, apoi comorile
  else if(strcmp(argv[1],"list")==0)
    {
      DIR *dir;
      //char path[256];
      snprintf(path, sizeof(path),"./%s",argv[2]);

      if((dir=opendir(path))==NULL)
	{
	  perror("Nu s-a putut deschide directorul");
	  exit(-1);
	}
      printf("Hunt name:%s\n", argv[2]);

      off_t total_size=0;
      time_t last_mod_time=0;

      while((entry=readdir(dir))!=NULL)
	{
	  if(strstr(entry->d_name,".dat")!=NULL)
	    {
	      char filepath[512];
	      snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

	      int f;
	      if ((f=open(filepath, O_RDONLY))==-1)
		{
		  perror("eroare deschidere fisier de scriere din directorul dat ca argument");
		  continue;
		}
	      if(fstat(f,&file_stat)==-1)
		{
		  perror("eroare obtinere dimensiune fisier din director");
		  close(f);
		  continue;
		}

	      total_size = total_size + file_stat.st_size;
	      if(file_stat.st_mtime > last_mod_time)
		{
		  last_mod_time=file_stat.st_mtime;
		}

	      //citeste si afiseaza comorile
	      Treasure x;
	      printf("\nDin fisierul: %s\n", entry->d_name);
	      while(read(f,&x,sizeof(Treasure))==sizeof(Treasure))
		{
		  printf("ID: %d\n",x.treasure_id); 
		  printf("Username: %s\n",x.name);
		  printf("Latitude: %2f\n", x.latitude);
		  printf("Longitude: %2f\n", x.longitude);
		  printf("Clue: %s\n",x.clue);
		  printf("Value: %d\n",x.value);
		}
	      close(f);
	    }
	}
      closedir(dir);
      printf("\nThe (total) file size: %ld bytes\n", total_size);
      printf("Last modification time: %s",ctime(&last_mod_time));

      //logam actiunea
      char log_entry[512];
	int len=snprintf(log_entry, sizeof(log_entry),"Listed all treasures from hunt %s\n",argv[2]);
	if(write(log_fd, log_entry, len) ==-1)
	  {
	    perror("eroare la scriere in log");
	    close(log_fd);
	    exit(-1);
	  }
	if(close(log_fd)==-1)
	  {
	    perror("eroare inchidere fisier de log");
	    exit(-1);
	  } 
    }
  //vizualizeaza datele unei comori specificate prin id
  //va gasi mai intai directorul dupa hunt_id, iar apoi se uita in toate fisierele ca sa gaseasca comoara dupa treasure_id si afiseaza detaliile (username, latitude, longitude, clue, value)
  else if(strcmp(argv[1],"view")==0)
    {
      DIR *dir;
      snprintf(path, sizeof(path),"./%s",argv[2]);

      if((dir=opendir(path))==NULL)
	{
	  perror("Nu s-a putut deschide directorul");
	  exit(-1);
	}
      int continua=1; //continui sa parcurg directorul vanatorii pana cand am gasit comoara; dupa ce am gasit-o, m-am oprit
      while((entry=readdir(dir))!=NULL && continua==1)
	{
	  if(strstr(entry->d_name,".dat")!=NULL)
	    {
	      char filepath[512];
	      snprintf(filepath,sizeof(filepath),"%s/%s",path,entry->d_name);

	      int f;
	      f=open(filepath,O_RDONLY);
	      if(f==-1)
		{
		  perror("Eroare deschidere fisier din directorul dat ca argument");
		  continue;
		}
	      Treasure x;
	      int id_arg=atoi(argv[3]);
	      while(read(f,&x,sizeof(Treasure))==sizeof(Treasure) && continua==1)
		{
		  if(x.treasure_id==id_arg)
		    {
		      printf("ID: %d\n",x.treasure_id); 
		      printf("Username: %s\n",x.name);
		      printf("Latitude: %2f\n", x.latitude);
		      printf("Longitude: %2f\n", x.longitude);
		      printf("Clue: %s\n",x.clue);
		      printf("Value: %d\n",x.value);
		      continua=0;
		    }
		  if(continua==0)
		     printf("\nComoara cu id-ul %s s-a gasit in fisierul: %s",argv[3], entry->d_name);
		}
	     
	      close(f);
	    }
	}
      closedir(dir);
      //logam actiunea
      char log_entry[512];
      int len=snprintf(log_entry, sizeof(log_entry),"Treasure %s from hunt %s was viewed\n",argv[3],argv[2]);
      if(write(log_fd, log_entry, len) ==-1)
	{
	  perror("eroare la scriere in log");
	  close(log_fd);
	  exit(-1);
	}
      if(close(log_fd)==-1)
	{
	  perror("eroare inchidere fisier de log");
	  exit(-1);
	} 
    }
  //remove_treasure: va sterge o comoara dintr-un fisier
  //se va cauta vanatoare dupa argv[1], iar apoi se va cauta prin fisiere comoara de sters
  //se va cauta ca la view. cand am gasit-o, o elimin
  else if(strcmp(argv[1],"remove_treasure")==0)
    {
      DIR *dir;
      snprintf(path, sizeof(path),"./%s",argv[2]);

      if((dir=opendir(path))==NULL)
	{
	  perror("Nu s-a putut deschide directorul");
	  exit(-1);
	}
      int id_arg=atoi(argv[3]);
      int continua=1; //continui sa parcurg directorul vanatorii pana cand am gasit comoara; dupa ce am gasit-o, m-am oprit
      while((entry=readdir(dir))!=NULL && continua==1)
	{
	  if(strstr(entry->d_name,".dat")!=NULL)
	    {
	      char filepath[512];
	      snprintf(filepath,sizeof(filepath),"%s/%s",path,entry->d_name);

	      int f;
	      f=open(filepath,O_RDONLY);
	      if(f==-1)
		{
		  perror("Eroare deschidere fisier din directorul dat ca argument");
		  continue;
		}

	      //creem fisierul temporar
	      char temp_filepath[1024];
	      snprintf(temp_filepath,sizeof(temp_filepath),"%s/temp_%s",path,entry->d_name);
	      int temp_f;
	      temp_f=open(temp_filepath,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR| S_IWUSR);
	      if(temp_f==-1)
		{
		  perror("Eroare deshidere fisier temporar pentru a scrie(copia) fisierul original in el");
		  close(f);
		  continue;
		}
	      
	      Treasure x;
	      while(read(f,&x,sizeof(Treasure))==sizeof(Treasure))
		{
		  if(x.treasure_id==id_arg)
		    {
		      continua=0;
		      continue; //nu copiem comoara pe care vrem sa o stergem
		    }
		  write(temp_f, &x, sizeof(Treasure));
		}
	      close(f);
	      close(temp_f);

	      if(continua==0)
		{
		  //inlocuim fisierul original cu cel temporar
		  if(remove(filepath)==-1)
		    {
		      perror("Eroare stergere fisier original");
		      exit(-1);
		    }
		  if(rename(temp_filepath, filepath)==-1)
		    {
		      perror("Eroare redenumire fisier temporar");
		      exit(-1);
		    }
		  //logam actiunea
		  char log_entry[512];
		  int len=snprintf(log_entry, sizeof(log_entry),"Treasure %s from hunt %s was removed\n",argv[3],argv[2]);
		  if(write(log_fd, log_entry, len) ==-1)
		    {
		      perror("eroare la scriere in log");
		      close(log_fd);
		      exit(-1);
		    }
		  if(close(log_fd)==-1)
		    {
		      perror("eroare inchidere fisier de log");
		      exit(-1);
		    } 
		}
	      else //daca nu s-a gasit comoara, stergem fisierul temporar
		{
		  remove(temp_filepath);
		}

	  
	    }
	     
	}
      closedir(dir);
      if(continua==1)
	{
	  printf("Nu s-a gasit comoara cu id-ul %s in vanatoarea %s.\n",argv[3], argv[2]);
	}
      else
	{
	  printf("Comoara a fost stearsa din fisierul %s.\n",  entry->d_name);
	}
    }
  //remove_hunt <hunt_id>
  //pasi:verific daca exista (daca nu exista ma opresc cu un perror si un exit); sterg directorul cu tot ce mai contine folosind rm -r nume_director
  //sa intrebi la lab ce faci cu logged_hunt daca tu stergi directorul in care se afla si logged_hunt!!!
  //eu cred ca nu vei mai avea nevoie de fisierul text cu toate operatiile si deci se va sterge si el cu restul fisierelor din director si doar afisezi pe ecran ca s-a sters directorul
  return 0;
}
