#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>

typedef struct{
  int treasure_id;
  char name[30];
  float latitude;
  float longitude;
  char clue[100];
  int value;
}Treasure;

pid_t monitor_pid = -1;
int monitor_activ = 0;

#define BASE_PATH "/home/debian/SO/proiect/" //asta este path-ul meu, unde se afla toate lucrurile legate de proiect

struct dirent *entry; //pentru deschidere director
struct dirent *file_entry; //pentru deschidere subdirectoare

void start_monitor()
{
  if(monitor_activ)
    {
      perror("Eroare: procesul monitor este deja activ");
      return;
    }

  monitor_pid=fork();
  if(monitor_pid < 0)
    {
      perror("Eroare la crearea procesului monitor");
      return;
    }
  if(monitor_pid == 0)
    {
      execl("./treasure_monitor","./treasure_monitor",NULL);
      perror("execl a esuat");
      exit(1);
    }
  monitor_activ=1;
  printf("Procesul monitor a inceput cu PID %d\n", monitor_pid);
}

void list_hunts()
{
    DIR *dir;

    dir = opendir(BASE_PATH);
    if (dir == NULL)
    {
        perror("Eroare la deschiderea directorului principal");
        return;
    }

    printf("Lista vanatorilor:\n");

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            char subdir_path[1024];
            int len_sub = snprintf(subdir_path, sizeof(subdir_path), "%s/%s", BASE_PATH, entry->d_name);
            if (len_sub < 0 || len_sub >= sizeof(subdir_path))
            {
                fprintf(stderr, "Calea subdirectorului este prea lunga: %s/%s\n", BASE_PATH, entry->d_name);
                continue;
            }

            int treasure_count = 0;

            DIR *subdir = opendir(subdir_path);
            if (subdir == NULL)
            {
                perror("Eroare la deschiderea subdirectorului");
                continue;
            }

            while ((file_entry = readdir(subdir)) != NULL)
            {
                if (file_entry->d_type == DT_REG && strstr(file_entry->d_name, ".dat"))
                {
                    char file_path[1024];
                    int len_file = snprintf(file_path, sizeof(file_path), "%s/%s", subdir_path, file_entry->d_name);
                    if (len_file < 0 || len_file >= sizeof(file_path))
                    {
                        fprintf(stderr, "Calea spre fisierul este prea lunga: %s/%s\n", subdir_path, file_entry->d_name);
                        continue;
                    }

                    int f = open(file_path, O_RDONLY);
                    if (f == -1)
                    {
                        perror("Eroare deschidere fisier de citire");
                        continue;
                    }

                    Treasure x;
                    while (read(f, &x, sizeof(Treasure)) == sizeof(Treasure))
                    {
                        treasure_count++;
                    }
                    close(f);
                }
            }

            printf(" -%s: %d comori\n", entry->d_name, treasure_count);
            closedir(subdir);
        }
    }

    closedir(dir);
}

//list_treasures: afiseaza toate comorile dintr-o vanatoare
//banuiesc ca vanatoarea o primim ca argument

void list_treasures(char *hunt)
{
  DIR *dir;
  char path[1024];
  snprintf(path, sizeof(path),"%s/%s",BASE_PATH,hunt);

  dir=opendir(path);
  if(dir==NULL)
    {
      perror("Eroare la deschiderea directorului");
      exit(-1);
    }

  printf("Vanatoarea: %s\n", hunt);

  while((entry=readdir(dir))!=NULL)
    {
      if(strstr(entry->d_name,".dat")!=NULL)
	{
	  char filepath[1024];
	  int len = snprintf(filepath, sizeof(filepath), "%s/%s",path, entry->d_name);
	  if(len<0 || len>=sizeof(filepath))
	    {
	      fprintf(stderr, "Calea spre fisierul este prea lunga: %s/%s\n", path, entry->d_name);
                continue;
	    }

	  int f=open(filepath, O_RDONLY);
	  if(f==-1)
	    {
	      perror("Eroare deschidere fisier .dat");
	      continue;
	    }
	  Treasure x;
	  printf("\nDin fisierul: %s\n", entry->d_name);
	  while(read(f,&x,sizeof(Treasure)) == sizeof(Treasure))
	    {
	      printf("ID: %d\n", x.treasure_id);
	      printf("Username: %s\n", x.name);
	      printf("Latitude: %f\n", x.latitude);
	      printf("Longitude: %f\n", x.longitude);
	      printf("Clue: %s\n", x.clue);
	      printf("Value: %d\n", x.value);
	    }
	  close(f);
	}
    }
  closedir(dir);
}


//view_treasure: tells the monitor to show the information about a treasure in hunt
//primesc ca argument id-ul cautat; parcurg fiecare vanatoare(subdirector), iar in fiecare vanatoare parcurg fiecare fisier .dat in care caut id-ul si afisez apoi datele despre comoara cu id-ul dat
void view_treasure(int id)
{
    DIR *dir;
    dir = opendir(BASE_PATH);
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului principal");
        exit(-1);
    }

    int gasit = 0;

    while ((entry = readdir(dir)) != NULL && gasit == 0) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char subdir_path[1024];
            int len_subdir = snprintf(subdir_path, sizeof(subdir_path), "%s/%s", BASE_PATH, entry->d_name);
            if (len_subdir < 0 || len_subdir >= sizeof(subdir_path)) {
                fprintf(stderr, "Calea spre subdirector este prea lunga: %s/%s\n", BASE_PATH, entry->d_name);
                continue;
            }

            DIR *subdir = opendir(subdir_path);
            if (subdir == NULL) {
                perror("Eroare la deschiderea subdirectorului");
                continue;
            }

            while ((file_entry = readdir(subdir)) != NULL && gasit == 0) {
                if (file_entry->d_type == DT_REG && strstr(file_entry->d_name, ".dat")) {
                    char file_path[1024];
                    int len_file = snprintf(file_path, sizeof(file_path), "%s/%s", subdir_path, file_entry->d_name);
                    if (len_file < 0 || len_file >= sizeof(file_path)) {
                        fprintf(stderr, "Calea spre fisierul este prea lunga: %s/%s\n", subdir_path, file_entry->d_name);
                        continue;
                    }

                    int f = open(file_path, O_RDONLY);
                    if (f == -1) {
                        perror("Eroare la deschiderea fisierului");
                        continue;
                    }

                    Treasure x;
                    while (read(f, &x, sizeof(Treasure)) == sizeof(Treasure)) {
                        if (x.treasure_id == id) {
                            printf("Comoara gasita in vanatoarea: %s (fisier: %s)\n", entry->d_name, file_entry->d_name);
                            printf("ID: %d\n", x.treasure_id);
                            printf("Username: %s\n", x.name);
                            printf("Latitude: %f\n", x.latitude);
                            printf("Longitude: %f\n", x.longitude);
                            printf("Clue: %s\n", x.clue);
                            printf("Value: %d\n", x.value);
                            gasit = 1;
                            break;
                        }
                    }

                    close(f);
                }
            }

            closedir(subdir);
        }
    }

    closedir(dir);

    if (!gasit) {
        printf("Nu s-a gasit comoara cu ID-ul %d in nicio vanatoare.\n", id);
    }
}



int main()
{
  
  return 0;
}
