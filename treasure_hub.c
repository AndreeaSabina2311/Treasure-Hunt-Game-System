#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>

pid_t monitor_pid = -1;
int monitor_activ = 0;
int monitor_terminating = 0;

struct sigaction sa;
struct dirent *entry;

//functii pentru semnale

/*
void handle_signal(int sig)
{
  // folosit doar de monitor
    if (sig == SIGUSR1) {
        int fd = open("monitor_cmd.txt", O_RDONLY);
        if (fd<0) {
            perror("Monitor: nu pot deschide fișierul de comenzi");
            return;
        }

       char cmd[256];
        int n = read(fd, cmd, sizeof(cmd) - 1);
        if (n < 0) {
            perror("Monitor: eroare la citirea comenzii din fișier");
            close(fd);
            return;
        }
        cmd[n] = '\0';

        // Elimină newline dacă există
        char *newline = strchr(cmd, '\n');
        if (newline) *newline = '\0';

        close(fd);

        if (strcmp(cmd, "list_hunts") == 0) {
	  pid_t child = fork();
	  if (child == 0) {
	    execl("./treasure_manager", "./treasure_manager", "--list_hunts", NULL);
	    perror("Monitor: exec eșuat pentru list_hunts");
	    exit(1);
	  } else if (child > 0) {
	    waitpid(child, NULL, 0);
	  } else {
	    perror("Monitor: fork eșuat");
	  }
        } else if (strncmp(cmd, "--list_treasures ", 17) == 0) {
            char *hunt_id = cmd + 17;
	    while( *hunt_id==' ') hunt_id++;
	    if(*hunt_id=='\0')
	      {
		printf("Monitor: lipseste id-ul vanatorului pentru list_treasures\n");
	      }
	    else{
	      pid_t child = fork();
	      if (child == 0) {
		execl("./treasure_manager", "./treasure_manager", "--list_treasures", hunt_id, NULL);
		perror("Monitor: exec eșuat pentru list_treasures");
		exit(1);
	      } else if (child > 0) {
		waitpid(child, NULL, 0);
	      } else {
		perror("Monitor: fork eșuat");
	      }
	    }
        } else if (strncmp(cmd, "--view_treasure ", 16) == 0) {
	  char *args = cmd + 16;
	  char args_copy[256];
	    strncpy(args_copy, args, sizeof(args_copy)-1);
	    args_copy[sizeof(args_copy)-1]='\0';
	    
            char *hunt = strtok(args_copy, " ");
            char *treasure_id = strtok(NULL, " ");

            if (hunt && treasure_id) {
	      pid_t child = fork();
	      if (child == 0) {
		execl("./treasure_manager", "./treasure_manager", "--view_treasure", hunt, treasure_id, NULL);
		perror("Monitor: exec eșuat pentru view_treasure");
		exit(1);
	      } else if (child > 0) {
		waitpid(child, NULL, 0);
	      } else {
		perror("Monitor: fork eșuat");
	      }
            } else {
	      perror("Monitor: argumente invalide pentru view_treasure");
            }
        } else {
            perror("Monitor: comandă necunoscută");
        }
    } else if (sig == SIGTERM) {
      printf("Monitorul se închide...\n");
      usleep(1000000);
    }
}
*/

void setup_signal_handlers()
{
  sa.sa_handler=SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags=0;

  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGUSR2, &sa, NULL);
}

//functii pentru procesul monitor
/*
void monitor_loop()
{
  setup_signal_handlers();//activeaza functiile care trateaza semnalele
  printf("Monitorul a pornit (PID: %d) și așteaptă comenzi prin semnale...\n", getpid());

  while(1)
    {
      pause(); //procesul asteapta pasiv sa primeasca un semnal
    }
  printf("Monitorul iese acum\n");
  exit(0);
}
*/
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
      // monitor_loop();
      // exit(0);
      setup_signal_handlers();
      printf("Monitorul a pornit (PID: %d) si asteapta comenzi",getpid());
      while(1) pause();
    }
  monitor_activ=1;
  monitor_terminating=0;
  printf("Procesul monitor a inceput cu PID %d\n", monitor_pid);
}

void stop_monitor()
{
  if (!monitor_activ) {
        printf("Monitorul nu este activ.\n");
        return;
    }

    kill(monitor_pid, SIGTERM);
    monitor_activ = 0;
    monitor_terminating = 1;

    // așteptăm să se termine
    int status;
    waitpid(monitor_pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Monitorul s-a închis cu codul %d.\n", WEXITSTATUS(status));
    } else {
        printf("Monitorul s-a închis necontrolat.\n");
    }

    monitor_pid = -1;
    monitor_terminating = 0;
}

/*
void send_command_to_monitor(const char *cmd) {
    if (!monitor_activ) {
        perror("Eroare: Monitorul nu este activ. Porniți-l cu --start_monitor");
        return;
    }

    int fd = open("monitor_cmd.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Eroare la deschiderea fișierului de comandă");
        return;
    }

    if (write(fd, cmd, strlen(cmd)) < 0 || write(fd, "\n", 1) < 0) {
        perror("Eroare la scrierea în fișierul de comandă");
        close(fd);
        return;
    }
    close(fd);

    kill(monitor_pid, SIGUSR1);
    usleep(100000);
}
*/

void run_manager_command(char *args[], const char *label)
{
  int pfd[2];
  if(pipe(pfd)<0)
    {
      perror("Eroare creare pipe pentru run_manager");
      exit(-1);
    }

  int pid=fork();
  if(pid<0)
    {
      perror("Eroare la creare proces fiu cu fork pentru run_manager");
      exit(-1);
    }
  if(pid==0)
    { //proces fiu
      close(pfd[0]);
      dup2(pfd[1],STDOUT_FILENO);
      close(pfd[1]);
      execv("./treasure_manager",args);
      perror("Eroare la execv");
      exit(1);
    }
  else
    {//proces parinte
      close(pfd[1]);
      printf("%s:\n",label);
      char buffer[256];
      int bytes;
      while((bytes=read(pfd[0],buffer,sizeof(buffer)-1))>0)
	{
	  buffer[bytes]='\0';
	  printf("%s",buffer);
	}
      close(pfd[0]);
      waitpid(pid,NULL,0);
      
    }
}

////Functie pentru faza 3
void calculate_scores()
{
  DIR *dir;
  dir=opendir(".");
  if(!dir)
    {
      perror("eroare deschidere director pentru functia calculate_scores");
      exit(-1);
    }

  while((entry=readdir(dir))!=NULL)
    {
      if(entry->d_type == DT_DIR && strncmp(entry->d_name,"game",4)==0)
	{
	  char *args[]={"./calculate_score",entry->d_name,NULL};
	  run_manager_command(args, entry->d_name);
	}
    }

  closedir(dir);
}
int main()
{

  char command[256];


  while(1)
    {
      printf("> ");
      int len = read(STDIN_FILENO, command, sizeof(command) - 1);
      if (len <= 0) {
	perror("Eroare citire comanda");
	exit(1);
      }
      command[len]='\0';
      char *newline = strchr(command, '\n');
      if (newline) *newline = '\0';
      if (monitor_terminating) {
	printf("Monitorul este în curs de oprire. Așteptați finalizarea.\n");
	continue;
      }

      if (strcmp(command, "--start_monitor") == 0) {
	start_monitor();
        }
        else if (strcmp(command, "--stop_monitor") == 0) {
            stop_monitor();
        }
        else if (strcmp(command, "--list_hunts") == 0) {
	  //send_command_to_monitor("list_hunts");
	  char *args[] = {"./treasure_manager", "--list_hunts", NULL};
	  run_manager_command(args, "Listă de vânători");
        }
        else if (strncmp(command, "--list_treasures ", 17) == 0) {
	  //send_command_to_monitor(command);
	  char *hunt_id = command + 18;
	  while (*hunt_id == ' ') hunt_id++;
	  if (*hunt_id == '\0') {
	    printf("ID-ul vânătorii lipsă\n");
	    continue;
	  }
	  char *args[] = {"./treasure_manager", "--list_treasures", hunt_id, NULL};
	  run_manager_command(args, hunt_id);
        }
        else if (strncmp(command, "--view_treasure ", 16) == 0) {
	  //send_command_to_monitor(command);
	  char *params = command + 17;
	  char *hunt_id = strtok(params, " ");
	  char *treasure_id = strtok(NULL, " ");
	  if (!hunt_id || !treasure_id) {
	    printf("Argumente invalide pentru view_treasure\n");
	    continue;
	  }
	  char *args[] = {"./treasure_manager", "--view_treasure", hunt_id, treasure_id, NULL};
	  run_manager_command(args, hunt_id);
        }
	else if(strcmp(command, "--calculate_score")==0)
	  {
	    calculate_scores();
	  }
        else if (strcmp(command, "--exit") == 0) {
            if (monitor_activ) {
                perror("Eroare: Monitorul este încă activ. Opriți-l cu --stop_monitor înainte de a ieși.\n");
                continue;
            }
            break;
        }
        else {
            printf("Comandă necunoscută: %s\n", command);
        }
    }
  return 0;
}
